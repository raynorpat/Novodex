# Task M6.28 brief — Joint contact suppression scans (`phys_fn_000750`, `000752`)

Base: `4e526410`

A direct probe was added to the already-registered `NxPhysicsSimulationTests` target. It resolves each DLL's row 000750 entry and uses aligned scratch body/joint records to test both contact-suppression list directions. Mutating only the owner scan changes `owner_scan` from 1.0 to 0.0; mutating only the fallback scan changes `fallback_scan` from 1.0 to 0.0. Both staged-pair runs reject the mutant (`candidate_exit=1`, `stdout_delta=4000`); restored controls are exact.

Rows 000750 and 000752 are now individually falsified. Phase 7 moves to 316 closed / 244 deferred. Coverage floors increase by one in each registering phase: Phase 5 2,728, Phase 6 1,653, Phase 7 1,802. No production source or public header changed.

Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-contact-filter-000750.md` and `000752.md`.
