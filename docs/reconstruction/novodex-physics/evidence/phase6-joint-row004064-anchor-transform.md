# Phase 6 closure: shared joint-anchor transform (`phys_fn_004064`)

`phys_fn_004064` at RVA `0x000957a0` implements `Joint::row004064` in `Physics/src/core/Joint.cpp`. It transforms both stored local anchors through the corresponding body poses and returns their world-space difference. Revolute and spherical projection callbacks consume this vector.

The registered `NxPhysicsJointSlotTests` staged-pair fixture creates rotated multi-family joints, moves body 1 after joint creation, and directly invokes projection slot 8 for the held body records. It runs the step and projection callbacks under both the default x87 control word and `0x0f7f`, so the row is reached with nonzero anchor error.

In the clean isolated worktree at mainline commit `58f0d3a3`, changed body 0’s transformed X anchor by `+1.0`. The registered differential rejected the mutant: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=250`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed: both exits were 0, `stdout_delta=0`, and stderr matched exactly.

The mutation was confined to the isolated worktree and was removed before recording this evidence.
