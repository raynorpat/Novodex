# `phys_fn_000036` actor shape creation return

`phys_fn_000036` (`NxPhysics.dll+0x2250`, 420 bytes) is implemented by `nxActorCreateShape` in `Physics/src/Scene.cpp`. The registered `NxPhysicsActorShapeMutationTests` target creates shapes on actors with no root, one root, and grouped roots. It records whether the public call returned a shape and whether the handle is present in the actor's child array.

The clean staged pair is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For falsification, changed only the final `nxActorCreateShape` return from the newly created shape pointer to null after the body had already installed the new shape. The candidate transcript records `shape_mutation added=0` and `shape_mutation group_added=0` where the oracle reports successful handles; the test then faults when it validates the absent handle. The staged runner rejects the mutant with `oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=359`, and exact stderr. Restoring the return and rebuilding returns an exact baseline (`stdout_delta=0`, exact stderr; both exit 0). Captured runs: `build/m2-create-shape-return-mutant-runner.log` and `build/m2-create-shape-return-restored-runner.log`.

Closure: `differential_falsified` on `NxPhysicsActorShapeMutationTests`, `stdout_delta=359`. No public Physics headers changed.
