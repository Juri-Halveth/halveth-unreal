# SPDX-License-Identifier: MIT
"""Eight authored adult hm08 characters with continuous, refined full skin.

Character art, not cellular, tissue, ageing or biological simulation. Old
clothes, footwear, hair cards and covered-skin masks are absent from exports;
their original source data remains on disk for preservation.
"""
from pathlib import Path
import bpy
import hashlib
import json
import math
import random
import sys
import numpy as np
from mathutils import Vector, Quaternion
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from character_identities import IDENTITIES, MORPHS, SOURCE_COMMIT, macro_targets, required_targets

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT/'ArtSource/Characters/Sources'
DEST = ROOT/'ArtSource/Characters'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_body():
    points, uv, faces, group = [], [], [], ''
    for raw in (SRC/'base.obj').read_text().splitlines():
        a = raw.split()
        if not a:
            continue
        if a[0] == 'v':
            points.append(Vector(tuple(map(float, a[1:4]))))
        elif a[0] == 'vt':
            uv.append(tuple(map(float, a[1:3])))
        elif a[0] == 'g':
            group = ' '.join(a[1:])
        elif a[0] == 'f' and group == 'body':
            faces.append([tuple(int(q)-1 for q in value.split('/')[:2]) for value in a[1:]])
    used = sorted({v for face in faces for v, t in face})
    return points, uv, faces, used, {v: i for i, v in enumerate(used)}


BASE, UV, FACES, USED, INDEX = read_body()
RIG = json.loads((SRC/'default.mhskel').read_text())
WEIGHTS = json.loads((SRC/'default_weights.mhw').read_text())['weights']
TARGET_ENTRIES = {e['path']: e for e in json.loads((SRC/'targets/sources.json').read_text())['targets']}
TARGETS = {}
for target in required_targets():
    path, entry = SRC/'targets'/target, TARGET_ENTRIES[target]
    if entry.get('sourceCommit') != SOURCE_COMMIT or sha(path) != entry['sha256']:
        raise ValueError('Target binding changed: '+target)
    lines = [s.split() for s in path.read_text().splitlines() if s.strip() and not s.startswith('#')]
    if not lines:
        raise ValueError('An active target must contain geometry: '+target)
    TARGETS[target] = [(int(a[0]), Vector(tuple(map(float, a[1:])))) for a in lines]


def smooth(lo, hi, x):
    t = max(0., min(1., (x-lo)/(hi-lo)))
    return t*t*(3.-2.*t)


def bump(center, radius, x):
    return max(0., 1.-((x-center)/radius)**2)**2


def joint(points, key):
    indices = RIG['joints'][key]
    return sum((points[i] for i in indices), Vector())/len(indices)


def anatomy(character):
    points = [p.copy() for p in BASE]
    targets = {**macro_targets(character), **character['faceTargets']}
    for path, weight in targets.items():
        for index, delta in TARGETS[path]:
            points[index] += delta*weight
    low = min(points[i].y for i in USED)
    span = max(points[i].y for i in USED)-low
    q = character['proportions']
    # Regional authored changes move the same anatomical helper vertices used
    # to rebuild the rig. They are more than one global X/Y scale.
    for p in points:
        u = (p.y-low)/span
        arm = smooth(1.1, 2.4, abs(p.x))*smooth(.53, .68, u)
        p.x *= (1.+(q['shoulders']-1.)*bump(.78, .14, u)
                +(q['hips']-1.)*bump(.49, .13, u)
                +(q['waist']-1.)*bump(.61, .10, u)+(q['arms']-1.)*arm)
        p.z *= 1.+(q['depth']-1.)*bump(.69, .23, u)
        p.y += q['legs']*span*smooth(.02, .55, u)
    low = min(points[i].y for i in USED)
    scale = character['authoredHeightCm']/(max(points[i].y for i in USED)-low)

    def convert(p):
        return Vector((p.x*scale, -p.z*scale, (p.y-low)*scale))
    return points, convert, targets


