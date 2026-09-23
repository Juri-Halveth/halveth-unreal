# Engine baseline — 23 September 2026

The `.uproject` binds Unreal Engine 5.8. Local inspection found **5.8.0, changelist 55116800**. The public version research found the official **5.8.2 hotfix announcement dated 25 August 2026**. These are separate facts; this project does not silently upgrade the local engine or claim that a 5.8.0 test ran on 5.8.2.

The current runtime uses stable engine APIs for characters, static meshes, lights, materials, native HUD, game user settings and local world generation. It does not require the experimental 5.8 mesh-terrain or vegetation tools. Those remain candidates for later, isolated content work after the current game loop is verified.

## Primary sources

- [Epic: Unreal Engine 5.8 is now available](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available), published 23 June 2026; read through Exa on 23 September 2026. Establishes the stable 5.8 release and distinguishes experimental worldbuilding features from released core tools.
- [Epic: 5.8.2 Hotfix Released](https://forums.unrealengine.com/t/5-8-2-hotfix-released/2746335), published 25 August 2026; fetched through Exa on 23 September 2026. Establishes the subsequent hotfix release.
- [Epic forum: UE5.8 / Visual Studio toolchain compatibility](https://forums.unrealengine.com/t/ue-5-8-fails-to-compile-with-visual-studio-2026-hash-map-missing-source-fix/2732357). The official reply identifies MSVC 14.44 and 14.50 as supported baseline choices and the removed `hash_map` issue in 14.51. This is a toolchain consideration; an observed successful project build takes precedence over assuming that every project fails with 14.51.
- [Microsoft: Open Unreal Engine projects in Visual Studio](https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-uproject) describes native `.uproject` support and toolchain prerequisites.

Research coverage: four Exa searches requesting five results each (20 result slots, not 20 independently verified facts); direct fetches of both release sources succeeded. A targeted follow-up for 5.8.3/5.8.4 returned 5.8.2 and earlier Epic announcements; it found no later release within that search. Two API-reference fetches did not return content. Engine header reads and compilation are used to resolve local API compatibility rather than treating inaccessible reference pages as evidence.

No advertised IDE discount, search-result snippet, sponsored link or documentation-version dropdown is used as proof of an installed engine version.
