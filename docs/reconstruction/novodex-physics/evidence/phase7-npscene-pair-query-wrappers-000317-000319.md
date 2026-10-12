# NpScene pair-query wrapper falsification

Phase 7 rows `phys_fn_000317` and `phys_fn_000319` map to the existing
`NpScene::getNbPairs` and `NpScene::getPairFlagArray` implementations. The
registered `NxPhysicsPairFlagTests` fixture reads both values for generated,
configured, and released pair sets.

The clean staged-pair control matches the pinned oracle exactly. Replacing the
pair count with zero is rejected (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=6`, exact stderr). Returning false without copying the pair-flag
array is rejected by the same fixture (`stdout_delta=4`, with both processes
exiting zero and exact stderr). After restoring both source lines and
rebuilding, the control returns to `stdout_delta=0` with exact stderr.

The fresh full Phase 7 gate passes at `1,704/1,704` required assertions. Raw
mutation, restored-control, and gate transcripts are archived under
[`evidence/raw/phase7-npscene-pair-query-wrappers-000317-000319-20261012/`](raw/phase7-npscene-pair-query-wrappers-000317-000319-20261012/).
