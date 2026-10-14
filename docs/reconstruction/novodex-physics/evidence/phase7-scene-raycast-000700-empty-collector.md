# Scene raycast empty-collector entry (`phys_fn_000700`)

The census row at RVA `0x15910` is the empty-collector guard at the entry to `nxRaycastClosestShapeLoop`. Its continuation/body is separately represented by `phys_fn_000702`. This packet closes only the guard row.

## Mutation proof

Starting from the restored source, the registered Phase 7 target `NxPhysicsSceneRaycastTests` passes its staged oracle/candidate differential with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr.

For falsification, the source guard was changed from `if(!count) return;` to `if(count) return;` at `phys_fn_000700`. The mutated source was configured and rebuilt through CMake (`NxPhysics` and `NxPhysicsSceneRaycastTests`, Release, Win32). The same registered differential rejected the candidate: `oracle_exit=0`, `candidate_exit=-1073741819 (0xC0000005)`, `stdout_delta=2707`, `stderr_exact=True`.

The source guard was restored and both CMake targets rebuilt. The restored staged-pair differential passes with zero stdout delta and exact stderr. No public Physics headers were changed.

Raw differential logs are archived under `evidence/raw/phase7-scene-raycast-000700-20261014/`.
