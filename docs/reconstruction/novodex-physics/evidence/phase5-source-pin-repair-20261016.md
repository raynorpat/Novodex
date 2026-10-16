# Phase 5 source-pin repair and gate rerun

The object-layout fixture edit was integrated on `main` in commit
`fa53527c` (`test: correct object layout descriptor fixture`), together with
the updated `NxPhysicsObjectLayoutTests` oracle-only source hash in
`evidence/oracle-only-baselines.json`. The source hash currently matches that
pin (`cedffb3229e4aaecbc7e20edd5cb929903a7ba7ad9cbe2a2afcf4f2cf3ee25c8`).

On 2026-10-16, I reran the full Phase 5 gate from the main checkout:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 `
  -Phase 5 -RepoRoot D:\github\Novodex -BuildRoot D:\github\Novodex\build `
  -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot D:\github\Novodex\build\pairs
```

The Win32 Release configure and build passed. All 25 staged oracle/candidate
differentials passed with zero stdout deltas and exact stderr, the registered
oracle differentials and oracle-only proofs passed, the public header checks
passed for both 80-file roots, inventory validation reported zero unexplained
rows, and the Phase 5 gate finished `status=pass` at 2,802/2,802 coverage
assertions. The object-layout oracle proof matched digest `774d629d`.

This rerun supersedes the current-working-copy source-hash failure recorded in
the 2026-10-15 reproduction note. That historical run correctly caught the
then-unpinned fixture edit; the edit and pin are now both committed.
