# Phase 5 `getLinearDamping` row

`phys_fn_000050` reads the dynamic actor's linear damping value from its body record at `+0xb8`, under the read guard. The registered `NxPhysicsActorDynamicSetterTests` transcript now pins `setter damping=3ecccccd.3f19999a.3ecccccd.3f19999a`: stored linear and angular damping values followed by the public getters. The same target already checks the static-actor zero return and its `NXE_INVALID_OPERATION` callback.

The clean staged-pair differential passed with `stdout_delta=0` and exact stderr. In an isolated mutation build, changing the getter offset from `+0xb8` to `+0xb4` changed the getter output from `3ecccccd` to `0`; the registered pair differential caught the mutation with `stdout_delta=2` while both processes exited zero and stderr remained exact.

Mutation evidence for `phys_fn_000050`: wrong field offset `+0xb4` was caught by `NxPhysicsActorDynamicSetterTests` with `stdout_delta=2`. The full Phase 5 gate subsequently passed at merged commit `559b865c`, with 2,250/2,250 coverage assertions. Public Physics headers were unchanged.


## Angular damping getter: `phys_fn_000052`

The same required `setter damping` transcript line also pins `getAngularDamping()` at record `+0xbc`: after setting linear damping to `0.4` and angular damping to `0.6`, the public getters return `3ecccccd` and `3f19999a`. The clean paired differential passes with `stdout_delta=0`.

An isolated mutation changed `getAngularDamping` to read `+0xb8`; the candidate returned the linear value `0.4` instead of `0.6`. `NxPhysicsActorDynamicSetterTests` caught it with `stdout_delta=2`, both processes exiting zero and stderr exact.

Mutation evidence for `phys_fn_000052`: wrong getter offset `+0xb8` was caught with `stdout_delta=2`.
