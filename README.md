# HALVETH Portal Garden — native worlds and responsive characters

Four locally generated 600 × 600 metre landscapes with hills, CC0 forest scans,
portal arches, spell practice, library, crafting and persistent construction.
Scarlet, Lucinet and Rachel now use dressed, skinned human meshes with individual
face targets, authored hair and eyes, a twelve-second breathing/idle animation,
blinking, nearby-player gaze and dialogue-triggered mouth motion.

![Scarlet in the native Unreal game](Preview/GARDEN_SCARLET_CHARACTER.png)

This is an independent HALVETH game in Unreal Engine 5.8. The OpenMW adapter
changes original Morrowind objects through its own loaded model paths. Its
private retail-derived clothing files are not part of this public repository.

## Verified current development build

The C++ build and the 31-step native gameplay sequence passed. Three character
checks verified skeleton, animation, Blink/Talk morphs and actual sampled internal
bone pose changes. A separate DX12 run produced twelve reviewed 1920 × 1080 views.
The crystal took four SPARK hits, broke and reformed after five simulation seconds.

See [the runtime receipt](QA/native-landscape.json),
[character construction](docs/CHARACTERS.md) and
[world and crystal checks](docs/GARDEN-LANDSCAPE.md).
Hair shape, detailed facial sculpting, garment folds and several props still need
art work. The screenshots show the current game; they are not concept renders.

## Git Bash launch

Use a locally installed/licensed Unreal Engine 5.8 with its Windows C++
prerequisites, Git Bash and Python:

```bash
bash GARDEN.sh play
```

The launcher downloads declared CC0 nature inputs through `experiments/void-native`,
prepares authored geometry and character assets and compiles changed source.
Later starts reuse bound local assets. `GARDEN_ENGINE` selects the engine;
`GARDEN_PYTHON` selects Python; `GARDEN_VOID_ROOT` selects a local VOID source tree.
`bash GARDEN.sh check` runs native gameplay checks; `bash GARDEN.sh visual` runs
the automated rendered capture. Bash orchestrates the native C++ game and art tools.

## Rebuild character art

The selected CC0 inputs, FBX sources, textures and MIT builders are included.
Normal game startup uses the supplied FBX files and needs no Blender installation.
To regenerate the art, install Blender 4.5, NumPy and Pillow, then:

```bash
export GARDEN_BLENDER="/c/path/to/blender.exe"
export GARDEN_PYTHON=python
bash CHARACTERS.sh build
bash GARDEN.sh prepare
```

The head deformation also transfers to the authored hair and beard. FBX path
sanitizing replaces local author paths with relative filenames and verifies the
parsed numeric geometry, skeleton and animation digest remains identical.

## Playable systems

Books, fifteen pages, recall, seven raw materials, three recipes, two enchantments
with three ranks, consumables, LOVE/SPARK/AEGIS, guide dialogue, council choices
and three construction types remain integrated. Progress is saved locally under
the existing world key. Automated tests/capture use an isolated test slot.

| Input | Action |
|---|---|
| WASD / mouse | Walk / look |
| Space / left Shift | Jump / run |
| E near a marker, book, guide or portal | Gather, read, speak or travel |
| Q / left mouse button | Select / cast LOVE, SPARK or AEGIS |
| L | Select and cast LOVE |
| I / F | Select / use an inventory item |
| Left Ctrl | Dodge, using stamina |
| B | Open the library and workshop |
| T / G | Select a structure / place it on visible ground ahead |
| R | Return home and recover position |
| 1 / 2 / 3 | Performance / Balanced / Epic graphics |
| H | Show or hide controls |
| F10 | Open or close credits and license notices |
| Escape | Quit the game |

| In the library | Action |
|---|---|
| Tab | Next text |
| Page Up / Page Down | Previous / next page |
| 1 / 2 / 3 | Answer the current page's recall prompt |
| O | Cycle the right-hand information panel |
| C / V | Select a recipe or enchantment / craft or enchant |
| P | Pack one crafted Lumen Draught into the healing-item pocket |
| T | Select the next structure |
| N | Select the next council decision |
| Z / X / Y | Choose its first / second / third alternative |
| F5 / F9 | Save progress / reload saved progress for this seed |
| B / Escape | Close the library |

To build, select a structure with **T**, close the library with **B**, face clear ground and press **G**. To use a crafted healing draught, pack it with **P**, close the library, select the healing item with **I** and use **F**; it restores up to 40 health. An item is retained when its resource is already full.

## Licenses and source

Original HALVETH C++/Bash/Python source is MIT. MakeHuman core basemesh, rig,
weights and target assets are CC0. The selected skins use the expressly CC0 pack
members recorded in [SOURCES.json](ArtSource/Characters/SOURCES.json). Original
hair, garments, eyes and fabric assets in that directory are dedicated to CC0.
See [MakeHuman's asset license](https://static.makehumancommunity.org/about/license.html)
and [the character license](ArtSource/Characters/LICENSE.txt).

Nature scans include [Tree Small 02](https://polyhaven.com/a/tree_small_02),
[Fern 02](https://polyhaven.com/a/fern_02) and
[Rock Moss Set 01](https://polyhaven.com/a/rock_moss_set_01), with URLs and hashes
in the VOID asset pipeline. Blender and Unreal remain separately licensed tools.
Engine binaries, generated Content, caches, saves and raw logs stay outside Git.
Current characters have no dependency on the earlier Epic mannequin placeholders.

The historical releases retain their own records. This branch is a development
source delivery; it does not claim a new standalone installer or complete AAA remake.
See [engine notices](THIRD-PARTY-NOTICES.md) and
[existing adventure mechanics](docs/ADVENTURE.md).

<!-- HALVETH_WORK_CERTIFICATES_V1_1 -->
## Juri Janovski / Juri Halveth – Privates HALVETH-Werkzertifikat

[Privates HALVETH-Werkzertifikat: Dokumentierte portable C++-Quellprüfungen – HALVETH Portal Garden](https://juri-halveth.github.io/werkzertifikate/#werk-halveth-unreal).

HALVETH VERACHEL STUDIOS · Quellstand, dokumentierte Ergebnisse und SHA-256-Belege stehen im Werkzertifikat. Private, mit Codex erstellte Werkdokumentation; keine ISTQB- oder sonstige Personenzertifizierung.
<!-- /HALVETH_WORK_CERTIFICATES_V1_1 -->
