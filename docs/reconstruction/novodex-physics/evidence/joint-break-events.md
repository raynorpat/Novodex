# Joint break-event drain reconstruction

The oracle's `fetchResults` wrapper calls scene event dispatch before `finishSimulation`. Scene row `000640` invokes the joint-break list drain `000577` at `Scene+0x620`; that drain calls each event's slot 0 and frees the event. With no user notify, event row `004113` calls `004105`, which removes the joint while its body pointers still identify the active island, marks it broken, wakes and clears both bodies, then moves it to the no-body list.

The candidate now follows this path through `NpScene::fetchResults`, `NxSceneInternal::processJointBreakEvents`, `JointBreakEvent::row004113`, and `Joint::handleBreakEvent`. The public headers were not changed.

## Verification

- `build/break-event-exact-detach.log`: the four-step low-force fixed-joint test matches the pinned oracle (`stdout_delta=0`, exact stderr).
- `build/phase7-break-event-drain-registered.log`: all eight Phase 7 targets pass; the break steps and joint states are registered in `tools/gate_targets.ps1`. The Phase 7 coverage floor is now 1,227, up from 1,218 by these nine assertions.
- `build/phase5-after-break-fix.log`: all 13 Phase 5 targets pass.
- CDB candidate trace on the staged candidate pair hit `JointBreakEvent::row004113` once and `Joint::handleBreakEvent` once. The oracle trace hit `0x00098050` and `0x00097ca0`; the callback call site was inside `phys_fn_000577` at `0x000109c0`.
- Staged candidate pair at verification: `NxPhysics.dll` SHA-256 `4dc50397e20b32996bb510e3c905611db4e833b122dba9cf31c504cded36639a`; `NxFoundation.dll` SHA-256 `98cf7a0ebb004969682612677c9a16da986792cd8f2d98c26371d702f50d3166`.

The original four-step break fixture uses no `NxUserNotify`; callback outcomes are recorded below.

## User-notify return outcomes (`phys_fn_004113`, 2026-10-08)

`NxPhysicsSimulationTests` now creates the same low-force fixed-joint break with
a real `NxUserNotify` callback returning both values. Both oracle and candidate
invoke the callback once with the expected joint and the same raw break-force
word. Returning false leaves the broken joint in the scene (`state=2`, joint
count 1) and `getActors()` confirms both body pointers are null; returning true
releases it after the callback (callback-time state 2, scene joint count 0).
The exact registered transcript lines are in
`tools/gate_targets.ps1` and contribute to the Phase 5, 6 and 7 coverage floors.
The refreshed Phase 5, 6, and 7 gates pass at 2,306/2,306, 1,064/1,064, and
1,385/1,385, respectively; the retained line records `detached=1`. Logs:
`build/phase5-joint-break-notify-detach-gate.log`,
`build/phase6-joint-break-notify-detach-gate.log`, and
`build/phase7-joint-break-notify-detach-gate.log`.

For mutation proof, commit `41cef17d` was exported with `git archive` to
`build/joint-break-archive-41cef17d`. Its clean `NxPhysicsSimulationTests`
baseline is exact. In that isolated archive, changing the callback-true branch
from `if(handled)` to `if(false && handled)` causes the candidate to retain the
joint and the registered differential catches it with `stdout_delta=2` and
exact stderr. The archive source was restored byte-for-byte to the commit's
`Physics/src/Scene.cpp` blob, rebuilt, and the differential returned to exact
output. Captured local logs: `build/joint-break-archive-41cef17d/clean.log`,
`mutant.log`, and `restored.log`.
