# Phase 7 closure: actor visualization (`phys_fn_000020`)

`phys_fn_000020` at RVA `0x00001560` implements
`NxActorVisualRecord::visualize` in `Physics/src/SceneVisualize.cpp`. The
registered `NxPhysicsSceneVisualizeTests` fixture records actor-axis drawing
and body visualization calls across its staged visualization parameter sets.

In a throwaway archive of mainline `40a5ddcc`, replacing the function body
with a no-op was detected by the registered Phase 7 staged-pair differential
(`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=916`,
`stderr_exact=True`). Restoring and rebuilding the row returned an exact
control (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).
