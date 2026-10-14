# Joint island linking (`phys_fn_000762`)

Date: 2026-10-14

Row: `phys_fn_000762` at RVA `0x000177c0`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential

Row 000762 refreshes wake state, compresses and releases the prior island roots, increments the root joint count, links the joint into the selected root's joint lists, and dispatches the body pair through row 000716. The existing registered simulation reaches this joint-island path during joint break handling.

In a Win32 Release build, replacing only `Row000762Fixture::row000762` with an immediate return is detected: the oracle exits 0 and reports the expected single joint-break notification, while the candidate exits 1 because the notification count is missing. The staged transcript differs by 876 bytes (`stderr_exact=False` because the candidate emits the test's failure diagnostic).

Restoring `Physics/src/Island.cpp` byte-for-byte returns an exact control (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The restored source SHA-256 is `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`; no production source or public header changed. Raw mutation and restored builds, staged-pair transcripts, source hashes, and the patch are preserved under `evidence/raw/phase7-joint-island-link-000762-20261014/`.

The full Phase 7 gate passes with `coverage_assertions_evaluated=1801 floor=1801` and `phase_gate=7 status=pass`. Its transcript is `evidence/raw/phase7-joint-island-link-000762-20261014/phase7-gate.log.gz`.
