# Task M6.26 brief — Dirty island record reset (`phys_fn_000764`)

Base: `d81833a8`

The registered Phase 7 `NxPhysicsSimulationTests` path exercises the per-body dirty-root reset after joint links are refreshed. An immediate-return mutation causes the candidate to access-violate while the oracle exits cleanly (`stdout_delta=3898`, exact stderr). Restoring `Physics/src/Island.cpp` byte-for-byte returns exact output.

No public header or production source changed. The row is recorded as differentially falsified in Phase 7; the full gate remains at 1,801 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-record-reset-000764.md`.

Ruling for M6.27: next test `phys_fn_000762`, the 357-byte joint-island linker. Keep its mutation scoped to row 000762 and first test the existing registered simulation differential before designing a direct probe.
