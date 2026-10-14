# Scene raycast any-shape wrapper (`phys_fn_000704`)

The row at RVA `0x15a50` implements `NxSceneInternal::raycastAnyShape`, validating the unit ray, collecting candidate shapes, and forwarding the query to `nxRaycastAnyShapeLoop`.

## Mutation proof

Replacing the loop-forwarding return with `return false` in a throwaway source mutation and rebuilding `NxPhysics` plus `NxPhysicsSceneRaycastTests` through CMake is rejected by the registered Phase 7 staged-pair differential: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=288`, and exact stderr.

After restoring and rebuilding the original forwarding implementation, the differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-scene-raycast-000704-20261014/`.
