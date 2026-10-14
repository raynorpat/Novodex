# Dirty island record reset (`phys_fn_000764`)

Date: 2026-10-14

Row: `phys_fn_000764` at RVA `0x00017930`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential

This row handles each body record after dirty joint-island links are rebuilt. It resolves the record root, releases and rebuilds a dirty root island object, then tail-dispatches the record reset in row 000722. The existing registered simulation traverses this path during its substep setup.

In a Win32 Release build, replacing only `Row000764Fixture::row000764` with an immediate return is detected by the staged-pair differential: the oracle exits 0 while the candidate access-violates (`candidate_exit=-1073741819`, Windows status `0xc0000005`), the transcript differs by 3,898 bytes, and stderr is exact. This shows the reset/rebuild path affects the simulation before its later output completes.

Restoring `Physics/src/Island.cpp` byte-for-byte returns an exact control (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The restored source SHA-256 is `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`; no production source or public header changed. Mutant and restored builds, staged-pair transcripts, source hashes, and the patch are preserved under `evidence/raw/phase7-island-record-reset-000764-20261014/`.

The full Phase 7 gate also passes after registering this closure: `coverage_assertions_evaluated=1801 floor=1801`, `phase_gate=7 status=pass`. Its compressed transcript is `evidence/raw/phase7-island-record-reset-000764-20261014/phase7-gate.log.gz`.
