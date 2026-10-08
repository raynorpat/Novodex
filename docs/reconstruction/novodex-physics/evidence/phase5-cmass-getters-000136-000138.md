# `phys_fn_000136` and `phys_fn_000138` CMass getters

`phys_fn_000136` and `phys_fn_000138` (`NxPhysics.dll+0x4b60` and `+0x4d60`) implement `NpActorVtable::getCMassGlobalPositionVal` and `NpActorVtable::getCMassGlobalOrientationVal` in `Physics/src/NpActor.cpp`. They derive the world-space mass frame from the actor record and return its translation or matrix.

`NxPhysicsActorCMassTests` records both getters for rotated single and grouped actors. The clean staged pair and restored candidate match exactly (both exit 0, `stdout_delta=0`, exact stderr).

For `phys_fn_000136`, changed the returned position source offset from `0x158` to `0x15c`; the registered differential rejects the shifted position with both processes exiting 0, `stdout_delta=54`, and exact stderr. For `phys_fn_000138`, changed the orientation matrix source from `0x134` to `0x138`; the target rejects the changed orientation with the same exit and transcript-delta result. Restoring both offsets returns an exact baseline. Captured runs: `build/m2-cmass-position-baseline-runner.log`, `build/m2-cmass-position-mutant-runner.log`, `build/m2-cmass-orientation-mutant-runner.log`, and `build/m2-cmass-getters-restored-runner.log`.

Closures: `differential_falsified` on `NxPhysicsActorCMassTests`, `stdout_delta=54` for each row. No public Physics headers changed.