def material(name, color, rough=.6, texture=None):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = (*color, 1)
    shader.inputs['Roughness'].default_value = rough
    shader.inputs['Specular IOR Level'].default_value = .28
    if texture:
        sample = mat.node_tree.nodes.new('ShaderNodeTexImage')
        sample.image = bpy.data.images.load(str(texture), check_existing=True)
        tint = mat.node_tree.nodes.new('ShaderNodeMixRGB')
        tint.blend_type = 'MULTIPLY'
        tint.inputs[0].default_value = 1.
        tint.inputs[2].default_value = (*color, 1)
        mat.node_tree.links.new(sample.outputs['Color'], tint.inputs[1])
        mat.node_tree.links.new(tint.outputs['Color'], shader.inputs['Base Color'])
    return mat


def mesh(name, points, faces, mat, face_uv=None):
    data = bpy.data.meshes.new(name)
    data.from_pydata(points, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    data.materials.append(mat)
    for polygon in data.polygons:
        polygon.use_smooth = True
    if face_uv:
        layer = data.uv_layers.new(name='UVMap')
        for polygon, values in zip(data.polygons, face_uv):
            for loop, uv in zip(polygon.loop_indices, values):
                layer.data[loop].uv = uv
    return obj


def rig(obj, arm, groups):
    for bone, values in groups.items():
        vg = obj.vertex_groups.new(name=bone)
        for vertex, weight in values:
            vg.add([vertex], weight, 'REPLACE')
    modifier = obj.modifiers.new('Skin', 'ARMATURE')
    modifier.object = arm
    obj.parent = arm


def make_armature(points, convert, sole):
    data = bpy.data.armatures.new('HumanRig')
    arm = bpy.data.objects.new('HumanRig', data)
    bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for name, source in RIG['bones'].items():
        bone = data.edit_bones.new(name)
        bone.head = convert(joint(points, source['head']))-Vector((0, 0, sole))
        bone.tail = convert(joint(points, source['tail']))-Vector((0, 0, sole))
        if (bone.tail-bone.head).length < .01:
            bone.tail.z += .05
    for name, source in RIG['bones'].items():
        if source['parent']:
            data.edit_bones[name].parent = data.edit_bones[source['parent']]
    bpy.ops.object.mode_set(mode='OBJECT')
    arm.select_set(False)
    return arm


def add_body_keys(body, points, convert):
    body.shape_key_add(name='Basis')
    keys = {name: body.shape_key_add(name=name) for name in MORPHS}
    eyes = [convert(joint(points, RIG['bones']['eye.'+side]['head'])) for side in ('L', 'R')]
    for src, new in INDEX.items():
        s, p = BASE[src], body.data.vertices[new].co
        # Selection retains source vertex identities after macro targets;
        # closure itself uses the resulting adult anatomical eye centres.
        eye = min(abs(s.x-.30775), abs(s.x+.30775))
        if eye < .205 and 7.18 < s.y < 7.39 and s.z > 1.17:
            center = min(eyes, key=lambda c: abs(p.x-c.x))
            weight = max(0., 1.-(eye/.205)**4)
            keys['Blink'].data[new].co.z += (center.z+.025-p.z)*weight*.98
        if abs(s.x) < .46 and 6.57 < s.y < 6.99 and s.z > 1.20:
            weight = bump(6.83, .27, s.y)*bump(0., .48, s.x)
            keys['Talk'].data[new].co.z -= .45*weight
            keys['Talk'].data[new].co.y -= .06*weight
        if 4.6 < s.y < 6.15 and abs(s.x) < 1.0:
            weight = bump(5.48, .85, s.y)*max(0., min(1., s.z+.2))
            keys['Breath'].data[new].co.y -= .24*weight
        if 6.7 < s.y < 7.0 and .16 < abs(s.x) < .48 and s.z > 1.15:
            weight = bump(6.84, .17, s.y)*smooth(.13, .36, abs(s.x))
            keys['Smile'].data[new].co.z += .25*weight
            keys['Smile'].data[new].co.x += math.copysign(.10*weight, s.x)
        jaw = bump(6.60, .37, s.y)*smooth(.20, .62, abs(s.x))
        if s.z > .62:
            keys['IdentityJaw'].data[new].co.x += math.copysign(.22*jaw, s.x)
        nose = bump(7.08, .32, s.y)*bump(0., .24, s.x)*smooth(1.3, 1.5, s.z)
        keys['IdentityNose'].data[new].co.x += math.copysign(.12*nose, s.x)
        keys['IdentityNose'].data[new].co.y -= .08*nose
        cheek = bump(7.03, .25, s.y)*bump(.55, .34, abs(s.x))*smooth(.8, 1.2, s.z)
        keys['IdentityCheek'].data[new].co.x += math.copysign(.14*cheek, s.x)
        keys['IdentityCheek'].data[new].co.y -= .12*cheek
        brow = bump(7.46, .18, s.y)*bump(.34, .29, abs(s.x))*smooth(.9, 1.2, s.z)
        keys['IdentityBrow'].data[new].co.z += .15*brow


def subdivide_all_keys(body):
    """Apply one identical Catmull-Clark evaluation to basis and every key.

    The evaluated basis carries UV and interpolated deform weights. Keeping
    the object retains the vertex-group name mapping when its mesh is replaced.
    An armature is attached only after all shape samples have been collected.
    """
    # A separate key-free input mesh for every sample avoids dependency-graph
    # caching of live shape values in background Blender. Each operator sees
    # explicitly supplied coordinates, with exactly the same UV/deform layer.
    snapshots, dense, count = {}, None, None
    for name in ('Basis', *MORPHS):
        source = body.data.shape_keys.key_blocks[name]
        low_coordinates = np.empty(len(source.data)*3, dtype=np.float32)
        source.data.foreach_get('co', low_coordinates)
        temporary = body.copy()
        temporary.data = body.data.copy()
        bpy.context.collection.objects.link(temporary)
        temporary.shape_key_clear()
        temporary.data.vertices.foreach_set('co', low_coordinates)
        temporary.data.update()
        modifier = temporary.modifiers.new('OneSmoothSurface', 'SUBSURF')
        modifier.subdivision_type = 'CATMULL_CLARK'
        modifier.levels = modifier.render_levels = 1
        modifier.uv_smooth = 'PRESERVE_BOUNDARIES'
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        sample = bpy.data.meshes.new_from_object(temporary.evaluated_get(depsgraph),
            preserve_all_data_layers=True, depsgraph=depsgraph)
        if dense is None:
            dense, count = sample, len(sample.vertices)
        elif len(sample.vertices) != count:
            raise ValueError('Subdivision lost correspondence for '+name)
        else:
            coordinates = np.empty(count*3, dtype=np.float32)
            sample.vertices.foreach_get('co', coordinates)
            snapshots[name] = coordinates
            bpy.data.meshes.remove(sample)
        temporary_data = temporary.data
        bpy.data.objects.remove(temporary, do_unlink=True)
        bpy.data.meshes.remove(temporary_data)
    old = body.data
    body.data = dense
    bpy.data.meshes.remove(old)
    basis = np.empty(count*3, dtype=np.float32)
    dense.vertices.foreach_get('co', basis)
    sole = float(basis.reshape((-1, 3))[:, 2].min())
    basis.reshape((-1, 3))[:, 2] -= sole
    dense.vertices.foreach_set('co', basis)
    dense.update()
    body.shape_key_add(name='Basis')
    for name, coordinates in snapshots.items():
        coordinates.reshape((-1, 3))[:, 2] -= sole
        body.shape_key_add(name=name).data.foreach_set('co', coordinates)
    for polygon in dense.polygons:
        polygon.use_smooth = True
    return sole


def neutral_keys(obj):
    obj.shape_key_add(name='Basis')
    for name in MORPHS:
        obj.shape_key_add(name=name)


def eyeball(center, radius, mat, arm, bone):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, location=center)
    obj = bpy.context.object
    obj.name, obj.scale = 'Eye', (radius, radius, radius)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = True
    neutral_keys(obj)
    rig(obj, arm, {bone: [(v.index, 1.) for v in obj.data.vertices]})


