# `NxSceneInternal::processJointBreakEvents` mutation closure (`phys_fn_000577`)

`phys_fn_000577` is the 79-byte Scene row at RVA `0x000109c0`. It drains the queued joint-break events, dispatches each event through `JointBreakEvent::row004113`, frees it, and clears the Scene head. The registered Phase 7 `NxPhysicsSimulationTests` fixture exercises both `NxUserNotify::onJointBreak` outcomes: retain the broken joint or release it from the callback.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr), including both break-notify result lines. For the row-specific mutation, an immediate return was inserted at the start of `processJointBreakEvents`. The callback was never delivered; the registered fixture rejected the candidate with `FAIL joint-break notify callback count was not one` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=801`, candidate stderr contains that assertion failure). Mutant `NxPhysics.dll` SHA-256: `523024c2744fbc6f7c7eef6aef954bed18f1b6040a0fa2666697d9f7288ad4c8`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `20ec465bd3e08b4ad518b82c7718b211cd74dff6920c312f57b855e3db6bd1ae`.

Evidence: `build/mutation-000577/mutant-differential.log` and `build/mutation-000577/restored-differential.log`. The row is now closed in the Phase 7 ledger. Phase 8 terminal closure remains open.
