# NpScene gravity getter wrapper falsification

Phase 7 row `phys_fn_000291` is the `NpScene::getGravity` wrapper. The
registered `NxPhysicsSimulationTests` fixture reads gravity on a fresh scene,
changes it through the public setter, reads it again, and restores the scene
descriptor value.

The clean staged-pair control matches the pinned oracle exactly. Replacing the
Scene read with a zero-vector mutation changes the transcript
(`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=4`, exact stderr). Restoring
the source and rebuilding returns the control to `stdout_delta=0` with exact
stderr. The fresh full Phase 7 gate passes at `1,704/1,704` required
assertions. Raw mutation, restored-control, and gate transcripts are archived
under
[`evidence/raw/phase7-npscene-getgravity-000291-20261012/`](raw/phase7-npscene-getgravity-000291-20261012/).
