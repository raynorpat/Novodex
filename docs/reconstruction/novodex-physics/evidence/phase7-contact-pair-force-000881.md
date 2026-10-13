# Phase 7 contact-pair force aggregation (`phys_fn_000881`)

`NxActorPair::row000881` sums the embedded friction patch and any additional vector patches into `NxContactPair::sumNormalForce` and `sumFrictionForce`. The registered `NxPhysicsSimulationTests` sliding-sphere fixture now records those callback fields as raw float bits. Across 59 callbacks, the pinned oracle reports summed friction `3d5ae33a.00000000.00000000`.

A no-op mutation replaced the row body and was rebuilt into the candidate DLL. The sliding fixture still completed, but the callback aggregate changed; the staged pair differential measured `stdout_delta=30` with both child processes exiting zero. The mutant candidate DLL SHA-256 was `0f436f5e6c2fe2c404a56e5858d98dcf5b44d0faa115bd306e2e17348ca57958`.

After restoring the implementation and rebuilding, oracle and candidate force lines matched exactly and the staged differential returned `stdout_delta=0` with exact stderr. The restored candidate DLL SHA-256 was `d17d8fefdac22a9c5efa1cbcbb256b5108e63de47fec61187f96a117da22fae95`. Raw logs are in [raw evidence](raw/phase7-contact-pair-force-000881/).

Fresh Phase 5 gate: pass at 2,701/2,701 assertions. See [phase5-gate.log.gz](raw/phase7-contact-pair-force-000881/phase5-gate.log.gz).

Fresh Phase 6 gate: pass at 1,634/1,634 assertions. See [phase6-gate.log.gz](raw/phase7-contact-pair-force-000881/phase6-gate.log.gz).

Fresh Phase 7 gate: pass at 1,743/1,743 assertions. See [phase7-gate.log.gz](raw/phase7-contact-pair-force-000881/phase7-gate.log.gz). Phase 7 now closes 247 functions and defers 313; the full-DLL reconstruction remains in progress.