def iris_cap(name, center, forward, radius, low, high, iris_radius, mat, arm, bone):
    # A ring on the eye sphere, not another projecting ellipsoid. Iris UV0 is
    # a centred disk and Pupil shares its coordinates without a second iris.
    right = forward.cross(Vector((0, 0, 1))).normalized()
    up = right.cross(forward).normalized()
    points, faces, uv = [], [], []
    segments, rings = 64, 5
    if low == 0:
        points.append(center+forward*radius)
        uv.append((.5, .5))
    start_row = 1 if low == 0 else 0
    for row in range(start_row, rings+1):
        r = low+(high-low)*row/rings
        depth = math.sqrt(max(0., radius*radius-r*r))
        for col in range(segments):
            angle = col*2.*math.pi/segments
            x, z = math.cos(angle)*r, math.sin(angle)*r
            points.append(center+forward*depth+right*x+up*z)
            uv.append((.5+.5*x/iris_radius, .5+.5*z/iris_radius))
    if low == 0:
        for col in range(segments):
            faces.append((0, 1+(col+1) % segments, 1+col))
    offset = 1 if low == 0 else 0
    rows = rings if low == 0 else rings+1
    for row in range(rows-1):
        for col in range(segments):
            nxt = (col+1) % segments
            faces.append((offset+row*segments+col, offset+row*segments+nxt,
                          offset+(row+1)*segments+nxt, offset+(row+1)*segments+col))
    obj = mesh(name, points, faces, mat, [[uv[i] for i in f] for f in faces])
    if any(p.normal.dot(forward) <= 0 for p in obj.data.polygons):
        raise ValueError('Eye cap winding does not face outwards')
    neutral_keys(obj)
    rig(obj, arm, {bone: [(v.index, 1.) for v in obj.data.vertices]})


