# Character motion, ground response and Earth sky

The native Unreal guides now move through `ACharacter` and
`UCharacterMovementComponent`. Their capsules participate in floor collision,
walking and falling, including operation without an AI controller. Skeletal
feet are placed through a two-bone solve, stance targets stay in world space,
and the feet align to the traced ground normal. Head, eyes, hands, fingers,
breathing, dialogue and bounded hair/garment morphs have independent responses.
The garments use authored deformation targets; they are not a Chaos cloth model.

## Mass, acceleration and support

The scene uses centimetres and seconds, with downward gravity set to
`980.665 cm/s²`. The authored guide masses are Scarlet 68 kg, Lucinet 82 kg and
Rachel 61 kg. The support **model** is

```text
N = max(0, m × (9.80665 + upwardAcceleration))  when supported
N = 0                                       when unsupported
```

Here `N` is in newtons, `m` in kilograms and acceleration in metres per second
squared. Constant upward speed has zero upward acceleration. The acceleration
term has portable mathematical tests; ordinary gameplay currently supplies zero
platform acceleration. The game does not claim that empty space creates force.

Near the guides, aligned terrain cells are replaced by three real deformable
ground patches in the hub, and one patch in each other realm. Each patch spans
4.6875 × 4.6875 metres with 4,225 vertices. Gaussian foot loads use an authored
stiffness of 450 N/cm and bounded depth; normals and triangle collision update
with the mesh. This is quasi-static soil compliance, not a measured soil law or
an impulse measurement. Foot-target locking is not proof of zero visible foot
drift. Millimetre-scale numerical tests of an IK model do not establish that
accuracy for every rendered foot, collider or terrain point.

The native capture test lifts Scarlet 1.5 metres, observes falling velocity and
then checks return to walkable ground. Results and their source hashes are in
`../QA/native-motion-source.json` and `../QA/native-earth-sky.json`.

## Dated NASA/JPL sunlight

`bash GARDEN.sh sky` retrieves 25 hourly Sun azimuth/elevation samples from the
[NASA/JPL Horizons API](https://ssd-api.jpl.nasa.gov/doc/horizons.html).
`play` refreshes that data once and continues with its dated cache if the
request fails. The declared observer is a **virtual** site at 45° N, 0° E and
zero altitude; it is not a player's detected location. The raw public response,
request, retrieval time and hash are retained under `assets/sky`.

The renderer interpolates Sun direction between samples and normally advances
with UTC time. The cache covers its declared 24-hour interval. Later offline
days repeat that dated path; they are not represented as newly observed data.
Volumetric clouds, 240 decorative stars and a weak night fill are authored game
features. Their presence is not a live weather feed, star catalogue or NASA Moon
model. The accelerated capture uses a factor of 4,320: 24 simulated sky hours in
20 seconds. It changes the astronomical clock, not character movement speed.
Bounded histogram exposure adapts through sunset instead of abruptly forcing
night gain while indirect-light and sky captures are still updating.

## Explicit renderer repair

An initial accelerated DX12 run failed with `DXGI_ERROR_DEVICE_HUNG` and a GPU
page fault. The active graphics breadcrumb was the Nanite Virtual Shadow Maps
shadow pass. Unreal reported about 2,484 MB of local memory use against an
11,229 MB budget. Those observations do not establish an out-of-memory cause
or uniquely identify the engine/driver defect.

The current project explicitly selects cascaded shadow maps with four sun
cascades. Every quality mode keeps that path; Epic quality cannot silently
restore the failing VSM path. Lumen, Nanite scene geometry, atmosphere and clouds
remain enabled. Subsequent complete day/night and rendered gameplay runs are
recorded in the current receipt. Crash records stay local. This is a tested
rendering workaround, not a claim to have repaired the installed GPU driver.

## Original OpenMW adapter

`../Source/OpenMW/veyra-live-motion.omwscripts` and its MIT Lua script request
available native idle variants for NPC upper bodies. Gesture priority remains
below locomotion and combat. The finite local test followed original Heidmir in
Ebonheart, observed three native groups, and retained NPC records, model bytes
and the copied save. Retail models and saves are not distributed in this repo.

## Relevant production techniques

- [Fortnite's character motion matching](https://www2.unrealengine.com/blog/unreal-engine-5-4-is-now-available?lang=en-US)
  illustrates choosing appropriate recorded movement for changing contexts.
- [Guerrilla's Jolt integration in Horizon Forbidden West](https://www.guerrilla-games.com/read/architecting-jolt-physics-for-horizon-forbidden-west)
  describes collision/physics simulation and its performance work.
- [UE CharacterMovement](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent)
  is the actual movement implementation used here.
- [NASA's weight equation](https://www1.grc.nasa.gov/beginners-guide-to-aeronautics/weight-equation-2/)
  provides the mass/weight distinction used by the local support model.

The current guides have responsive motion and real terrain contact. Full
anatomical animation, fine skin/hair/cloth art, ragdolls, profiling over long
sessions, multiplayer and all original Morrowind NPC/world regions remain open.
These are finite native tests and captures, not a complete AAA remake or a
universal physics/visual accuracy guarantee.
