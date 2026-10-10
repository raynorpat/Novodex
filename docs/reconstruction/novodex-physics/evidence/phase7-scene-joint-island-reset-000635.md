# Phase 7 Scene substep island-reset row `phys_fn_000635`

`phys_fn_000635` (`NxPhysics.dll+0x127e0`, 176 bytes) runs after the per-substep pair refresh and before active roots are collected. The oracle decompilation shows three ordered passes: for each Scene joint, select the first non-null body pointer at joint `+0x08` or `+0x0c`, call `phys_fn_000762` when present, then call `phys_fn_000720` on the selected record; reset Scene `+0x590` to the joint-list start at `+0x58c`; and call `phys_fn_000764` for each body record in `[+0x56c,+0x570)`. The sole direct caller is `phys_fn_000659`.

The registered `NxPhysicsSimulationTests` staged-pair differential rejects a mutation that omits the joint/body island-reset passes while retaining the Scene cursor assignment. In the active broadphase fixtures the candidate then accesses stale body island links and exits `-1073741819`; the pinned oracle exits 0 and the measured `stdout_delta` is 3728. After restoring the passes and rebuilding, both sides exit 0 with `stdout_delta=0` and exact stderr. The mutation is detected before the later fixed-, revolute-, and distance-joint trajectory cases; the static listing provides the full ordered row behavior. The separately open downstream `phys_fn_004172` island rebuild is not closed by this row.

```powershell
cmake --build .\build --config Release --target NxPhysics
.\docs\reconstruction\novodex-physics\tools\run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot . -BuildRoot .\build -OracleRoot 'D:\FlamingEnt__\Unreal_3' -PairsRoot 'D:\FlamingEnt__\novodex-analysis\pairs'
```

The restored candidate was staged at SHA-256 `dc8fd231964a6821c06bee88cefa9e84bffeb8bf9641dd9c9cb14dda29889841`; the pinned oracle is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
