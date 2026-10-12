# NpScene timing wrapper mutation proofs

Phase 7 rows `phys_fn_000338` (`NpScene::setTiming`) and `phys_fn_000340` (`NpScene::getTiming`) are now independently falsified through the registered `NxPhysicsSimulationTests` staged-pair differential. The fixture observes the default triplet `3c888889.8.0` and the changed triplet `3e000000.4.1`.

For `setTiming`, a parameter-focused mutation skipped only the `(0.125f, 4, NX_TIMESTEP_VARIABLE)` Scene forward. Oracle and candidate exited zero with exact stderr; the changed observation reverted to the initial triplet and the differential reported `stdout_delta=2`. For `getTiming`, the Scene output was replaced with deterministic zero values; both timing observations changed, producing `stdout_delta=4`, again with both processes exiting zero and exact stderr.

After restoring source and rebuilding, both timing observations matched and the clean differential returned `stdout_delta=0`, with oracle and candidate exit zero and exact stderr. The fresh full Phase 7 gate passes at `1,704/1,704` required assertions. Mutation, restored-control, and full-gate logs are archived under [`evidence/raw/phase7-npscene-timing-wrappers-000338-000340-20261012/`](raw/phase7-npscene-timing-wrappers-000338-000340-20261012/).
