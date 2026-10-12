# Phase 7 NpScene overlap-query wrapper mutation closures (2026-10-12)

Rows `phys_fn_000378`, `000380`, `000382`, `000384`, and `000386` are the public sphere/AABB collection, plane-cull, and sphere/AABB boolean overlap wrappers. The registered `NxPhysicsSceneRaycastTests` fixture already exercises collection limits and callbacks, plane culling, misses and hits, broadphase type selection, moved geometry, and rotated/tangent bounds.

The unmutated candidate matched the pinned oracle exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Each proof replaced only the delegated return with zero/false, rebuilt `NxPhysics.dll` and the fixture, and ran the registered staged-pair differential. Every mutant was caught; both processes exited zero and stderr remained exact. The clean source was restored byte-for-byte, rebuilt, and the restored control again matched exactly.

| Census row | Wrapper mutation | Differential result |
| --- | --- | --- |
| `phys_fn_000378` (`0x0000d390`) | `NpScene::overlapSphereShapes` returns zero | `stdout_delta=5,465` |
| `phys_fn_000380` (`0x0000d400`) | `NpScene::overlapAABBShapes` returns zero | `stdout_delta=5,473` |
| `phys_fn_000382` (`0x0000d480`) | `NpScene::cullShapes` returns zero | `stdout_delta=5,459` |
| `phys_fn_000384` (`0x0000d500`) | `NpScene::checkOverlapSphere` returns false | `stdout_delta=66` |
| `phys_fn_000386` (`0x0000d580`) | `NpScene::checkOverlapAABB` returns false | `stdout_delta=14` |

The per-row unified patches and full runner transcripts are retained in this directory and in `evidence/raw/phase7-npscene-overlap-wrappers-000378-000386-20261012/` as losslessly compressed `.log.gz` files. No public header changed.
