# Scene closest-shape distance finalization (`phys_fn_000706`)

The row at RVA `0x15b20` is `NxSceneInternal::raycastClosestShape`, which runs the closest-shape loop and converts the retained squared-distance key to the public distance before returning the public shape.

## Mutation proof

Replacing the final `x87Fsqrt` distance conversion with `hit.distance = 0.0f` in a throwaway source mutation, then rebuilding `NxPhysics` and `NxPhysicsSceneRaycastTests` through CMake, is rejected by the registered Phase 7 staged-pair differential: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=288`, and exact stderr.

Restoring and rebuilding the original conversion makes the differential exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-scene-raycast-000706-20261014/`.
