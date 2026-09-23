# Portal Garden 0.2.0 — native validation

Recorded on 2026-09-23 for the Windows Development prototype. The earlier
`NATIVE-VALIDATION.md`, `RELEASE-VALIDATION.json` and 0.1 release artifacts remain
historical records. This version adds the living library and persistent crafting
loop to the existing four-realm game.

## Executed game

Unreal Engine **5.8.0, changelist 55116800**, compiled the Editor target and the
Win64 runtime. The final BuildCookRun completed with exit code 0. Packaging
verified and copied 1,299 engine legal-document sources against both the prepared
notice manifest and the installed engine files. These notices are a broad legal
companion, not an assertion that every listed library is a runtime dependency.

The packaged game executable ran `-HalvethSmokeTest -NullRHI -unattended -nosound`
and exited **0**, producing a fresh passing receipt. `RunSmokeStep()` contains
31 phases (indices 0–30); the runtime receipt identifies its scope by name and
does not contain a numeric phase counter. The sequence exercises:

- Three portal roundtrips, world collision, fall recovery, LOVE and graphics settings.
- Recovery of all three project quality profiles from the actual Unreal settings,
  including the project's custom 67% and 85% resolution scales.
- Inventory consumption, AEGIS mitigation, authored guide interaction, dodge movement,
  four projectile hits and training-crystal regeneration.
- Repeated native resource gathering in all four realms; actual material counts rise.
- Physical book lookup without a resource marker taking over its interaction.
- Incorrect and correct recall, a one-time page reward, all three crafting recipes,
  and the explicit transfer of a crafted healing draught into the usable inventory.
- Both enchantments: maximum mana becomes 110 and the selected SPARK cost becomes 14.25.
- Three sequential council choices, including rejection of a second choice for a
  completed decision.
- A real colliding workbench actor, rejection of overlapping placement, realm-specific
  reconstruction, unchanged closed-book reading time, and save/reload of knowledge,
  inventory, enchantments, council history and construction positions.

Smoke progress uses its own `Saved/Tests` slot and does not load the player's slot.
Normal progress lives outside versioned installations under
`%LOCALAPPDATA%/HALVETH/PortalGarden/SaveGames/knowledge-SEED.json`.

## Rendered interface

The packaged DX12/SM6 executable rendered all four library panels at a requested
1280 × 720 viewport. Images were obtained using Unreal's own `Shot showui` command
and visually inspected: book text, answers, material counts, costs and council
alternatives fit inside their panels. The compiler-generated interface is the
running game HUD, not a browser page or a promotional mockup.

- [Crafting](screenshots/library-craft-0.2.0.png)
- [Construction after cold start](screenshots/knowledge-coldstart-0.2.0.png)
- [Council](screenshots/library-council-native-0.2.0.png)
- [Inventory](screenshots/library-bag-0.2.0.png)

The final executable also rendered the [restored Performance profile](screenshots/quality-restored-0.2.0.png)
from an isolated prepared settings file, without a quality command-line override.
With that same settings fixture, an explicit quality override produced the
[Epic profile](screenshots/quality-override-0.2.0.png). Both HUD labels were visually
checked. This checks startup consumption of saved settings; it does not substitute
for a physical-key test of choosing and saving the profile.

The separate [Editor window capture](screenshots/library-council-0.2.0.png) and
`library-build-0.2.0.png` / `portal-garden-0.2.0.png` record the earlier 0.2
candidate before the final profile-restoration and potion-name fixes. Windows
automation could not reliably activate
the game window; physical keyboard/mouse interaction in this pass is **not
verified**. Runtime methods were exercised by the in-game smoke sequence and
input bindings were checked in source. Screenshots are finite visual samples,
not a performance, animation, audio or all-resolution test.

## Cold-start progression

The final packaged executable was started in six separate DX12 processes with a
reserved QA seed and an own fixture derived from the smoke test. Each process
exited 0 using the engine's timed-exit option. The four library views and two
graphics-profile views were inspected. The book shows a learned page and the
construction panel shows the restored workbench (`1/32 placed`).

Inventory, XP, crafted counts, enchantment ranks, construction counts and positions,
council choices, packed consumables and recipe discoveries remained equal to the
fixture across those starts. The own QA slot was then removed. This is a
source-defined example-state test, not an import or migration of a player's
Morrowind save. Reading time may legitimately advance while a focused page is open.

## Domain and source tests

The production C++20 knowledge header passed **18,008 local MSVC checks** with
`/W4 /WX`. For every seed from 0 through 255, renewable nodes cover all seven raw
materials and the tests reach all recipes, both enchantments at rank 3, all three
construction kinds and the council sequence without `GrantItem` test gifts.
The suite also checks state restoration, invalid inputs, inventory boundaries,
once-only progress and 1,500 mixed transitions. These counts describe assertions
within the declared model, not exhaustive coverage of a whole game.

The Python suite passes its ten runnable Windows tests; the filesystem-link test
is skipped where that test process cannot create a link. CI independently runs
the source suite and production C++ tests on Windows/MSVC and Linux/GCC. CI does
not build Unreal or exercise a GPU.

## Installation record and limits

The versioned installation, preservation test and final artifact hashes are
recorded separately in `RELEASE-VALIDATION-0.2.0.json`. The installer and portable
package carry the exact staged file manifest; player saves and logs are excluded.

The final candidate was installed over the earlier unpublished 0.2 candidate,
then uninstalled and reinstalled. The installed native smoke test exited 0 with a
fresh passing receipt. Every one of the **1,350 payload files** matched its
manifest after installation and reinstallation. The three versioned shortcuts and
uninstall registry were checked. Two own save-retention fixtures survived both
operations with unchanged hashes and were removed afterwards. The five recorded
0.1 executable/shortcut files and its uninstall registration remained unchanged.
This is a bounded preservation test, not a claim that every file on the host was
inventoried. Version 0.2 remains installed after the test.

This remains an unsigned Development prototype tested on one existing Windows
machine. Clean-machine prerequisite setup, multiplayer, live AI conversations,
voice, detailed character animation and a full campaign are outside this release's
verified scope. There is no new frame-rate claim for 0.2. The 0.1 frame sample is
retained only as its own historical measurement.
