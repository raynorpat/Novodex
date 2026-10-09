# Phase 6 closure: Joint::purgeLimitPlanes (phys_fn_004089)

`phys_fn_004089` at RVA `0x00095d20` is implemented in `Physics/src/core/Joint.cpp`. The registered `NxPhysicsCoreDumpTests` staged-pair differential observes allocator balance after scene release, exercising the joint limit-plane cleanup path.

In a throwaway git archive copy, the allocator free in `Joint::purgeLimitPlanes` was replaced with a no-op while list traversal and iterator reset remained intact. The candidate exited successfully but retained three extra allocations: oracle count 14, candidate count 17. The registered differential caught the mutation with `stdout_delta=2` and exact stderr. The archive source was then restored from the worktree, rebuilt, and rerun: both allocator counts were 14, `stdout_delta=0`, and stderr was exact. The mutation remained in the throwaway archive.

This closes the row as `differential_falsified` under `NxPhysicsCoreDumpTests`.