def eyebrows(body, arm, eyes, mat, character):
    # Three irregular rows form a tapered brow with visible skin between hairs.
    # Every curve sample is fitted to this exact forehead. There is no solid
    # backing strip or painted geometry, and no reused scalp/ponytail cards.
    bvh = BVHTree.FromPolygons([v.co.copy() for v in body.data.vertices],
                               [p.vertices[:] for p in body.data.polygons])
    points, faces = [], []
    hairs, curve_lengths, root_radii = [], [], []
    for side_index, center in enumerate(eyes):
        rng = random.Random(0xB207+character['identity']*101+side_index*4099)
        direction = 1. if center.x > 0 else -1.
        width = 2.45+.12*(character['identity'] % 3)
        band = .38+.055*(character['identity'] % 4)
        arch_height = .33+.035*(character['identity'] % 3)
        side_hairs = 0
        for row in range(3):
            for strand in range(70):
                t = (strand+.5+rng.uniform(-.40, .40))/70.
                # Reduced edge density and irregular local gaps prevent a
                # rectangular dark bar. The middle row has the fullest roots.
                retention = .70+.26*math.sin(math.pi*t)
                if row != 1:
                    retention -= .08
                if rng.random() > retention:
                    continue
                edge = max(.10, math.sin(math.pi*t)**.65)
                x = center.x+direction*(t-.5)*width
                z = (center.z+1.07+arch_height*math.sin(math.pi*t)+.10*t
                     +(row-1)*band*.5*edge+rng.uniform(-.065, .065))
                length = rng.uniform(.26, .49)
                radius = rng.uniform(.015, .025)
                # Inner hairs rise; outer hairs sweep towards the temple.
                sweep = .18+.78*t+rng.uniform(-.10, .10)
                rise = .94-.70*t+rng.uniform(-.10, .10)
                growth = Vector((direction*sweep, 0, rise)).normalized()
                # Fit a longer source curve, then sample by arc length. Surface
                # projection must not turn a nominal .5 cm hair into .65 cm.
                fitted, fitted_normals = [], []
                bend_scale = rng.uniform(.7, 1.3)
                for sample in range(13):
                    u = sample/12.
                    bend = direction*.035*math.sin(math.pi*u)*bend_scale
                    sample_x = x+growth.x*.60*u+bend
                    sample_z = z+growth.z*.60*u-.045*u*u
                    hit, normal, face_index, distance = bvh.ray_cast(
                        Vector((sample_x, -100., sample_z)), Vector((0, 1, 0)), 200.)
                    if hit is None:
                        break
                    lift = radius*.7+.012+.025*math.sin(math.pi*u)+.025*u*u
                    fitted.append(hit+normal*lift)
                    fitted_normals.append(normal)
                if len(fitted) != 13:
                    continue
                arc = [0.]
                for a, b in zip(fitted, fitted[1:]):
                    arc.append(arc[-1]+(b-a).length)
                if arc[-1] < length:
                    continue
                curve, normals = [], []
                for sample in range(5):
                    distance = length*sample/4.
                    span = next(i for i in range(1, len(arc)) if arc[i] >= distance)
                    fraction = (distance-arc[span-1])/(arc[span]-arc[span-1])
                    curve.append(fitted[span-1].lerp(fitted[span], fraction))
                    normals.append(fitted_normals[span-1].lerp(fitted_normals[span], fraction).normalized())
                actual_length = sum((b-a).length for a, b in zip(curve, curve[1:]))
                if not .25 <= actual_length <= .50:
                    continue
                first, sides = len(points), 6
                phase = rng.uniform(0, math.pi/3.)
                for sample, position in enumerate(curve):
                    u = sample/4.
                    tangent = (curve[min(sample+1, 4)]-curve[max(sample-1, 0)]).normalized()
                    cross = tangent.cross(normals[sample]).normalized()
                    upright = cross.cross(tangent).normalized()
                    thickness = max(.0018, radius*(1.-u)**.8)
                    for col in range(sides):
                        a = phase+col*2.*math.pi/sides
                        points.append(position+(cross*math.cos(a)+upright*math.sin(a))*thickness)
                for sample in range(4):
                    for col in range(sides):
                        nxt = (col+1) % sides
                        faces.append((first+sample*sides+col, first+sample*sides+nxt,
                                      first+(sample+1)*sides+nxt, first+(sample+1)*sides+col))
                # End caps are individual, with no connecting backing surface.
                for col in range(1, sides-1):
                    faces.append((first, first+col+1, first+col))
                    end = first+4*sides
                    faces.append((end, end+col, end+col+1))
                side_hairs += 1
                curve_lengths.append(actual_length)
                root_radii.append(radius)
        hairs.append(side_hairs)
    if not points:
        raise ValueError('Eyebrow forehead intersections were empty')
    obj = mesh('Brows', points, faces, mat)
    neutral_keys(obj)
    for p in obj.data.shape_keys.key_blocks['IdentityBrow'].data:
        p.co.z += .10
    rig(obj, arm, {'head': [(v.index, 1.) for v in obj.data.vertices]})
    return dict(construction='three-row-surface-fitted-curved-strands-v2', rows=3,
        hairsPerSide=hairs, vertices=len(points), faces=len(faces),
        rootRadiusCmRange=[min(root_radii), max(root_radii)],
        actualCurveLengthCmRange=[min(curve_lengths), max(curve_lengths)],
        designedLengthCmRange=[.25, .50], tubeSides=6, curveSamples=5,
        continuousBackingGeometry=False, deterministicPerIdentity=True)


