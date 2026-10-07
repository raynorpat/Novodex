# Phase 5 `getLinearDamping` row

`phys_fn_000050` reads the dynamic actor's linear damping value from its body record at `+0xb8`, under the read guard. The registered `NxPhysicsActorDynamicSetterTests` transcript now pins `setter damping=3ecccccd.3f19999a.3ecccccd.3f19999a`: stored linear and angular damping values followed by the public getters. The same target already checks the static-actor zero return and its `NXE_INVALID_OPERATION` callback.

The clean staged-pair differential passed with `stdout_delta=0` and exact stderr. In an isolated mutation build, changing the getter offset from `+0xb8` to `+0xb4` changed the getter output from `3ecccccd` to `0`; the registered pair differential caught the mutation with `stdout_delta=2` while both processes exited zero and stderr remained exact.

Mutation evidence for `phys_fn_000050`: wrong field offset `+0xb4` was caught by `NxPhysicsActorDynamicSetterTests` with `stdout_delta=2`. Public Physics headers were unchanged.
