# `phys_fn_000026` body record construction

`phys_fn_000026` (`NxPhysics.dll+0x19b0`, 465 bytes) is implemented by `nxActorBuildRecord` in `Physics/src/Scene.cpp`. When all three `massSpaceInertia` words are zero, it asks `nxActorComputeMassFromShapes` to fill the copied descriptor's mass, local pose, and inertia tensor before constructing and registering the dynamic-body record. `NxPhysicsBodyCreationTests` exercises this path and prints the resulting mass, inverse mass, and frame.

The clean staged-pair baseline and byte-restored candidate match exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For falsification, forced the all-zero tensor branch condition false. The oracle computes mass from the actor's shapes; the candidate skips the recomputation and reports different body fields. The registered differential rejects the mutation with both processes exiting 0, `stdout_delta=18`, and exact stderr. Restoring the branch and rebuilding returns an exact baseline. Captured runs: `build/m2-body-record-baseline-runner.log`, `build/m2-body-record-mutant-runner.log`, and `build/m2-body-record-restored-runner.log`.

Closure: `differential_falsified` on `NxPhysicsBodyCreationTests`, `stdout_delta=18`. No public Physics headers changed.
