# Task M6.27 brief — Joint island linking (`phys_fn_000762`)

Base: `68a150c5`

The registered Phase 7 `NxPhysicsSimulationTests` path reaches row 000762 during joint-break handling. An immediate-return mutation suppresses the expected joint-break notification (`candidate_exit=1`, `stdout_delta=876`); restored source returns exact output.

Phase 7 now records 314 closed / 246 deferred rows. No public header or production source changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-island-link-000762.md`.

Ruling for M6.28: choose the next reconstructed-but-unfalsified row in the articulation/island chain based on the current Phase 7 ledger and test it against a registered public simulation before adding a private direct probe.
