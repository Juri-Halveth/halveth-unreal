# World contact and moving guides — 2026-10-04

This increment changes the independent HALVETH Portal Garden Unreal project.
The four landscapes retain their 600 × 600 metre authored extent. This pass
expands guide movement and adds forest contact; it does not replace retail
Morrowind assets or introduce a new character art set.

## Runtime changes

- Scarlet, Lucinet and Rachel follow time-varying goals within a 6 × 5.2 metre
  local patrol area, at a maximum translational speed of 84 centimetres/second.
- Before requesting a step, each guide checks ground support, slope and step
  height, then sweeps its capsule in the requested direction. If obstructed,
  it also tries headings 60 degrees to either side. This is local steering;
  a global navigation route planner remains a separate development task.
- Turning in place supplies zero invented translational speed to the gait.
  Steps now follow the character movement component's observed XY velocity.
- Every placed forest tree has a hidden simple cylinder for trunk collision.
  Foliage rendering remains independent, allowing movement beside branches.
  Cylinder dimensions are approximations derived from the selected mesh bounds.
- Character view captures follow each guide's current position. Dialogue and
  dodge checks also use the moving character instead of a fixed spawn address.

## Test scope

The native sequence exercises four realms, terrain, trunk rays, character
animation, patrol displacement, inventory, dialogue, dodge, spells, the
break/reform crystal cycle, construction and save/restore. The additional patrol
assertion requires all three guides to move at least 1.5 metres from their origin
within this test sequence. See [the measurement receipt](../QA/native-world-scale.json).

Trunk tests sample the first, middle and last instance of each loaded realm.
They test collision geometry, while free play supplies the ordinary movement
and collision response. The sample establishes this bounded coverage rather
than every possible terrain position or path.

The motion capture uses a controlled 1.5 metre lift/fall, distant-camera walking,
nearby conversation and resumed walking. Its sparse variant records 40 native
1280 × 720 images over roughly 20 simulation seconds at a nominal 2 Hz. These
are sampled stills, not a smooth 24/60 FPS movie. Actual sample times are retained
in `QA/world-scale-motion-frames.tsv`. The first image includes the deliberate
lift for the gravity check.

```bash
bash GARDEN.sh play     # ordinary game
bash GARDEN.sh check    # native gameplay checks
bash GARDEN.sh motion   # short native motion/gravity capture, then automatic exit
bash GARDEN.sh visual   # four realms, characters and crystal views
```

The sparse capture writes `Preview/WorldScale/frame-*.png`,
`QA/native-world-scale-motion.json` and `Saved/GARDEN-motion.log`. The existing
full `-HalvethMotionCapture` entry point retains its nominal 24 Hz output path.

For a declared daylight review, `GARDEN_CAPTURE_SKY_UNIX` passes an explicit sky
time to `visual` and `motion`. Ordinary play uses its normal live clock. The
daylight review in this increment uses `1791115200`, 2026-10-04 12:00 UTC, with
the project's cached NASA/JPL trajectory and declared virtual observer.

## Visual findings and continuation

Movement changes are visible in the sampled native views. Ground texture and
lighting remain the existing art. The overview review still shows stylized
posture, hand placement, very bright portal faces and primitive pickup props.
These are preserved as explicit art/animation tasks: captured gait with the
current skeleton, stronger foot/hip coordination, relaxed elbows and hands,
skin detail, fantasy clothing, portal luminance and authored pickup meshes.
Some stride poses retain pronounced knee flexion or apparent sole clearance;
capsule-ground contact and visible footwear contact require separate measurement.
Passing gameplay checks establishes function within the test scope; it does
not establish a finished photorealistic remake.
