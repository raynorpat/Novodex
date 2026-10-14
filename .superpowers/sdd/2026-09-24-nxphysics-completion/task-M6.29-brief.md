# Task M6.29 brief — Body CCD step update (`phys_fn_000770`)

Base: `321577c4`

A row-only immediate-return mutation is rejected by the registered simulation (`candidate_exit=1`, `stdout_delta=7134`) and CCD (`candidate_exit=1`, `stdout_delta=5`, out-of-range response velocity). Restoring `BodyStep.cpp` byte-for-byte yields exact controls for both targets.

Phase 7 moves to 317 closed / 243 deferred. No public headers changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-body-ccd-update-000770.md`.

Ruling for M6.30: select the next reconstructed-but-unfalsified row from the current Phase 7 ledger, prioritizing public simulation paths with existing target coverage.
