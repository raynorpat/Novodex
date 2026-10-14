# Clean mainline Phase 7 verification — 2026-10-14

The complete registered Phase 7 gate passed from a fresh managed worktree at source commit `81b9cf395198e3fae78d830cb6d3e136666decc8`, with no working-tree modifications. The gate used the CMake Win32 Release build and passed the inventory, pinned public-header, vendored-source, build, oracle-only proof, staged-pair differential, and coverage-floor checks. It evaluated all `1,979/1,979` Phase 7 coverage assertions and ended with `phase_gate=7 status=pass`.

The staged oracle `NxPhysics.dll` SHA-256 was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The freshly built candidate SHA-256 was `aa36d8ad53fa3aac316fd80c8b26474444f1246fb8ee6c5197a39335227de860`, and the gate confirmed that the staged candidate matched its build output. The oracle-only `NxPhysicsObjectLayoutTests` proof also passed against its pinned fixture source.

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 7 -RepoRoot <clean-worktree> -BuildRoot <clean-worktree>\build\phase7-clean
```

The complete gate transcript is retained in `evidence/raw/phase7-mainline-clean-20261014/phase7-gate.log.gz` (SHA-256 `484bc8a2d3abd8f8f08d9271f045c1b977066ca77d62eb1e85e15cdcc7ed5ff7`). This verifies the committed mainline source only. In the user's primary checkout, `tests/PhysicsObjectLayoutTests.cpp` has a separate local edit that changes its pinned source hash; it was preserved and not staged.
