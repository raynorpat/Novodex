# Scene raycast closest-hit selection (`phys_fn_000702`)

The 277-byte census row at RVA `0x15930` is the body of `nxRaycastClosestShapeLoop`. The preceding empty-collector entry guard is separately closed as `phys_fn_000700`.

## Mutation proof

For falsification, the comparison selecting a new closest hit was inverted from `if(key < hit.distance)` to `if(key > hit.distance)`. The changed source was rebuilt with CMake (`NxPhysics` and `NxPhysicsSceneRaycastTests`, Release, Win32). The registered Phase 7 staged-pair differential rejected the candidate with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=288`, and exact stderr.

The original comparison was restored and both CMake targets rebuilt. The registered raycast differential then passed with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-scene-raycast-000702-20261014/`.
