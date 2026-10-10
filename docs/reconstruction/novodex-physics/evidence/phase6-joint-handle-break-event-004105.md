# Phase 6 mutation evidence: `phys_fn_004105` — retained joint break detach

`phys_fn_004105` (`Joint::handleBreakEvent`, RVA `0x00097ca0`) removes a broken joint from the active scene/island while its actor body pointers are still available, marks it broken, wakes both bodies, clears both actor pointers, then pushes the joint onto the scene's no-body list. The existing retained-break case in `NxPhysicsSimulationTests` checks the callback result, retained broken state, both detached actor pointers, joint count, and scene cleanup. Its exact expected output is registered for Phase 6.

The clean baseline staged-pair differential passed: oracle and candidate exited 0, `stdout_delta=0`, and stderr matched exactly. I then removed only the `mBody[0] = 0` store from `Joint::handleBreakEvent` and rebuilt `NxPhysics.dll`. The candidate failed with `FAIL retained broken joint actors were not detached`; oracle exit was 0, candidate exit was 1, `stdout_delta=796`, and stderr differed due to the expected assertion failure. The mutation candidate hash was `4213ca025aa449c3630f9782a676620b7160784e64c4dab4e741d301f35b3ca6`.

Restoring `Joint.cpp` and using `cmake --build build --config Release --clean-first --target NxPhysics NxPhysicsSimulationTests` returned the staged differential to exact output (both exits 0, `stdout_delta=0`, exact stderr). The restored candidate hash was `e8cba040a121a18ebed7170c380af2321e9864c20edf0abc1347d5bdacd3625d`; the pinned oracle hash was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

An initial mutation that changed only the broken-state mask did not change output, because the solver had already set the broken bit before this handler ran. That attempt is excluded from closure evidence. The body-pointer mutation above is the mutation that closes the row.

Source revision before this evidence commit: `64099b72234ec4c95632f9ccacb7b922eb942b1e`.

Exact commands:

- `cmake --build C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build --config Release --target NxPhysics NxPhysicsSimulationTests`
- `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\docs\reconstruction\novodex-physics\tools\run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex -BuildRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build\pairs`
- `cmake --build C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build --config Release --clean-first --target NxPhysics NxPhysicsSimulationTests` (restored control)
- `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\docs\reconstruction\novodex-physics\tools\run_phase_gate.ps1 -Phase 6 -RepoRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex -BuildRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-contact-report-scene-release\Novodex\build\pairs`

Raw differential transcripts are retained in `phase6-joint-handle-break-event-004105-mutant.log` and `phase6-joint-handle-break-event-004105-restored.log`.

The full Phase 6 gate also passed after restoration: `phase_gate=6 status=pass`, with all 1,267/1,267 registered coverage assertions evaluated. `validate_inventory.py` reports 98 closed and 335 deferred Phase 6 function rows.
