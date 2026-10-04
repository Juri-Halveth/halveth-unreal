# Character construction and response

The current runtime adds independent head/eye/hand/finger response, world-space
stance targets, two-bone limb placement, eight morph targets and actual
`ACharacter` walking/falling. See [motion and ground response](MOTION-EARTH-SKY.md)
for the current implementation, physics scope and renderer repair.

Scarlet, Lucinet and Rachel use the source-bound CC0 MakeHuman body topology,
rig, weights and individual head targets. Own meshes add coats, raised lapels,
stitched trim, sleeves, boots, eyes, scalp and swept hair strands. Cloth color and
tangent-space normal textures are authored at 1024 pixels. Each exported joined
mesh has approximately 41–42 thousand vertices and 163 source bones.

The authored twelve-second idle animates breathing, slow head/gaze movement and
released arms. In game the guides turn toward a nearby player, blink on staggered
cycles and use their Talk morph during E-triggered dialogue. These are explicit
3D geometry and time-dependent responses. They are not a physical four-dimensional
renderer or autonomous agent minds. The authoring code exposes the rules for further work.

The macro head deformation is transferred to the cap, strands and beard using
a source-coordinate displacement field; facial morph membership remains bound
to the original topology. The importer preserves the same skeletal and animation
contract, maps known material slots and fails unknown slots. Native QA samples
an internal spine bone at two animation times rather than merely checking that
an animation file exists.

`ArtSource/Characters/SOURCES.json` binds source URLs and selected skin members.
`Sources/targets/sources.json` pins target revisions and hashes. Character FBX,
four texture inputs and portable builders are included; `.blend` files, editor
caches and imported runtime Content are excluded. `CHARACTERS.sh build` needs
Blender 4.5 plus a Python environment with NumPy and Pillow. `GARDEN.sh prepare`
imports the generated art into the native game.

The portable builder was run from a separate copied source tree. The FBX writer
receipt verifies that path redaction leaves parsed numeric geometry, skeleton and
animation unchanged. Native imports and sampled animation checks then passed.
Fine facial art, shoulder/cloth detailing, less uniform hair and more individual
body/outfit styles remain open. The supplied screenshots show this prototype.
