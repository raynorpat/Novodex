# Fresh Phase 5 gate on current `main` (2026-10-17)

A clean Win32 Release build in `build/phase5-current-goal` and its isolated oracle/candidate pair passed the Phase 5 gate from source commit `c01fd7e8`.

The gate evaluated 2,850 coverage assertions against a required floor of 2,850 and ended with `phase_gate=5 status=pass`. The candidate DLL SHA-256 was `db9d22e89c7c44d5d1e9047008c53b3db704e66d1114632b1b2cb48beb6c1f27`; the paired Foundation DLL was `e7a72a921f8c6c94332b4a50fe92388710d6641cc49904a7f6057799c8329fff`. The oracle DLL remained the pinned `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The invocation was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -BuildRoot D:\github\Novodex\build\phase5-current-goal -PairsRoot D:\github\Novodex\build\phase5-current-goal-pairs
```

The complete runner transcript and SHA-256 manifest are retained under [raw/phase5-current-main-fresh-build-20261017](raw/phase5-current-main-fresh-build-20261017/).