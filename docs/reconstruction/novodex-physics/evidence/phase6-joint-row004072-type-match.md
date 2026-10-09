# Phase 6 closure: shared joint type matcher (`phys_fn_004072`)

`phys_fn_004072` at RVA `0x00095a90` implements `Joint::is` in `Physics/src/core/Joint.cpp`. It returns `this` for the matching `NxJointType` and null otherwise. `SceneDump` uses it to select each family-specific serializer.

The registered `NxPhysicsCoreDumpTests` staged-pair differential serializes named joints from multiple families through those typed branches. In the clean isolated worktree at mainline commit `379b75c6`, changing the matcher to always return null was caught: `oracle_exit=0`, `candidate_exit=-1073741819` (`0xc0000005`), `stdout_delta=2206`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