def idle_animation(arm):
    # Native locomotion still owns walking. This is a small source rest idle.
    for frame in range(1, 362, 10):
        t = (frame-1)/30.
        for bone in ('spine03', 'spine02', 'neck03', 'head'):
            pose = arm.pose.bones[bone]
            pose.rotation_mode = 'QUATERNION'
            axis = Vector((1, 0, 0)) if bone.startswith('spine') else Vector((0, 1, 0))
            angle = .012*math.sin(t*2*math.pi/4) if bone.startswith('spine') else .018*math.sin(t*2*math.pi/12)
            pose.rotation_quaternion = Quaternion(axis, angle)
            pose.keyframe_insert('rotation_quaternion', frame=frame, group=bone)
        for side, sign in (('L', 1), ('R', -1)):
            bone = 'upperarm01.'+side
            pose = arm.pose.bones[bone]
            pose.rotation_mode = 'QUATERNION'
            q = arm.data.bones[bone].matrix_local.to_quaternion()
            pose.rotation_quaternion = q.inverted() @ Quaternion(Vector((0, 1, 0)), sign*math.radians(32)+.012*math.sin(t*1.8)) @ q
            pose.keyframe_insert('rotation_quaternion', frame=frame, group=bone)
        pose = arm.pose.bones['spine02']
        pose.scale = (1, 1, 1+.007*math.sin(t*2*math.pi/4))
        pose.keyframe_insert('scale', frame=frame, group='Breath')
    arm.animation_data.action.name = 'HumanIdle'


