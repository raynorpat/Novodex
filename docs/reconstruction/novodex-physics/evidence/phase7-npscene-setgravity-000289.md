# `NpScene::setGravity` mutation proof — `phys_fn_000289`

`phys_fn_000289` (`0x0000c400`, 84 bytes) is the `NpScene::setGravity` write-locked wrapper. The registered `NxPhysicsSimulationTests` staged-pair fixture observes the initial gravity, sets a changed vector through the public `NxScene` interface, reads it back, and restores the descriptor value.

The clean Win32 Release candidate built from `main` matches the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). In a throwaway `git archive` of `main`, replacing `mScene->setGravity(gravity)` with a no-op causes the same registered differential to report `stdout_delta=2` while both processes still exit zero and stderr remains exact. Restoring the source and rebuilding returns to `stdout_delta=0`, exact stderr. The archive's restored `Physics/src/NpScene.cpp` byte hash matches the mainline source (`0a0907f2e1b2849f1895f4904bc77d23a031cf5fec9d43b9f40ff7706e7a908d`).

The baseline, mutant, and restored-control runner transcripts are retained, with their full child output and staged module identities, under [`evidence/raw/phase7-npscene-setgravity-000289-20261012/`](raw/phase7-npscene-setgravity-000289-20261012/). This closes only `phys_fn_000289`; the rest of the Phase 7 ledger and full-image audit remain open.
