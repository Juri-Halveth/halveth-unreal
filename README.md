<!-- HUB_LANGUAGES_V1 -->
[Original / Deutsch](README.md) · [English](README.en.md) · [Русский](README.ru.md)
<!-- /HUB_LANGUAGES_V1 -->

# HALVETH Portal Garden — begehbare Welt, freie Levitation und Wissen

Ein eigenständiger C++-Spielprototyp für Unreal Engine. Portalwelten, eine interaktive Bibliothek und Bau- und Rezeptsysteme verbinden Gestaltung mit nativer Spielentwicklung.

[**Projekt öffnen →**](https://github.com/Juri-Halveth/halveth-unreal/releases) · [Arbeitsproben](https://juri-halveth.github.io/koennen/) · [Portfolio](https://juri-halveth.github.io/)


## Körperkontinuität und elastischer Träger

Der neue [Körperträger](docs/BODY-CONTINUITY.md) erhält sichtbare Bewohner,
statt sie durch eine neue Rangliste auszutauschen. Ein kleiner räumlicher
Gewebesolver übergibt nachgiebige Restbewegungen an die GPU-Oberfläche.
Das ist ein nativer Entwicklungskandidat mit vorläufig wiederverwendeten
Charakterformen; individuelle Anatomie und biologische Zellen sind noch offen.

## Entspanntere Figuren und verlässlicher Bodenkontakt

Die [aktuelle Bewohnerbewegung](docs/RESIDENT-MOTION.md) ergänzt entspanntere
Ellenbogen, eine kurze Nachbewegung der Hände und durchgängige Schrittphasen.
Menschen dienen anderen Figuren nicht mehr als Treppen oder Boden.
[Native Gehaufnahme ansehen](Preview/RESIDENT_MOTION.gif).

## Wiese, Wind und ein tatsächlich aktueller Start

![Native Spielaufnahme: Wiesenhalme und Blüten bewegen sich vor der Archivtreppe](Preview/MEADOW_WIND.gif)

Das native Windows-Spiel verbindet vier Landschaften, ein begehbares Archiv,
freie Levitation und lokale Bewohner. Die neue Wiese besteht aus gekrümmten
Geometriehalmen mit festen Wurzeln, verschiedenen Windphasen und kleinen
Blüten. Ein begrenztes Fenster mit höchstens 3.844 Instanzen folgt den
besuchten Weltzellen; zurückkehrende Zellen erhalten dieselbe Platzierung.

**Start:** `Start-Astralis.cmd` · **Fliegen:** J · **Bewegen:** WASD ·
**Interagieren:** E · **Bibliothek:** B.

Die [Wiesenwerkzeuge und der aktuelle Prüfstand](docs/MEADOW-AND-START.md)
trennen native Aufnahmen, Bodenmessung, Funktionstests und noch offene
Darstellungsarbeit. Das ist ein spielbarer Entwicklungsstand mit begrenzter
Welt und wiederverwendeten Baum-/Charaktermodellen.

![Dichte Halme, Blüten und die Bodentextur im nativen Paket](Preview/MEADOW_ROOTS.png)

## Astralis · räumlicher Baukasten und begehbares Archiv

Der neue [Astralbereich und seine Werkzeuge](docs/ASTRAL-TOOLKIT.md) ergänzen das
native Startgebiet um eine massive Steintreppe, eine Pforte mit drei
ausrichtbaren Ringen, eine obere Archivterrasse, ein lesbares Buch sowie
frei verfügbare Levitation mit einer Spur aus der tatsächlichen Flugbahn.
Durch die geöffnete Pforte erscheint eine zusätzliche, an die Spielerkamera
gebundene Live-Ansicht des Archivs.
`Start-Astralis.cmd` baut bei Bedarf das lokale Windows-Paket und startet es.
Eigene periodische Mineralkarten erzeugen Materialrelief; die 16 privaten
Gestaltungsreferenzen werden als getrennte Beobachtungen und technische
Rezepte erschlossen. Die [Datei- und Versionskarte](LICENSES.md) trennt den
bestehenden MIT-Stand von den neuen, eng zugeordneten PIRL-Meadow-Beiträgen.

![Astralis: geöffnete Pforte mit Live-Ansicht im nativen Spiel](Preview/ASTRAL_OPEN.png)

Der [datierte lokale Prüfstand](QA/native-astral.json) trennt den nativen
Funktionstest, die gerenderten Ansichten und die begrenzte Framezeitmessung.

Die anschließende [Kamera- und Nachtkorrektur](QA/native-free-look-sky.json)
entfernt den vertikalen Blickanschlag und ersetzt das regelmäßige Sternmuster
durch 1.800 unterschiedlich helle, weich auslaufende Lichtpunkte in einem Mesh.
Sonne und Nacht-Hilfslicht besitzen getrennte Atmosphärenkanäle; das Hilfslicht
erzeugt keine sichtbare Himmelskörperscheibe. Nachtlampen und Belichtung sind
aufeinander abgestimmt. `Start-Astralis.cmd` verwendet das neue lokale Paket,
während das vorherige Paket getrennt erhalten bleibt.

![Nachtansicht aus dem korrigierten nativen Windows-Paket](Preview/NIGHT_GATE.png)

Der nächste [Bewohner- und Flugschritt](docs/LIVING-WORLD.md) verbindet freie
Levitation in Außen- und Innenräumen mit lokalen Bewohnern, verschiedenen
Antworten und einer Verwarnung durch sichtfähige Wachen. Ein Register mit
4.096 Zuständen wird getrennt von höchstens 18 körperlich dargestellten
Bewohnern verwaltet. Das ist ein begrenzter, lokal berechneter Spielablauf;
eine vollständig bevölkerte Welt oder eine mit Morrowind verglichene
fünffache Bevölkerung ist damit noch nicht erreicht.

![Bewohner und kurzer Fangimpuls im nativen Funktionstest](Preview/LIVING_WARNING_HOLD.png)

---

Four locally generated 600 × 600 metre landscapes with hills, CC0 forest scans,
portal arches, spell practice, library, crafting and persistent construction.
Scarlet, Lucinet and Rachel now use dressed, skinned human meshes with individual
face targets, authored hair and eyes, a twelve-second breathing/idle animation,
blinking, nearby-player gaze and dialogue-triggered mouth motion. They now walk
with capsule collision and gravity, place their feet on traced ground, transfer
weight, move their hands/fingers and respond with hair/garment deformation.
Ground patches deform under a declared mass/support model and update collision.
A dated NASA/JPL Sun trajectory drives the 24-hour sky, with volumetric clouds,
decorative stars and authored night lighting.

![Scarlet in the native Unreal game](Preview/GARDEN_SCARLET_CHARACTER.png)

This is an independent HALVETH game in Unreal Engine 5.8. The OpenMW adapter
changes original Morrowind objects through its own loaded model paths. Its
private retail-derived clothing files are not part of this public repository.

## Verified current development build

The 2026-10-04 world-contact increment gives every placed tree a simple colliding
trunk. The following body/development increment replaces the fixed 6 × 5.2 metre
patrol with individual terrain-supported goal selection, exploration, book and
portal visits, and recovery. Ground probes and capsule sweeps steer the next step;
translational velocity drives gait,
avoiding invented strides from turning in place. Capture cameras and
dialogue tests follow the moving guide. See [world-contact scope and measurements](docs/WORLD-SCALE.md).

The current body increment derives pelvis support from anatomical leg lengths,
removes the permanent crouch and independent eye oscillation, rebuilds face
targets, and connects reserve, breathing and response to actual movement.
A growing virtual genome records development events and adapts future behavior.
See [body and development](docs/BODY-AND-DEVELOPMENT.md) and its current receipt.

The C++ build and the 31-step native gameplay sequence passed. Three character
checks verified the skeleton, eight morph targets and sampled internal head/hand
motion. Native motion capture additionally tested a 1.5 metre fall, walkable
ground and deforming collision. An accelerated 24-hour sky sequence observed day
and night using 25 dated NASA/JPL samples. The explicit current shadow path uses
four cascades after a GPU page fault in the initial accelerated VSM/Nanite run.
A separate DX12 run produced twelve reviewed 1920 × 1080 views.
The crystal took four SPARK hits, broke and reformed after five simulation seconds.

The character rebuild replaces generated tube clothing and hair with authored
CC0 meshes fitted to anatomy, preserved UVs, garment normal maps, masked hair and
separate modeled footwear. Runtime stance targets narrow the rigging A-pose and
relax the hands beside the body. The regeneration entry point also refreshes its
public byte inventory after FBX sanitizing.

See [the current body and development receipt](QA/native-body-development.json),
[the previous character rebuild receipt](QA/native-character-rebuild.json),
[the historical landscape receipt](QA/native-landscape.json),
[motion, gravity and sky](docs/MOTION-EARTH-SKY.md),
[character construction](docs/CHARACTERS.md) and
[world and crystal checks](docs/GARDEN-LANDSCAPE.md).
Skin microdetail, facial corrective shapes, richer captured gait, cloth simulation,
fantasy-specific wardrobes and several props still need art work. The screenshots
show the current game; they are not concept renders.

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
the automated rendered capture. `bash GARDEN.sh motion` runs a short motion/gravity
capture with sparse 2 Hz stills. Bash orchestrates the native C++ game and art tools.
`bash GARDEN.sh sky` refreshes the Sun data. The observer is a declared virtual
site, 45° N and 0° E. Cached later days repeat the explicitly dated path; clouds,
stars and night fill are authored, rather than a live weather or Moon service.

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

The anatomical deformation also transfers to authored clothing and hair. FBX path
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

Existing HALVETH C++/Bash/Python source retains MIT. Six new Meadow source/test
files assign only their original new contributions to PIRL 2.0; see
[LICENSES.md](LICENSES.md) for the exact scope and earlier-grant exceptions.
Commercial permission for those assigned contributions requires the separate
written agreement described there. MakeHuman core basemesh, rig,
weights and target assets are CC0. The selected skins use the expressly CC0 pack
members recorded in [SOURCES.json](ArtSource/Characters/SOURCES.json). Authored CC0 clothing, footwear and hair retain their license headers and hashes in [wardrobe-sources.json](ArtSource/Characters/wardrobe-sources.json). Own eyes, fabric and animation additions are dedicated to CC0.
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
