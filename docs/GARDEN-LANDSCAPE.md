# Native worlds and character runtime — 4 October 2026

Four continuous 600 × 600 metre landscapes replace the small circular block
platform in the actual Unreal 5.8 runtime. Each has 66,049 authored vertices,
131,072 triangles and full triangle collision fallback. Arrival clearings preserve
book, crafting, resource and portal interactions. Tidelight includes water.
CC0 tree, fern and rock scans provide the natural scenery.

Three dressed human guide meshes now replace the earlier mannequin placeholders.
Their skeletal animation, Blink/Talk morphs and time-dependent gaze/dialogue
response are integrated. See `CHARACTERS.md`. Some book, resource and construction
props retain simple development geometry.

## Crystal function

The crystal is a spell practice target. Travelling SPARK collision deals 25 damage.
Four hits reduce 100 health to zero; the main meshes and query collision hide and
six fragments become visible. After five simulation seconds, health, visibility
and collision return. Native gameplay and rendered capture both passed this cycle.

## Checks and coverage

- Unreal 5.8 C++ runtime/editor modules compiled successfully.
- The 31-step native sequence passed portal roundtrips, gathering, spells,
  terrain/collision, quality, items, guide dialogue, library, crafting,
  enchantments, council, construction, realm persistence and save/restore.
- Terrain checks used 441 rays per visited instance and a 55 cm interpolation tolerance.
- Three character checks confirmed shared mesh/animation skeleton, 164 imported
  bones including root, a twelve-second idle, Blink/Talk morphs and actual sampled
  internal bone pose changes.
- Twelve 1920 × 1080 DX12 captures were reviewed: four worlds, three crystal
  states, landscape, three guides and a face closeup. No engine error was observed.
  This is finite image/time coverage, not an exhaustive gameplay or art approval.
- Automated capture owns its camera/input only in its explicit mode. Normal play
  retains ordinary movement, physics and the existing save key.

Source/result binding is in `../QA/native-landscape.json`. Authoring/import bind-pose
warnings remain diagnostic information; the runtime animation checks passed.
Fine skin/hair/cloth art, more props, profiling, multiplayer and complete Morrowind
world replacement remain open work branches.

## Run

```bash
bash GARDEN.sh play
bash GARDEN.sh check
```

The Git Bash launcher uses the locally licensed UE 5.8 installation and its C++
prerequisites. Character sources are included; nature inputs download on a clean
first preparation. Generated `.uasset` files and engine/template files stay local.