def bounds(vertices):
    return [[min(v.co[i] for v in vertices), max(v.co[i] for v in vertices)] for i in range(3)]


SKIN_PIXELS = {}


def skin_albedo(character, points):
    """Area-weighted skin UV samples, with explicit sRGB-to-linear transfer.

    Blender's byte-PNG pixel buffer is encoded sRGB (checked against the PNG
    source), so its values must not be treated as linear shader albedo.
    This character-appearance measurement is not a reflectance calibration.
    """
    source = character['skinSource']
    if source not in SKIN_PIXELS:
        image = bpy.data.images.load(str(DEST/(source+'.png')), check_existing=True)
        if image.colorspace_settings.name != 'sRGB' or image.is_float:
            raise ValueError('Skin measurement requires the declared byte sRGB PNG')
        width, height = image.size
        pixels = np.empty(width*height*4, dtype=np.float32)
        image.pixels.foreach_get(pixels)
        SKIN_PIXELS[source] = (width, height, pixels.reshape((height, width, 4)))
    width, height, pixels = SKIN_PIXELS[source]
    tint = np.array(character['skinTintLinear'], dtype=np.float64)
    sums = {kind: np.zeros(3) for kind in ('face', 'body')}
    weights = {kind: 0. for kind in sums}
    counts = {kind: 0 for kind in sums}
    for face in FACES:
        source_position = sum((BASE[v] for v, t in face), Vector())/len(face)
        if source_position.y > 6.35 and source_position.z > 1.05 and abs(source_position.x) < .95:
            region = 'face'
        elif .7 < source_position.y < 6.1:
            region = 'body'
        else:
            continue
        coords = [UV[t] for v, t in face]
        if any(max(uv[a] for uv in coords)-min(uv[a] for uv in coords) > .5 for a in (0, 1)):
            continue
        u, v = (sum(uv[a] for uv in coords)/len(coords) for a in (0, 1))
        x = max(0, min(width-1, int(u*width)))
        y = max(0, min(height-1, int(v*height)))
        encoded = pixels[y, x, :3].astype(np.float64)
        linear = np.where(encoded <= .04045, encoded/12.92, ((encoded+.055)/1.055)**2.4)
        vertices = [points[i] for i, t in face]
        area = sum((vertices[i]-vertices[0]).cross(vertices[i+1]-vertices[0]).length*.5
                   for i in range(1, len(vertices)-1))
        sums[region] += linear*tint*area
        weights[region] += area
        counts[region] += 1
    means = {kind: sums[kind]/weights[kind] for kind in sums}
    result = .6*means['face']+.4*means['body']
    receipt = dict(texture=source+'.png', textureSha256=sha(DEST/(source+'.png')),
        method='source-quad-UV-centroid; area-weighted within region; 0.6 face + 0.4 body',
        transfer='IEC-sRGB-piecewise-to-linear-before-tint', samplePolygons=counts,
        faceAlbedoLinear=means['face'].tolist(), bodyAlbedoLinear=means['body'].tolist(),
        scope='Texture appearance estimate, not measured real skin reflectance')
    return result.tolist(), receipt


