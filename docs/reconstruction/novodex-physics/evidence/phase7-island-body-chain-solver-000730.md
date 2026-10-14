# Island body-chain solver traversal (`phys_fn_000730`)

Date: 2026-10-14
Row: `phys_fn_000730` at RVA `0x00016830`
Gate: registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential

`Row000730Fixture::row000730` traverses the sleep-group body chain at +0x1fc and calls `row000728` for each record. The public friction simulation drives this chain.

The clean Win32 Release baseline is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Replacing only row 000730 with an immediate return suppresses the chain traversal; the registered differential rejects the mutant (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`, `stderr_exact=False`).

The archive source was restored byte-for-byte (SHA-256 is recorded in the raw evidence folder) and MSBuild recompiled `Island.cpp`; the restored differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Raw configure/build/differential logs and the exact row-only patch are under `evidence/raw/phase7-island-body-chain-solver-000730-20261014/`. Row 000720 remains open because the current fixture does not expose its dirty-island rebuild branch.

The post-merge full Phase 7 gate passes at 1,793/1,793 coverage assertions. Its compressed transcript is preserved as `evidence/raw/phase7-island-body-chain-solver-000730-20261014/post-merge-phase7-gate.log.gz`.
