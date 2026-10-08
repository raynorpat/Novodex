# `phys_fn_000134` center-of-mass global pose

`phys_fn_000134` (`NxPhysics.dll+0x47d0`, 907 bytes) is implemented by `NpActorVtable::getCMassGlobalPoseVal` in `Physics/src/NpActor.cpp`. It guards the actor, derives the mass-frame world pose from the dynamic-body record, copies the 3x3 matrix from record+0x134 and the position from record+0x158, then returns the value.

`NxPhysicsActorCMassTests` exercises this getter with single and grouped actors and rotated mass frames. The clean staged pair matches exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

For falsification, changed only the matrix source offset from record+0x134 to record+0x138. This shifts the returned matrix and changes the recorded pose values; the registered target rejects the candidate with both processes exiting 0, `stdout_delta=54`, and exact stderr. Restoring the original offset and rebuilding returns an exact transcript. Captured runs: `build/m2-cmass-pose-baseline-runner.log`, `build/m2-cmass-pose-mutant-runner.log`, and `build/m2-cmass-pose-restored-runner.log`.

Closure: `differential_falsified` on `NxPhysicsActorCMassTests`, `stdout_delta=54`. No public Physics headers changed.