def build(character):
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for data in list(bpy.data.actions):
        bpy.data.actions.remove(data)
    for data in list(bpy.data.materials):
        bpy.data.materials.remove(data)
    scene = bpy.context.scene
    scene.render.fps = 30
    scene.frame_start, scene.frame_end = 1, 361
    scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', .01
    points, convert, targets = anatomy(character)
    measured_albedo, albedo_receipt = skin_albedo(character, points)
    skin = material('Skin', character['skinTintLinear'], .59, DEST/(character['skinSource']+'.png'))
    sclera = material('Sclera', (.52, .49, .44), .23)
    iris = material('Iris', character['irisColorLinear'], .38)
    pupil = material('Pupil', (.002, .0025, .003), .2)
    brow = material('Hair', character['browColorLinear'], .72)
    body = mesh('Human', [convert(points[i]) for i in USED],
        [[INDEX[v] for v, t in face] for face in FACES], skin,
        [[UV[t] for v, t in face] for face in FACES])
    # Source deform weights are attached before subdivision; only the
    # armature modifier waits until all shape correspondence is resolved.
    totals = {}
    for values in WEIGHTS.values():
        for src, weight in values:
            if src in INDEX:
                totals[src] = totals.get(src, 0.)+weight
    for bone, values in WEIGHTS.items():
        group = body.vertex_groups.new(name=bone)
        for src, weight in values:
            if src in INDEX and totals[src] > 0:
                group.add([INDEX[src]], weight/totals[src], 'REPLACE')
    add_body_keys(body, points, convert)
    sole = subdivide_all_keys(body)
    if any(not v.groups or abs(sum(g.weight for g in v.groups)-1.) > .002 for v in body.data.vertices):
        raise ValueError('Subdivision lost normalized anatomical weights')
    body_bounds = bounds(body.data.vertices)
    if abs(body_bounds[2][0]) > 1e-5:
        raise ValueError('Barefoot sole is not at zero after subdivision')
    body_count, body_faces = len(body.data.vertices), len(body.data.polygons)
    arm = make_armature(points, convert, sole)
    modifier = body.modifiers.new('Skin', 'ARMATURE')
    modifier.object, body.parent = arm, arm
    eye_receipts, eye_centers = [], []
    for side in ('L', 'R'):
        bone = arm.data.bones['eye.'+side]
        center = bone.head_local.copy()
        forward = (bone.tail_local-bone.head_local).normalized()
        radius = 1.18*(character['authoredHeightCm']/172.)**.15
        eye_centers.append(center)
        eyeball(center, radius, sclera, arm, bone.name)
        iris_radius, pupil_radius = radius*.51, radius*.205
        iris_cap('Iris', center, forward, radius+.008, pupil_radius, iris_radius,
                 iris_radius, iris, arm, bone.name)
        iris_cap('Pupil', center, forward, radius+.009, 0., pupil_radius,
                 iris_radius, pupil, arm, bone.name)
        eye_receipts.append(dict(bone=bone.name, centerCm=list(center), forward=list(forward),
            globeRadiusCm=radius, irisRadiusCm=iris_radius, pupilRadiusCm=pupil_radius,
            irisUV='disk-centered-0.5-radius-0.5-UV0', capSurface='sphere-intersection-ring',
            maxScleraReliefCm=.009))
    brow_geometry = eyebrows(body, arm, eye_centers, brow, character)
    idle_animation(arm)
    # Every part shares the same eight channels; non-face eye components have
    # exact neutral coordinates. No fake clothing/hair channels are required.
    bpy.ops.object.select_all(action='DESELECT')
    for obj in list(scene.objects):
        if obj.type == 'MESH':
            obj.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    scene.frame_set(1)
    body.name = character['name']+'Surface'
    if len(body.data.vertices) > 100000:
        raise ValueError('Character exceeded the source vertex budget')
    slots = [slot.name for slot in body.data.materials]
    if sorted(slots) != sorted(['Skin', 'Hair', 'Sclera', 'Iris', 'Pupil']):
        raise ValueError('Unexpected/duplicated material slots: '+repr(slots))
    morphs, basis = {}, body.data.shape_keys.key_blocks['Basis']
    for name in MORPHS:
        key = body.data.shape_keys.key_blocks[name]
        deltas = [(a.co-b.co).length for a, b in zip(key.data, basis.data)]
        moved = sum(d > 1e-5 for d in deltas)
        if moved == 0:
            raise ValueError('Required morph has no geometry: '+name)
        morphs[name] = dict(changedVertices=moved, maximumDisplacementCm=max(deltas), weightRange=[0., 1.])
        key.slider_min, key.slider_max, key.value = 0., 1., 0.
    path = DEST/(character['name']+'.fbx')
    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, axis_forward='-Y',
        axis_up='Z', apply_unit_scale=True, bake_anim=True,
        bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0, use_mesh_modifiers=False, mesh_smooth_type='FACE',
        use_tspace=True, path_mode='RELATIVE', embed_textures=False)
    bpy.ops.wm.save_as_mainfile(filepath=str(DEST/(character['name']+'.blend')))
    result = dict(character)
    result.update(construction='hm08-adult-smooth-surface-v2',
        vertices=len(body.data.vertices), faces=len(body.data.polygons),
        triangles=sum(len(p.vertices)-2 for p in body.data.polygons),
        bodyVertices=body_count, bodyFaces=body_faces, bones=len(arm.data.bones),
        seconds=12, morphs=list(MORPHS), morphGeometry=morphs,
        materialSlots=slots, bodyMinimumZCm=body_bounds[2][0],
        skinAlbedoLinear=measured_albedo, skinAlbedoMeasurement=albedo_receipt,
        heightCm=body_bounds[2][1]-body_bounds[2][0], boundsCm=bounds(body.data.vertices),
        sourceBoundsCm=body_bounds, subdivision=dict(method='Catmull-Clark', levels=1,
            sameOperatorForAllMorphs=True, UVAndWeightsPreserved=True),
        sourceTopology=dict(vertices=len(USED), quads=len(FACES), fullBody=True,
                            deletedCoveredFaces=0, helperFacesExported=False),
        sourceCommit=SOURCE_COMMIT, macroTargets=macro_targets(character),
        sourceBindings=[dict(path='Sources/targets/'+p, sha256=TARGET_ENTRIES[p]['sha256'], weight=w)
                        for p, w in sorted(targets.items())],
        eyes=eye_receipts, scalp='bald', eyebrows='three-row-surface-fitted-curved-strands-v2',
        eyebrowGeometry=brow_geometry,
        ageInterpretation='Static authored adult age; no biological simulation', fbxSha256=sha(path))
    print('ADULT_HUMAN_BUILT', character['name'], result['vertices'], body_bounds,
          'morphs='+','.join(MORPHS), flush=True)
    return result


