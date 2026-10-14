# Task M6.22 brief — island body-chain solver traversal (`phys_fn_000730`)

Base: `ae609b4f`

The registered Phase 7 `NxPhysicsSimulationTests` friction case rejects an immediate-return mutation of `Row000730Fixture::row000730` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`). The archive source was restored byte-for-byte and MSBuild forced to recompile it; the restored differential is exact (`stdout_delta=0`, `stderr_exact=True`).

The packet is committed to main as `7f5a0d6b`. Tool tests pass (831 tests, 742 subtests), inventory and raw evidence checksums validate, and the post-merge Phase 7 gate passes with 1,793/1,793 coverage assertions. The full gate transcript is compressed in the raw evidence folder. Row 000720 remains open.
