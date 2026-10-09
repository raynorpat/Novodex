# Phase 6 closure: joint support-record allocator (`phys_fn_004093`)

`phys_fn_004093` at RVA `0x00095da0` implements `Joint::row004093` in `Physics/src/core/Joint.cpp`. It grows the Scene support-record array at `+0x5b8` when full, allocates the next record by advancing count `+0x5bc`, and updates the joint’s contiguous record window.

The registered `NxPhysicsJointSlotTests` staged-pair fixture builds support records across multiple joint families, invokes their solver slots, and prints the Scene record windows. In the clean isolated worktree at mainline commit `70ed5f86`, changing the count increment from one to two was caught: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2918`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
