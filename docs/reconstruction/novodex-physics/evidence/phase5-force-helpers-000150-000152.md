# `phys_fn_000150` and `phys_fn_000152` force helpers

`phys_fn_000150` (`NxPhysics.dll+0x5e70`) implements `nxNpActorRotateLocalForce`, and `phys_fn_000152` (`+0x5fa0`) implements `nxNpActorLocalPosition`; both are in `Physics/src/NpActor.cpp`. `NxPhysicsActorForceTests` exercises them through the public `addLocalForce` and `addForceAtLocalPos` paths with rotated actors and records the resulting momentum and position.

The clean and restored staged-pair transcript is exact (both exit 0, `stdout_delta=0`, exact stderr). For `phys_fn_000150`, changed the first-row local-y coefficient from `m[1]` to `m[2]`; the force target rejects it with `stdout_delta=18`. For `phys_fn_000152`, changed the first-row local-y coefficient from `r[1]` to `r[2]`; the target rejects it with `stdout_delta=4`. Both mutant processes exit 0 and stderr remains exact. Captured runs: `build/m2-force-helpers-baseline-runner.log`, `build/m2-local-force-mutant-runner.log`, `build/m2-local-position-mutant-runner.log`, and `build/m2-force-helpers-restored-runner.log`.

Closures: `differential_falsified` on `NxPhysicsActorForceTests`, with per-row deltas of 18 and 4. No public Physics headers changed.
