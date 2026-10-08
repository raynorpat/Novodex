# `phys_fn_000128` global-pose reference

`phys_fn_000128` (`NxPhysics.dll+0x4430`, 333 bytes) is implemented by `NpActorVtable::getGlobalPoseReference` in `Physics/src/NpActor.cpp`. The row refreshes the actor body's cached global pose from its dynamic-body record. Its x and y position words pass through x87 load/store operations, which quiet signaling NaNs, while z is copied directly as a dword.

`NxPhysicsActorCMassTests` installs signaling-NaN words `7f800001`, `ff800002`, and `7fa00003` in the record position and calls the reference getter. The clean staged pair reports `7fc00001.ffc00002.7fa00003` on both sides and matches exactly (both exit 0, `stdout_delta=0`, exact stderr).

For falsification, changed only the z dword source from record+0x58 to record+0x54. The oracle retains z=`7fa00003`; the candidate returns y's `ff800002` instead. Both processes exit 0 and stderr remains exact, while the registered differential rejects the mutant with `stdout_delta=4`. Restoring the +0x58 load and rebuilding returns exact output. Captured runs: `build/m2-getglobalpos-ref-baseline-runner.log`, `build/m2-getglobalpos-ref-mutant-runner.log`, and `build/m2-getglobalpos-ref-restored-runner.log`.

Closure: `differential_falsified` on `NxPhysicsActorCMassTests`, `stdout_delta=4`. No public Physics headers changed.
