# Character reference and implemented rebuild

The supplied reference is [Breath of the Wild, first 30 minutes, BNGamesReviews](https://www.youtube.com/watch?v=dP4dY0C1BOk).
The available quality menu topped out at 1080p60, and the selected player reported
1920 × 1080. It did not expose a 4K source. This inspection is five bound visual
samples, not a complete frame-by-frame review or a measured motion-capture study.

| Source time | Directly visible detail | Construction implication |
|---|---|---|
| 2:57 | Link behind the clothing pickup UI; coherent torso and hip silhouette | Preserve anatomical continuity through the clothing pipeline |
| 10:27 | Link, the old man and a campfire within layered landscape depth | Character and environment need consistent scale and material response |
| 13:33 | Link carries an axe, with coordinated arm/torso stance | Let equipment and gestures affect a joint chain |
| 18:04 | Link occupies a sloping ruined arch | Ground support and leg pose must follow the surface |
| 20:40 | Link in a running stride with carried equipment | Feet, pelvis, shoulder motion and arm swing need a shared temporal pose |

These implications are engineering interpretation of the visible poses. They
do not establish Nintendo's exact solver, topology, animation source or shader
implementation. An advertisement and a later unrelated piano video were excluded.
Reference frames stay outside this repository; no Nintendo art was imported.

Nintendo's own [GDC session](https://www.gdcvault.com/play/1024562/)
discusses coordinated game, art and technical design. The useful lesson here is
to build a consistent character system rather than treat resolution as its source.

## Replaced active construction

The earlier builder assembled cylindrical coats, tube sleeves, strip lapels,
ellipsoid footwear and procedural hair strips. Those routines are absent from
the active generator. The current source surface contains anatomical hm08 bodies,
authored garments, authored hair, footwear, eyes, rig weights and morph channels.
The male macro target now changes the complete anatomical body.

An authored garment vertex is reconstructed from its declared anatomy addresses:

`garment_vertex = sum(coefficient[i] * body_vertex[address[i]]) + axis_scaled_offset`

The geometric coefficients form an affine map; negative coefficients may describe
valid extrapolation. Skeletal influence weights are separately kept nonnegative,
normalized and bound to existing bones. Skinning uses the animated bone matrices
to deform vertices over time. These are different mathematical contracts.

Original UVs retain garment texture placement. Clothing occlusion masks remove
covered skin faces. Normal maps supply small surface detail; masked hair uses the
source alpha channel. Shoes have modeled soles and their own material rather than
inheriting the trouser texture. Actual runtime limb solving relaxes the rigging
stance and hands while retaining traced foot contacts and gravity.

The implemented temporal model is 3D geometry with state at time `t`: pose,
velocity, gaze, speech, breath and contact. It is a controllable animation model.
The native receipts bind its finite tests and source version.

## Selected sources and alternative pipelines

The active garment/hair/footwear source is the explicitly CC0
[MakeHuman system asset pack](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html).
The actual selected members, author/license headers and hashes are preserved in
`ArtSource/Characters/Sources` and `wardrobe-sources.json`.
The geometry reader is an independently written MIT module; it imports mesh data,
not MakeHuman's application implementation. See [the asset license](https://static.makehumancommunity.org/about/license.html).

[Blender Studio Rain](https://studio.blender.org/characters/rain/) is a researched
CC-BY alternative for a more thoroughly authored stylized character rig, with
IK/FK, face controls and corrective deformation. Attribution and its Blender rig
contract would need to travel with an actual integration. Rain was not imported.

[MetaHuman 5.8](https://www.metahuman.com/news/metahuman-5-8-is-now-available)
provides another researched route for rigged human bodies/faces and experimental
single-camera full-body capture. Its rig library license and its character/tool
asset licenses have different scopes. No MetaHuman character or capture plugin was
installed by this rebuild.

## Remaining art work

The native close views still show smooth skin and limited facial nuance. Full-body
capture, facial correctives, skin microdetail, physically simulated clothing and
fantasy-specific art direction are the next material improvements. The rebuild
does not establish AAA parity or a complete replacement of the original Morrowind
world. Its exact implemented scope is the three native HALVETH guide characters.
