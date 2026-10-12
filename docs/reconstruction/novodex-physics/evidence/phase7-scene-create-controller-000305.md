# `NpScene::createController` forwarding — `phys_fn_000305`

The 8-byte entry forwards `NxScene::createController` to the Scene-owned controller list when the internal Scene exists, and returns null otherwise. The registered `NxPhysicsSimulationTests` fixture already covers rejected-type behavior, two valid controller creations, generated actor counts and positions, movement, and releases. The restored staged-pair differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

For mutation proof, a throwaway `git archive` changed `NpScene::createController` to return null. The candidate lost both successful controllers, retained zero actors, and failed the fixture (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=51`). Restoring the forwarding call restored the exact control. Raw transcripts: [restored control](raw/phase7-scene-create-controller-000305/restored-control.log) and [archive mutant](raw/phase7-scene-create-controller-000305/archive-mutant-differential.log).

This closes only `phys_fn_000305`; the controller constructors, destructors, and remaining virtual interface still need reconstruction. The Phase 7 ledger now records 158 closed / 403 deferred functions.
