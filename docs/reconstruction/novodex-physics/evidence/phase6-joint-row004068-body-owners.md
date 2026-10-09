# Phase 6 closure: joint body-owner reader (`phys_fn_004068`)

`phys_fn_004068` at RVA `0x00095a40` implements `Joint::getBodyOwners` in `Physics/src/core/Joint.cpp`. It resolves each internal joint body record to its public actor owner, with null for a missing world body. `SceneDump` uses these pointers when it serializes joint actor references.

The registered `NxPhysicsCoreDumpTests` staged-pair fixture creates named actors and joints across multiple families, then writes the core dump. In the clean isolated worktree at mainline commit `cea17aaa`, changing the body-0 result to null was caught: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=128`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
