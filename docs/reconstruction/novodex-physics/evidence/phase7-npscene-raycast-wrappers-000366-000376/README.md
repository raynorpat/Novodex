# Phase 7 NpScene raycast wrapper mutation closures (2026-10-12)

Rows `phys_fn_000366`, `000368`, `000370`, `000372`, `000374`, and `000376` are the public `NpScene` any/all/closest bounds/shape raycast wrappers. The registered `NxPhysicsSceneRaycastTests` fixture already covers static and dynamic geometry, hits and misses, filters and range limits, report termination, moved geometry, and the returned hit shape/flags/impact data.

The unmutated candidate was rebuilt from this source and matched the pinned oracle exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Each proof then changed only its wrapper's delegated call to a constant no-hit/null result, rebuilt `NxPhysics.dll` and the fixture, and ran the registered staged-pair differential. All six mutants were caught; candidate and oracle test processes both exited zero and stderr remained exact. The clean source was restored byte-for-byte, rebuilt, and the restored control again matched exactly.

| Census row | Wrapper mutation | Differential result |
| --- | --- | --- |
| `phys_fn_000366` (`0x0000ced0`) | `NpScene::raycastAnyBounds` returns false | `stdout_delta=2,721` |
| `phys_fn_000368` (`0x0000cf90`) | `NpScene::raycastAnyShape` returns false | `stdout_delta=2,667` |
| `phys_fn_000370` (`0x0000d050`) | `NpScene::raycastAllBounds` returns 0 | `stdout_delta=5,103` |
| `phys_fn_000372` (`0x0000d110`) | `NpScene::raycastAllShapes` returns 0 | `stdout_delta=5,137` |
| `phys_fn_000374` (`0x0000d1d0`) | `NpScene::raycastClosestBounds` returns null shape | `stdout_delta=2,819` |
| `phys_fn_000376` (`0x0000d2b0`) | `NpScene::raycastClosestShape` returns null shape | `stdout_delta=2,815` |

The per-row unified patches and full runner transcripts are stored beside this README and under `evidence/raw/phase7-npscene-raycast-wrappers-000366-000376-20261012/` as losslessly compressed `.log.gz` files. No public header changed.
