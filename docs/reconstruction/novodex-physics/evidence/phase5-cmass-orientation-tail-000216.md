# Phase 5 `phys_fn_000216` orientation-setter tail mutation

`phys_fn_000216` is the final 163-byte tail of `NpActorVtable::setCMassOffsetLocalOrientation` (`phys_fn_000214`), with no independent entry point. This probe isolates the tail's static/kinematic error-report arm at the recorded source line `0x39f`; the setter's successful store path is covered separately by the 000214 mutation evidence.

The registered `NxPhysicsActorDynamicSetterTests` baseline matched the pinned oracle exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The baseline candidate DLL hash was `6bc25cbc3825d8b376de3b10818fce231e464b917cbf4c2cc08d3db59ca7f07d`.

In the isolated `codex/nxphysics-cmass-setter-closures` worktree, the one-line mutation changed the error report's source line from `0x39f` to `0x39e`, leaving the setter state changes untouched. After rebuilding both `NxPhysics` and the registered test target, the differential rejected the mutant with both processes exiting zero, exact stderr, and `stdout_delta=4`. The mutant DLL hash was `a14f659336496835a991f5db870192a5d5c5a05635cf7d01261ba8a34e1e6749`.

The source line was restored to `0x39f`, `NxPhysics` and the test target were rebuilt, and the restored differential returned to `stdout_delta=0` with exact stderr and zero exits. The restored candidate DLL hash was `498cc83a3280633e860d29d991d050a04b8ce816275a631d744c54eecd981b57`. Logs are retained locally as `build/phase5-000216-mutant.log` and `build/phase5-000216-restored.log`; the initial baseline output was captured in the task transcript.

No public header changed. The production source was restored byte-for-byte after the mutation; this closes the separately identified error-report tail behavior only.
