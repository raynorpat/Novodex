# Phase 7 step-body array row `phys_fn_000600`

`phys_fn_000600` (`NxPhysics.dll+0x10fe0`, 122 bytes) maintains the Scene's per-step `JointSupportBody` buffer. It writes the used count at `Scene+0x5b0`, reuses the buffer when `Scene+0x5b4` has enough capacity, otherwise frees the old count-prefixed allocation, requests `count * 0x60 + 4` bytes from the SDK allocator, writes the count header, publishes the payload at `Scene+0x5ac`, and updates capacity. The 0x60-byte stride is consumed by `phys_fn_000611`. The oracle's vector-constructor iterator invokes a no-op element constructor; the candidate also clears record storage after allocation.

The registered `NxPhysicsSimulationTests` staged-pair differential covers active simulation scenes. Its whole-helper mutation omitted the step-body allocation, count header, and capacity update. The oracle exited 0 while the candidate hit access violation `-1073741819`; the differential reported `stdout_delta=3731`. Rebuilding after restoring the helper returned both exits to 0 with `stdout_delta=0` and exact stderr. This falsifies removal of the row's active allocation path; it does not close the remaining M6 scheduler, solver, or trajectory work.

Commands:

```powershell
cmake --build .\build --config Release --target NxPhysics
.\docs\reconstruction\novodex-physics\tools\run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot . -BuildRoot .\build -OracleRoot 'D:\FlamingEnt__\Unreal_3' -PairsRoot 'D:\FlamingEnt__\novodex-analysis\pairs'
```

The restored-control candidate was staged at SHA-256 `b5b85da8ac5b6a56ec9f96193e883ebb8473e41c5ab164d1a8b82f25256f0aa5`; the pinned oracle was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
