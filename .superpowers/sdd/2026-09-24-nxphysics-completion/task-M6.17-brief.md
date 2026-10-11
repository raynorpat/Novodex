# Task 17 — NpScene::isWritable write-lock probe

Plan contract: M6 requires writable-state behavior through public Scene calls. Stable row: phys_fn_000392 at RVA 0x0000d660 (31 bytes).

Oracle evidence: IDA 12.1.2 shows a write-lock try on the scene lock link, false on contention, leave/release after success, and true on success. The existing staged public simulation fixture prints simulation writable=initial 1.

Verification contract: mutate only the successful return to false, rebuild the candidate NxPhysics.dll, and run NxPhysicsSimulationTests against the pinned oracle. The existing transcript must reject the mutation. Restore the original code and verify the full simulation target returns to stdout_delta=0 with exact stderr. Register the row-specific proof in the Phase 7 closure ledger and update the inventory/program/backlog/work units. Keep Phase 8 terminal closure open.