records = [build(character) for character in IDENTITIES]
(DEST/'characters.json').write_text(json.dumps(dict(schema='halveth.human-source.v2',
    geometryLicense='CC0 MakeHuman hm08 graphics/targets + CC0 authored eyes/brows/surface additions; MIT builder',
    construction='hm08-adult-smooth-surface-v2', sourceCommit=SOURCE_COMMIT,
    characters=records), indent=2)+'\n', encoding='utf8', newline='\n')
source_manifest = json.loads((DEST/'SOURCES.json').read_text())
source_manifest['activeConstruction'] = 'hm08-adult-smooth-surface-v2'
source_manifest['activeBodyBasis'] = dict(sourceCommit=SOURCE_COMMIT,
    minimumAuthoredAgeYears=25, fullAdultMacroTargets=True,
    targets='Sources/targets/sources.json', staticCharacterArtOnly=True)
source_manifest['inactivePreservedWardrobe'] = 'wardrobe-sources.json'
for key in ('activeWardrobe', 'activeFootwear', 'activeHairCards'):
    source_manifest[key] = None
(DEST/'SOURCES.json').write_text(json.dumps(source_manifest, indent=2)+'\n', encoding='utf8', newline='\n')
print('EIGHT_ADULT_CHARACTER_SOURCES_READY', flush=True)
