# Joint contact suppression owner-list scan (`phys_fn_000750`)

Date: 2026-10-14

Row: `phys_fn_000750` at RVA `0x00016fb0`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential; the same shared target is also registered in Phases 5 and 6.

The shared direct probe resolves the candidate through its staged map and the oracle at its pinned RVA. It builds aligned scratch body and joint records and checks whether a joint in one body's joint list disables contact. The reverse direction is separately checked in the same target. The baseline result is `owner_scan=1.0 fallback_scan=1.0` on both DLLs, with an exact transcript.

The direct probe verifies both contact-allow states for a joint present only on the receiver body list. A row-only owner-list scan mutation changes owner_scan from 1.0 to 0.0; candidate_exit=1 and stdout_delta=4000. The restored differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The restored `Physics/src/ContactPairManager.cpp` SHA-256 is `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`. No public header or production source changed. Raw baseline, mutation, restored-control logs, hashes, and isolated patch are under `evidence/raw/phase7-joint-contact-filter-000750-20261014/`.

The shared-target gates pass in all three phases: Phase 5 at 2,728/2,728, Phase 6 at 1,653/1,653, and Phase 7 at 1,802/1,802 coverage assertions. Their transcripts are preserved in this row's raw evidence folder.
