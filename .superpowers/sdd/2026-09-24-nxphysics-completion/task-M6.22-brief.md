# Task M6.22 brief — island body-chain solver traversal (`phys_fn_000730`)

Base: `ae609b4f`

The registered Phase 7 `NxPhysicsSimulationTests` friction case rejects an immediate-return mutation of `Row000730Fixture::row000730` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`). The archive source was restored byte-for-byte and MSBuild forced to recompile it; the restored differential is exact (`stdout_delta=0`, `stderr_exact=True`).

Remaining: retain raw logs and patch, run tooling/inventory/header checks, merge to main, and pass the post-merge full Phase 7 gate. Row 000720 stays open.
