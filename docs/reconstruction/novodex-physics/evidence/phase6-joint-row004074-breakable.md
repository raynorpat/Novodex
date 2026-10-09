# Phase 6 closure: joint break-threshold setter (`phys_fn_004074`)

`phys_fn_004074` at RVA `0x00095ab0` implements `Joint::setBreakable` in `Physics/src/core/Joint.cpp`. It validates the thresholds, stores maxForce and maxTorque, and raises the connected bodies’ wake counters.

The registered `NxPhysicsCoreDumpTests` staged-pair fixture sets breakability on a cylindrical joint (`100.0`, `250.5`) and a fixed joint (`1000.0`, max finite float), then serializes those values. In the clean isolated worktree at mainline commit `d0614344`, storing zero for `mMaxForce` was caught: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=20`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
