"""Local image measurements and human-authored design recipes; no image upload.

A still image does not establish geometry, dynamics, real physics or source code.
Recipes below are implementation hypotheses derived from visible composition.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

RECIPES = {
    '1F7DF442-83F4-4E44-9288-3487BF8FD617': ('industry', 'Interlocking production levels, bridges, active work and visible transport.', 'A connected activity graph; settlements exchange materials through physical routes.'),
    '3D29929A-7CA9-4133-A108-5F510D7BD795': ('library', 'Tall shelves, alcoves, a central spatial catalogue and labelled activities.', 'A walkable archive linked to the existing book/recall system; many spatially separated destinations.'),
    '5AD7289B-B8F4-4D26-9DEF-DB7F71450D6B': ('door-sequence', 'Six panels depict approach, an artifact, activation and an opening door.', 'An explicit alignment state machine, animated thick leaves, clear arrival and cooldown.'),
    '15DB530B-9326-4556-8A8E-3E776FF2927D': ('gate-detail', 'Stone mouldings, metal inlays, ivy and local warm practical lights.', 'Closed arch voussoirs, bevelled pilasters, separate metal roughness and local lights.'),
    '31CDB56D-A6F8-4782-A8CD-7B4E73729182': ('arrival', 'A readable central route through near, middle and far architecture.', 'A continuous colliding path, guard walls, repeated lamps and a dominant entrance silhouette.'),
    'A8751685-7EA6-4173-8BC1-D5CF353BD338': ('threshold', 'A dark workroom frames a sunlit outside world; notes remain in the room.', 'Interior/exterior exposure transition, an actual doorframe and diegetic workspace.'),
    'BD42FE4F-AE8A-4D57-BA41-12281B073D69': ('orbital-depth', 'Nested rings, stepped approach and figures supply depth and scale.', 'True toroidal geometry in independent planes, stable shared origin and bounded rotation.'),
    'D2D37318-0AC8-4CE9-A547-A9EAA9317669': ('first-person', 'Foreground hands and wrist sigils frame an extensive landscape.', 'View-relative casting focus triggered by actual spell success; anatomical player hands remain future work.'),
    'D58F5C31-6A4D-4F03-964A-13CCB0662491': ('floor', 'Uneven stone, fine inlays, joints and shoes anchor the viewpoint.', 'Solid foundation beneath staggered bevelled pavers; inlays follow the same support plane.'),
    'E92A5D33-74D3-4E63-A554-9D2F43E649C8': ('workshop', 'Many devices occupy specific benches and overhead supports.', 'Reusable parameterised devices with a placement/support contract and explicit operating state.'),
    'E825C5CB-55D0-441C-BDFC-00EF87AE005E': ('flight', 'Trails align with flying figures and a visible destination.', 'Movement-derived ribbons with spawn, lifetime, arc-length spacing and an explicit particle budget.'),
    'F657EEA4-19BA-40F0-80AD-2C079CEBB2A3': ('quiet-room', 'Wood, cloth, clutter, vegetation and small light sources suggest habitation.', 'A quiet, navigable home with varied book placement, roughness and local lighting.'),
    'E20CDDDC-A273-4247-99C0-A84394B1701C': ('routes', 'Readable paths and signposts lead toward distinct places.', 'Walkable route topology and destination signs; signs use gameplay destinations.'),
    '25FA6943-2101-4057-8F15-9954D6CC6563': ('convergence', 'A stepped gate combines ring geometry, pillars and central light.', 'Three aligned rings open colliding doors; further use travels to a checked archive landing.'),
    '82103AFF-E118-4C65-9050-D79CC3266525': ('identities', 'Named figures have distinct silhouettes, materials and colours.', 'Stable character IDs and individual art direction; existing guides remain the current characters.'),
    'F0045D82-3295-4119-BE3D-8190360BA4C0': ('terraces', 'Layered inhabited terraces and spell-marked hands supply vertical depth.', 'Supported upper archive terrace, thick slabs, guard walls, local spell geometry.'),
}

# A visible reference feature is not automatically a finished game capability.
IMPLEMENTED = {
    'library': 'Supported upper archive, individually varied volumes, existing book reader.',
    'door-sequence': 'Three-step alignment, colliding sliding leaves, checked two-way travel.',
    'gate-detail': 'Closed arch stones, fluted piers, brass inlays and local movable lights.',
    'arrival': 'A grounded approach, physical stairs and an upper vaulted archive.',
    'threshold': 'A perspective-linked live view and controlled transition between lower gate and upper archive.',
    'first-person': 'Success-triggered spatial focus; anatomical hands are still open.',
    'floor': 'Solid foundation, closed bevelled pavers and three-dimensional inlays.',
    'orbital-depth': 'Three rotating rings with one stable scene-space origin.',
    'flight': 'Unlockable capsule flight; 20 Hz sampled closed-tube trail, 1.2 second lifetime.',
    'routes': 'Local destination signs and a walkable stair route.',
    'convergence': 'Alignment state and two-way portal destination.',
    'terraces': 'Supported upper archive terrace and protective parapets.',
}

def analyze(root: Path, output: Path):
    rows=[]
    for identity,(theme,observed,recipe) in RECIPES.items():
        path=root/(identity+'.PNG')
        raw=path.read_bytes()
        with Image.open(path) as src:
            size=src.size
            rgb=np.asarray(src.convert('RGB').resize((384,256)),dtype=np.float64)/255
        linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)
        luminance=linear@np.array([.2126,.7152,.0722])
        rows.append(dict(file=path.name,sha256=hashlib.sha256(raw).hexdigest(),width=size[0],height=size[1],theme=theme,
                         observed_visual_features=observed,implementation_hypothesis=recipe,
                         implementation_state='PARTIAL_LOCAL_IMPLEMENTATION' if theme in IMPLEMENTED else 'RESEARCH_BRANCH_OPEN',
                         implemented_subset=IMPLEMENTED.get(theme,'No new implementation for this reference theme in this increment.'),
                         relative_linear_luminance_percentiles={str(p):round(float(np.percentile(luminance,p)),5) for p in (10,50,90,99)}))
    result=dict(schema='halveth.astral-reference-recipes.v1',source_count=len(rows),coverage='All 16 supplied still images; no motion, hidden geometry or source-code inference.',
                measurement='sRGB decoded to relative linear luminance after a declared 384x256 thumbnail; not physical scene illuminance.',
                original_media_transfer='NONE',images=rows)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'{len(rows)} references measured; recipes saved to {output.name}; originals not copied.')

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--reference-dir',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'docs/ASTRAL-REFERENCE-RECIPES.json')
    args=parser.parse_args();analyze(args.reference_dir,args.output)
