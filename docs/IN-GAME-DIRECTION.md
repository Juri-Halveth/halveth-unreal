# One game, one playable entry

Design direction recorded on 2026-09-23 from the user's request. New gameplay
belongs inside the running native game. An external dashboard or web page is not
a substitute for a playable feature.

The 0.2 entry is the installed **HALVETH Portal Garden 0.2.0** shortcut, or
`HALVETHRealms.exe` in the portable game folder. The normal Unreal runtime needs
its associated engine and content files. Those files are distributed together;
the player does not start a development shell, local server or browser.

| Activity | Current native route |
| --- | --- |
| Explore and use portals | Move with WASD, interact with E |
| Talk to a guide | E near an authored guide |
| Read books, scrolls and notes | E near a lectern, or B for the library |
| Learn and recall | Page Up/Down, then 1/2/3 inside the book |
| Craft and enchant | Library workshop; C selects, V creates |
| Manage materials | Library inventory panel, O cycles panels |
| Pack and use a potion | P in the library, then I/F in the world |
| Build | T selects, close the library, G places on clear visible ground |
| Make council choices | Council panel; N selects, Z/X/Y chooses |
| Cast and defend | Q selects, left mouse casts, L quick-casts LOVE |
| Save and reload progression | F5/F9 inside or outside the library |
| Choose graphics and read controls | 1/2/3 outside the library, H for help |
| Read credits | F10, inside the game |

## Registered next branches

These are future implementation requirements, not implemented capabilities:

- A unified in-game journal should explain objectives, relevant books, costs and
  consequences, with readable shortcuts into the existing native panels.
- A connected archive may present source-attributed external knowledge as
  in-world writings. It must distinguish authored fiction, retrieved information
  and character interpretation. The offline game remains playable.
- Connected conversations should appear through the same native interaction
  surface, with local character history and explicit player-directed actions.
- New worlds and collaborators' content need a versioned content format and
  real import/load tests before they can join the running world.
- Original Morrowind/OpenMW integration is a separate engine adapter. The
  current Unreal executable does not alter Morrowind.exe or load Morrowind saves.
  A shared name, icon or launch button would not implement that integration.

Earlier versions, Genesis and Realms remain separate preserved installations.
The single-entry direction concerns the experience of playing this game; it does
not authorize silently deleting those existing projects, saves or shortcuts.
