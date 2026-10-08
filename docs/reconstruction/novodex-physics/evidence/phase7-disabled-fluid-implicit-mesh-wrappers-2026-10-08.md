# Phase 7 disabled fluid and implicit-mesh wrapper mutations

The registered `NxPhysicsSimulationTests` baseline matches the pinned oracle exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). In a throwaway `git archive` of the committed source, each row below was mutated independently, rebuilt, and rejected by the registered Phase 7 staged-pair differential. The archive was restored to HEAD and rebuilt; its final simulation differential returned to exact output.

| Row | Independent mutation | Detection |
| --- | --- | --- |
| `phys_fn_000400` | Replaced the NpScene::createFluid Scene::createFluid forwarding call with a null result in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the missing lazy manager and unavailable-feature diagnostic; oracle_exit=0, candidate_exit=1, stdout_delta=59, stderr_exact=False. | `stdout_delta=59` |
| `phys_fn_000402` | Omitted the NpScene::releaseFluid forwarding call in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the retained empty manager and missing release warning; both exits=0, stdout_delta=4, stderr_exact=True. | `stdout_delta=4` |
| `phys_fn_000404` | Omitted the unavailable-feature warning from NpScene::createImplicitMesh in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the missing diagnostic; both exits=0, stdout_delta=2, stderr_exact=True. | `stdout_delta=2` |
| `phys_fn_000406` | Omitted the unavailable-feature warning from NpScene::releaseImplicitMesh in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the missing diagnostic; both exits=0, stdout_delta=2, stderr_exact=True. | `stdout_delta=2` |
| `phys_fn_000408` | Changed NpScene::getNbImplicitMeshes to return one instead of zero in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the wrong count; both exits=0, stdout_delta=2, stderr_exact=True. | `stdout_delta=2` |
| `phys_fn_000410` | Omitted the unavailable-feature warning from NpScene::getImplicitMeshes in a throwaway git archive of HEAD. NxPhysicsSimulationTests caught the missing diagnostic; both exits=0, stdout_delta=2, stderr_exact=True. | `stdout_delta=2` |

The clean and mutant runs used the pinned oracle at `D:\FlamingEnt__\Unreal_3` and staged separate oracle/candidate pairs. Build and transcript logs are retained locally under `build/fluid-archive-*`; the archive-source clean run is `build/fluid-archive-restored.log`.

This closes only the exercised wrapper behavior: the pinned binary reports the fluid backend and implicit meshes unavailable. Real fluid objects, emitter/manager extension behavior, and callback interactions remain open full-DLL work.
