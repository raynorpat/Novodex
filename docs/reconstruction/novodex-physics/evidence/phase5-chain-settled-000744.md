# Phase 5 — group-sleep chain helper (`phys_fn_000744`)

The registered `NxPhysicsObjectLayoutTests` actor-layout fixture drives the helper through the `actorsm5` family. It covers a compressed two-hop chain with all sleep counters zero, a member with a positive counter, and the actor's null-record case. The baseline oracle/candidate run matches at digest `75b57124`, with zero layout mismatches.

Changing the helper's comparison from `v > 0.0f` to `v >= 0.0f` makes a zero-valued sleeping member count as awake. The mutant is rejected: `actorsm5 candidate ok=0`, digest `ebe08234`, and `layout candidate mismatches=1`. Restoring `ObjectModel.cpp`, rebuilding the target, and rerunning returns digest `75b57124`, zero mismatches, and `layout result=differential-pass`. Logs: `build/phase5-chain-settled-000744-baseline-direct.log`, `build/phase5-chain-settled-000744-mutant.log`, and `build/phase5-chain-settled-000744-restored.log`.

This closes the direct helper row `phys_fn_000744`; Phase 5 now records 127 closed and 78 deferred functions. The production implementation and public Physics headers are unchanged.


Fresh Win32 Release Phase 5 gate passes all 18 staged targets and 2,311/2,311 coverage assertions with the restored source (`build/phase5-chain-settled-000744-full-gate.log`).
