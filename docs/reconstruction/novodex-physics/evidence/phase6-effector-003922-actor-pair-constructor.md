# Phase 6 closure: ActorPairEffector constructor (phys_fn_003922)

`phys_fn_003922` at RVA `0x0008ed20` constructs the actor-pair effector and initializes its two body-record pointers to null. The registered `NxPhysicsEffectorTests` staged-pair differential exercises this constructor through the public scene factory.

In a throwaway git archive, the constructor was mutated to initialize `mBody[0]` to address 1 instead of null. The factory then passed that invalid old pointer to the body-record setter. The oracle exited 0; the mutated candidate hit an access violation. The registered differential caught it with `stdout_delta=78` and exact stderr. The mutation remained only in the archive.

The restored worktree control passed with both processes exiting 0, `stdout_delta=0`, and exact stderr. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
