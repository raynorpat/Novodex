# Static and selected pruner object counts (`phys_fn_001957`, `phys_fn_001960`)

IDA confirms `phys_fn_001957` returns zero for a null static pruner and otherwise sums the section-one and section-two counts (`mNbObjects[1] + mNbObjects[2]`). `phys_fn_001960` selects the pruner from the engine selector at +0x70 and returns the same sum, or zero for a null slot. Both are no-argument x86 thiscall members. Implementations are in `Physics/src/opcode/IcePruningEngine.cpp` and candidate methods are resolved through `build/Release/NxPhysics.map`.

The registered `NxPhysicsSceneRaycastTests` fixture reports static count 8 (3 + 5) and selected slot-2 count 18 (7 + 11) on oracle and candidate. Mutating the section-two operand to section zero is independently detected for each row: row001957 reports 3/8 and row001960 reports 7/18; each mutation exits 1 and leaves the other row exact. Restored controls are exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

Fresh Phase 2, 4, 5, and 7 gates pass at 18/18, 555/555, 2,867/2,867, and 3,006/3,006 registered assertions. The Phase 2 ledger keeps these census-owned rows in Phase 2 and records their discharge through Phase 7, one of their driving phases. Phase 2 now records 90 closed / 53 deferred. Public headers are unchanged.

Compressed gates, mutation transcripts, map addresses, and hashes are in [raw/phase2-pruner-object-count-001957-001960-20261017](raw/phase2-pruner-object-count-001957-001960-20261017/).
