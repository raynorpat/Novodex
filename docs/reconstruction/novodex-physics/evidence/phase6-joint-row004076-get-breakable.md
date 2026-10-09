# Phase 6 closure: joint break-threshold getter (`phys_fn_004076`)

`phys_fn_004076` at RVA `0x00095b90` implements `Joint::getBreakable` in `Physics/src/core/Joint.cpp`. The cylindrical-joint fixture now reads the values back through the public API after setting force `100.0` and torque `250.5`, printing exact words `42c80000.437a8000`.

In the isolated worktree at mainline commit `cf83177c`, changed the internal getter to return zero for maxForce. The registered `NxPhysicsCoreDumpTests` staged-pair differential caught it: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
