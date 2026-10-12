# `NpScene::releaseController` forwarding — `phys_fn_000307`

The 8-byte entry forwards controller release to the Scene-owned controller list when the internal Scene exists. `NxPhysicsSimulationTests` releases a non-head controller and then the remaining head, checking the list link, generated actor count, and error count. The restored staged-pair differential matches exactly.

A throwaway `git archive` removed the forwarding call. The candidate retained a non-null stale-next link after the first release and failed (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=2`, exact stderr). Restoring the forward returned the exact control. Raw transcripts: [restored control](raw/phase7-scene-release-controller-000307/restored-control.log) and [archive mutant](raw/phase7-scene-release-controller-000307/archive-mutant-differential.log).

This closes only `phys_fn_000307`; controller destruction and the remaining controller interface are still open. Phase 7 now records 159 closed / 402 deferred functions.
