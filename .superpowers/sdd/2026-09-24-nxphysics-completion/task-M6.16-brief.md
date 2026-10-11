# Task 16 — NxScene::checkResults status-mask branch

Plan contract: M6 requires correct checkResults status and blocking/nonblocking behavior. Stable row: phys_fn_000396 at RVA 0x0000d710.

Oracle evidence: IDA pseudocode checks (status & 1) == 0 and immediately returns true; only requests containing bit 0 inspect the completion event using zero or infinite timeout according to block.

Implementation contract: keep public Physics headers unchanged. Extend tests/PhysicsSimulationTests.cpp with an assertion that checkResults(static_cast<NxSimulationStatus>(0), false) returns true through the staged public NxScene interface. The focused oracle/candidate target must match. A candidate-only mutation that makes this status-mask early return false must fail the target. Restore the source and verify the exact control.

Closure: register the assertion in the existing Phase 6/7 simulation target (no new target or coverage-count line required); add row-specific evidence and update the Phase 7 closure ledger, inventory, work units/backlog, and project ledger. Keep Phase 8 terminal closure distinct from the intermediate row proof.
