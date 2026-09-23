# Adventure controls and current rules

The adventure component is local game state. Its three guide profiles are authored dialogue, not live AI personas. There is no account, network call, wallet or paid API behind a conversation.

| Action | Input | Current rule |
|---|---|---|
| Select / cast a spell | Q / left mouse | Cycles LOVE, Spark, Aegis; casting requires mana and a short cooldown |
| LOVE | L, or select LOVE and click | Restores up to 35 health and produces four seconds of light; costs 20 mana |
| Spark | Select with Q, then click | A travelling projectile with collision; delivers 25 impact damage; costs 15 mana |
| Aegis Shield | Select with Q, then click | Reduces incoming damage by 75% for six seconds; costs 30 mana |
| Dodge | Left Ctrl | Uses 28 stamina, requires ground contact and a 0.9-second cooldown; movement follows input direction; no invulnerability |
| Select / use an item | I / F | Cycles Roseleaf, Moonwater and Sunfruit; an item is retained if its resource is already full |
| Speak / enter a portal | E | Uses nearby guide dialogue first, otherwise a nearby portal |

Mana regenerates at eight points per second and stamina at twenty points per second, each up to 100. The pack starts with three Roseleaf (+40 health), two Moonwater (+45 mana) and two Sunfruit (+45 stamina). Current health, mana, stamina, selected spell and inventory appear in the native HUD. These values are scoped to the running session; persistence is not implemented.

The practice crystal accepts Spark damage, breaks after four full hits from its initial 100 health, then reforms. Ordinary static scenery is not destructible. The prototype currently has no complete death/respawn combat loop; LOVE or Roseleaf can restore health after it reaches zero.

Scarlet introduces the garden and LOVE; Lucinet explains portals and Spark; Rachel explains the three inventory items. Their original lines advance locally and remain clearly marked as authored dialogue.

## Integration API

`UHALVETHAdventureComponent` exposes Blueprint-callable methods:

- `CycleAbility`, `ActivateAbility`, `CycleItem`, `UseItem`, `Dodge`.
- `InteractWithNearbyCharacter`, `ReceiveDamage`, `ClearTransientEffects`.
- Read-only getters for health, mana, stamina, shield state, selected spell/item, item counts, Spark impacts and HUD text.

The character owns the component. The game mode and generated world own portal transitions and guide placement. `AHALVETHTrainingTarget` receives the actual damage event and handles breakage/regeneration. A separate future expansion can add save serialization or network replication around these explicit boundaries.

## Test scope

The runtime smoke sequence exercises item consumption, no waste at full health, Shield reduction, LOVE healing, guide interaction, dodge displacement, four projectile impacts and crystal regeneration. Those assertions are implemented; their latest run result is recorded by the release workflow separately. Compilation alone does not prove runtime behavior or rendered appearance.
