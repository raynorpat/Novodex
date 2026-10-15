# Phase 2 row `phys_fn_000287`: NpScene internal Scene accessor

IDA identifies RVA `0x0000c3f0` as a `thiscall` getter returning the pointer at `NpScene+0x24`. The candidate now represents it as the noinline `NpScene::scene()` method in `Physics/src/NpScene.cpp`; public `releaseScene` uses this accessor.

The registered `NxPhysicsSDKSceneLookupTests` differential creates two Scenes, releases the first, and checks the surviving public Scene. A mutation changing the getter to return `this` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=7`; the candidate reports `count=2`, `survivor=0`, and `row000287 accessor=0`). The byte-restored implementation matches the shipped DLL exactly (`stdout_delta=0`, exact stderr).

The Phase 2 gate passes at 18/18 coverage assertions, and the adjacent Phase 3 gate passes at 984/984. The row is closed in the Phase 2 ledger. The focused pair, mutation/restoration build logs, and DLL identities are retained in [the raw evidence directory](raw/phase2-row-000287-20261015/manifest.json).
