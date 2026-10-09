# Phase 6 closure: joint base descriptor save (`phys_fn_004066`)

`phys_fn_004066` at RVA `0x00095930` implements `Joint::saveToDescBase` in `Physics/src/core/Joint.cpp`. It copies the shared joint state—including actors, local frames, force/torque break thresholds, user data, and base flags—into each public joint descriptor.

The registered `NxPhysicsJointStagedPairTests` staged-pair differential creates rotated joints across multiple families, saves the descriptors, and records their shared base values. In the clean isolated worktree at mainline commit `3b87269f`, changing the saved maxForce assignment to zero was caught: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=216`, `stderr_exact=True`. After restoring `Joint.cpp` and rebuilding `NxPhysics`, the exact control passed with both exits 0, `stdout_delta=0`, and exact stderr.

The mutation was confined to the isolated worktree and removed before recording this evidence.
