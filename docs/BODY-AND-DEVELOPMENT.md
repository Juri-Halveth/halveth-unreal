# Body support and developing characters

The active native game uses a skinned anatomical human mesh, traced feet,
capsule movement, independently solved arms and legs, and a gameplay body model.
The previous permanent 2.8 cm pelvis drop is replaced by support height derived
from the actual two leg lengths, horizontal reach and current foot targets.
Standing requests a six-degree knee bend. Walking permits more flexion and the
pelvis responds to both feet. Gaze follows a smoothed target instead of unrelated
oscillation of the eyes. Face rebuilding removes the strong authored age targets
and reduces the hollow-cheek and prominent-chin contributions.

## Movement and response

Guides choose terrain-supported destinations, explore, visit books or portals,
rest on arrival, and choose another goal when a nearby step is obstructed.
Capsule sweeps and ground probes check the next step. This is local steering,
with further route planning still an open extension. Ordinary player proximity
permits movement; a conversation pauses it. Explicit automated portrait captures
hold a nearby subject so its resting posture can be inspected.

The effective mass budget closes across skeleton, muscles, organs, fat, skin and
blood. Total mass is used by the actual CharacterMovement and ground support
model. Exertion spends muscle reserve; resting restores it. Reserve changes
movement speed and acceleration, while sufficient fatigue requests recovery.
Heart and respiratory phases influence the visible breathing morph. Response
time varies with the character profile and fatigue. Oxygen supply and tendon
strain are simplified modeled state variables; tendon strain currently supplies
an audit value rather than a separate physical elastic tissue solve.

These are authored gameplay rules and effective compartments. Individual organ
geometry, cellular neural simulation, deformable internal muscles and tendons,
skin microdetail and a richer captured locomotion library remain separate art
and simulation work. The current facial mesh and cloth are real runtime assets.

## Growing virtual genome

The genome uses a reversible two-bit `A/C/G/T` alphabet for SHA-256 bytes. Each
development event appends another 128 symbols. The sequence is a virtual game
representation, with an explicit gameplay trait mapping.

Birth binds an origin digest, entity address, UTC event time and simulation
time. Its digest supplies variation in mass, pace, curiosity and response time.
Later exploration, visits, rest and recovery append a new record and allow
curiosity and pace to change with the body's accumulated experience. The same
entity keeps its record chain, profile and current body state across realm
transitions during that game session. Each new launch begins a fresh lineage;
its previous local journals remain available for review.

Every journal line is UTF-8 JSON with a parent digest. The next record references
the SHA-256 of the exact previous line, excluding its line terminator. Each
record stores the source digest and its corresponding new symbol segment.
Journal append precedes updating the committed in-memory lineage. The local
journals live in `Saved/BodyLineage` and are excluded from public source exports.
The engine's bundled OpenSSL supplies SHA-256. The engine's generic platform
SHA-256 method was found to abort on this Windows build and was replaced.

## Person, environment and observation

The guide changes its world position through CharacterMovement and capsule
collision. The terrain remains in world coordinates; local ground deformation
responds to support load. The automated motion camera follows the guide and
changes the observation frame. Rendering the scene relative to a stationary
camera or rebasing world coordinates is a coordinate choice, rather than a
replacement for articulated walking, planted feet and weight transfer.

The current animation uses authored idle poses, procedural limb solving,
contact phases and smoothed gaze. The final native render review still shows
smooth wax-like faces and limited expressive locomotion. Art acceptance for
convincing human appearance and motion remains OPEN. Correct endpoint matching
and a successful gravity test are technical measurements, rather than visual
acceptance of the character design.

## Verification

The current receipt is [native-body-development.json](../QA/native-body-development.json).
It binds native build/gameplay checks, the selected rendered views, matching
pose/foot targets from completed animation evaluations, lineage verification,
source tests and the preserved prior implementation.

The math suite checks anatomical segment preservation over 10,000 limb solves,
1,000 supported hip positions, body reserve/recovery for three profiles and
256 exact genome encode/decode samples. The rendered audit is finite sampling:
it provides sampled posture and movement evidence, rather than a biological
equivalence claim or a judgment that the game's final art quality is complete.

GitHub's strict GCC check identified an ambiguous-looking same-line return and
declaration in `GuideGenome.h`. The publication fix inserts a line break between
the two statements, preserving their tokens and control flow. The native
receipt retains the input hash of the runtime-tested parent; the format bridge
is bound in `QA/genome-source-format-bridge.json`.
