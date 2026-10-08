# Scene ground and bounds planes

`NxSceneDesc::groundPlane` creates one static actor with the default `y = 0`
plane. `boundsPlanes` creates six static plane actors from `maxBounds`, ordered
X maximum, X minimum, Y maximum, Y minimum, Z maximum, Z minimum.

IDA's `phys_fn_000501` (`0x0000ff10`) computes the six plane equations. For
`NxBounds3` storage `[minX,minY,minZ,maxX,maxY,maxZ]`, the equations use these
normals and `d` values (`normal dot x = d`):

| Face | Normal | d |
| --- | --- | --- |
| X maximum | `(-1,0,0)` | `-maxX` |
| X minimum | `(1,0,0)` | `minX` |
| Y maximum | `(0,-1,0)` | `-maxY` |
| Y minimum | `(0,1,0)` | `minY` |
| Z maximum | `(0,0,-1)` | `-maxZ` |
| Z minimum | `(0,0,1)` | `minZ` |

`NxPhysicsSceneBoundsPlanesTests` uses asymmetric bounds, checks all six
equations through the public actor and shape descriptors in oracle order, checks
the default ground plane separately, and verifies that both flags together create seven
actors in ground-then-bounds order. The first run against the candidate failed as
expected with zero actors. After reconstruction, it passed against both the
pinned DLL and candidate. The test is registered in the Phase 7 differential and
contributes three coverage assertions. This is baseline differential coverage;
the helper has not yet been mutation-falsified for Phase 7 closure.

Verification: `run_phase_gate.ps1 -Phase 7` passes with 1,383/1,383 coverage
assertions; the focused target reports `stdout_delta=0` and `stderr_exact=True`.
The captured gate transcript is `build/scene-bounds-planes-phase7.log`. The
gate's inventory validation passes, and the immutable-header checks pass for all
80 public Physics headers in both trees.

The implementation is in `Physics/src/Scene.cpp`. No public Physics header was
changed.
