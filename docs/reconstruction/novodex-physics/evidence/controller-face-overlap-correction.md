# Controller face-overlap correction probe (2026-10-07)

The approved standalone scene places a small rising triangle inside the moving
controller's footprint. The controller descends through the triangle plane
while its corner rays miss the face. Before the change, the pinned DLL and the
candidate stopped at identical position bits
`3f000000.3f8ccccd.3f000000`, but the oracle returned collision flags `0` and
the candidate returned `4`.

The candidate's continuous triangle SAT selects an entry axis for movement
response. In this scene that axis is world-up even though the triangle face is
sloped, so using it for the slope threshold skips the resolver's qualifying
correction path. The candidate now keeps the triangle face normal separately
for slope classification and retains the SAT normal for response arithmetic;
this preserves the already-matching sloped-correction simulation transcript.
For a qualifying descending face correction, the final downward-probe flag is
consumed even when the corner-ray subquery has no hit. The focused fixture now
matches the pinned DLL's pose and flags exactly.

Validation on the Win32 Release build:

- `NxPhysicsControllerSweepFaceTests` and `NxPhysicsSimulationTests` match the
  pinned oracle exactly (`stdout_delta=0`, exact stderr; see
  `build/controller-face-regression.log`).
- The full Phase 5 gate passes 18 staged targets and 2,302/2,302 coverage
  assertions (`build/phase5-approved-face-scene.log`).
- The full Phase 7 gate passes 13 staged targets and 1,380/1,378 coverage
  assertions (`build/phase7-approved-face-scene.log`).
- All 48 Viewer CTest selections complete across the 39 available scenes: 43
  pass and five existing pinned-oracle asset cases skip under their established
  signatures (`build/viewer-all-scenes-controller-face-overlap.log`).
- Inventory validation reports 6,338 functions, 5,138 data objects, and zero
  unexplained items. All 80 pinned public Physics headers remain unchanged.

This closes only the tested face-overlap correction and the observed Phase 5
red transcript. General controller face/edge callbacks, transformed sweeps,
successful step-up behavior, and the remaining NxPhysics reconstruction stay
open.
