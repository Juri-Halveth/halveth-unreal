# Character construction and response

The current runtime adds independent head/eye/hand/finger response, world-space
stance targets, two-bone limb placement, eight morph targets and actual
`ACharacter` walking/falling. See [motion and ground response](MOTION-EARTH-SKY.md)
for the current implementation, physics scope and renderer repair.

Scarlet, Lucinet and Rachel use the source-bound CC0 MakeHuman body topology,
rig, weights and individual head targets. The active builder now fits authored
CC0 garment, footwear and hair meshes to that anatomy through their MHCLO
three-vertex affine binding, scale and offset contract. Scarlet wears a ruby
shirt and trousers with a ponytail; Lucinet wears a blue shirt and trousers with
short hair; Rachel wears a blouse and skirt with a bob. Shoes have modeled soles
and a separate texture/material slot. The old cylindrical coats, tube sleeves,
raised strip lapels, ellipsoid boots and generated hair strips were removed from
the active generator. Each joined mesh has approximately 27–31 thousand vertices
and 163 source bones; the native import adds the FBX root bone.

The authored twelve-second idle animates breathing, slow head/gaze movement and
released arms. In game the guides turn toward a nearby player, blink on staggered
cycles and use their Talk morph during E-triggered dialogue. These are explicit
3D geometry and time-dependent responses. They are not a physical four-dimensional
renderer or autonomous agent minds. The authoring code exposes the rules for further work.

The male macro target now applies to the complete body instead of only the head.
Garments and hair follow the resulting source coordinates. Author-provided
occlusion masks remove covered body faces. Every exported vertex has normalized,
nonnegative skinning weights and valid bone addresses; original garment UVs and
normal maps are retained. Hair uses its alpha texture as a masked material.
The importer explicitly selects the legacy FBX route after texture imports load
Interchange, creates its task after those imports to retain a live UObject, maps
known material slots and fails unknown slots. Hair texture names and hair material
names no longer collide in Unreal's case-insensitive package namespace.

The runtime derives anatomical axes before narrowing the rigging stance to 55%
of its original width. Limb solving moves hips/knees into that stance rather than
scaling the body. The body/development increment relaxes hand targets to 99.3%
of arm reach with opposed arm swing while walking. The footwear source extends about 3.2 cm below the body
origin; the foot contact target includes that sole clearance.

`ArtSource/Characters/SOURCES.json` binds source URLs and selected skin members.
`Sources/targets/sources.json` pins target revisions and hashes.
`wardrobe-sources.json` binds the selected authored CC0 pack members and their
license headers. Character FBX, textures and portable builders are included;
`.blend` files, editor
caches and imported runtime Content are excluded. `CHARACTERS.sh build` needs
Blender 4.5 plus a Python environment with NumPy and Pillow. `GARDEN.sh prepare`
imports the generated art into the native game.

The portable builder was run from a separate copied source tree. The FBX writer
receipt verifies that path redaction leaves parsed numeric geometry, skeleton and
animation unchanged. Native imports and sampled animation checks then passed.
Fine facial art, full-body motion capture, cloth simulation, fantasy-specific
wardrobes and high-quality facial corrective shapes remain open. The supplied
screenshots show the native prototype. No Nintendo geometry, textures or
animations were copied from the Zelda reference. Blender Studio Rain and Epic
MetaHuman were researched as separate pipelines; their assets were not imported.
