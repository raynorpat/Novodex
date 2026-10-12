# NpScene legacy run wrapper mutation proofs

Phase 7 rows phys_fn_000335 (startRun), phys_fn_000336 (finishRun), and phys_fn_000342 (runFor) are independently falsified through the registered NxPhysicsSimulationTests staged-pair differential. The fixture creates a zero-gravity moving sphere, sets an explicit one-step timing triplet, records its initial state, then records the actor after the legacy wrappers.

Changing startRun(0.01f) to simulate 0.011f changes the post-start/finish position from 3c23d70a to 3c343958 (stdout_delta=4). Making finishRun fetch nonblocking leaves the sampled actor at zero (stdout_delta=4). Changing only runFor's 0.02f simulation interval to 0.021f changes the final position from 3cf5c28f to 3cfdf3b6 (stdout_delta=2). For each mutation both oracle and candidate processes exit zero and stderr is exact.

After restoring source and rebuilding, both recorded positions match the oracle exactly (stdout_delta=0, both exits zero, exact stderr). Fresh Phase 5, 6, and 7 gates pass at 2,676/2,676, 1,607/1,607, and 1,707/1,707 assertions. The gate and raw mutation/control transcripts are archived under evidence/raw/phase7-npscene-legacy-run-wrappers-000335-000336-000342-20261012/.
