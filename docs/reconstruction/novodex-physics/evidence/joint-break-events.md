# Joint break-event drain reconstruction

The oracle's `fetchResults` wrapper calls scene event dispatch before `finishSimulation`. Scene row `000640` invokes the joint-break list drain `000577` at `Scene+0x620`; that drain calls each event's slot 0 and frees the event. With no user notify, event row `004113` calls `004105`, which removes the joint while its body pointers still identify the active island, marks it broken, wakes and clears both bodies, then moves it to the no-body list.

The candidate now follows this path through `NpScene::fetchResults`, `NxSceneInternal::processJointBreakEvents`, `JointBreakEvent::row004113`, and `Joint::handleBreakEvent`. The public headers were not changed.

## Verification

- `build/break-event-exact-detach.log`: the four-step low-force fixed-joint test matches the pinned oracle (`stdout_delta=0`, exact stderr).
- `build/phase7-break-event-drain-registered.log`: all eight Phase 7 targets pass; the break steps and joint states are registered in `tools/gate_targets.ps1`. The Phase 7 coverage floor is now 1,227, up from 1,218 by these nine assertions.
- `build/phase5-after-break-fix.log`: all 13 Phase 5 targets pass.
- CDB candidate trace on the staged candidate pair hit `JointBreakEvent::row004113` once and `Joint::handleBreakEvent` once. The oracle trace hit `0x00098050` and `0x00097ca0`; the callback call site was inside `phys_fn_000577` at `0x000109c0`.
- Staged candidate pair at verification: `NxPhysics.dll` SHA-256 `4dc50397e20b32996bb510e3c905611db4e833b122dba9cf31c504cded36639a`; `NxFoundation.dll` SHA-256 `98cf7a0ebb004969682612677c9a16da986792cd8f2d98c26371d702f50d3166`.

The tested route uses no `NxUserNotify`. The notify-return-true release path still needs a dedicated fixture.
