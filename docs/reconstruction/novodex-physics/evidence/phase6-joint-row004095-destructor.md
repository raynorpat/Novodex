# Phase 6 closure: joint destructor (`phys_fn_004095`)

`phys_fn_004095` at RVA `0x00095e20` is `Joint::~Joint` in `Physics/src/core/Joint.cpp`. Its destructor clears the SDK pointer binding, removes the joint from its scene when still attached, and purges owned limit-plane allocations.

The isolated Win32 Release build was configured from source revision `36ba492d` at `D:\github\Novodex\build\phase6-joint-004095-20261009`. The registered `NxPhysicsCoreDumpTests` target first passed against the pinned pair with both exits zero, `stdout_delta=0`, and exact stderr. The target includes joints with limit planes and reports outstanding allocator allocations after scene release.

For row-specific falsification of `phys_fn_004095`, only the `purgeLimitPlanes()` call in `Joint::~Joint` was omitted. The rebuilt mutation DLL (`8c0641e8e3f3495327f0c9032fb079b91a8ff318833e7eb59cbaff35a7e880b4`) was rejected by the same staged-pair test: oracle and candidate both exited zero, but the released-scene allocation count was 14 for the oracle and 17 for the candidate, producing `stdout_delta=2` with exact stderr. The restored source was rebuilt as candidate DLL `513ef5f352a01d967ea72ffbee840eae1c203dbf27b5b2a821bce0e1f8cf60e2`; the allocation counts returned to 14/14 and the differential passed with `stdout_delta=0`, both exits zero, and exact stderr. The unmodified initial candidate was also exact (`stdout_delta=0`, DLL SHA-256 `8efe7d2f8c82cc013bd0bfd68a745357504a06afe4e25c0a293f504347581a73`). The oracle Physics DLL is pinned at SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Commands used:

```powershell
cmake -S C:\Users\raynorpat\.codex\worktrees\nxphysics-step-closure\Novodex -B D:\github\Novodex\build\phase6-joint-004095-20261009 -G "Visual Studio 18 2026" -A Win32
cmake --build D:\github\Novodex\build\phase6-joint-004095-20261009 --config Release --target NxPhysics NxPhysicsCoreDumpTests --parallel 8
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\raynorpat\.codex\worktrees\nxphysics-step-closure\Novodex\docs\reconstruction\novodex-physics\tools\run_differential.ps1 -Targets NxPhysicsCoreDumpTests -RepoRoot C:\Users\raynorpat\.codex\worktrees\nxphysics-step-closure\Novodex -BuildRoot D:\github\Novodex\build\phase6-joint-004095-20261009 -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot D:\github\Novodex\build\phase6-joint-004095-20261009\pairs
```

Raw runner logs: `D:\github\Novodex\build\phase6-joint-004095-20261009\baseline.log`, `mutation.log`, and `restored.log`. The mutation existed only in the isolated worktree and was removed before the restored build.
