# Phase 7 body integration row `phys_fn_000726`

`NxSceneInternal::row000610` calls `Row000726Fixture::row000726` for every body in the active sleep groups. The registered `NxPhysicsSimulationTests` differential exercises this path through the public scene simulation API.

For the negative control, the `row000726` call in `Physics/src/Scene.cpp` was rebuilt with a zero timestep. The staged-pair run caught the change: oracle exit 0, candidate exit 1, `stdout_delta=6845`, with the candidate reporting that the expected joint-break callback count was missing. The exact source blob was restored, `NxPhysics` was rebuilt, and the clean staged-pair run returned oracle exit 0, candidate exit 0, `stdout_delta=0`, `stderr_exact=True`.

Commands:

```powershell
cmake --build build --config Release --target NxPhysics --parallel 8
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot (Get-Location).Path -BuildRoot (Join-Path (Get-Location) 'build') -OracleRoot 'D:\FlamingEnt__\Unreal_3' -PairsRoot 'D:\FlamingEnt__\novodex-analysis\pairs'
```

This closes the active-body integration row’s dynamic proof. It does not close the remaining body-step rows, all Phase 7, or the DLL reconstruction.
