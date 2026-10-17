# Collision-pruner visualization dispatch (`phys_fn_001969`)

IDA at RVA `0x0004c070` identifies the shared pruner-visualization dispatcher. With a non-null pruning engine it routes enabled visualization to the static pruner, bounded dynamic pruner, free pruner, and type-3 static-base pruner. The static and type-3 trees are lazily rebuilt before traversal; the bounded dynamic path uses its cached visualization tree, and the free path submits world-box AABBs. The SAP endpoint callback remains in the enclosing `phys_fn_001978` implementation.

The candidate dispatch is now isolated as `nxSceneVisualizePrunerDispatch` in `Physics/src/SceneVisualize.cpp`, called from `nxSceneVisualizeCollisionPruners`. The public scene fixture prints separate static and bounded-dynamic dispatch markers and requires 36 red AABB lines from each path. The existing free-pruner and type-3 fixtures continue to exercise the other two branches. Both new markers are registered in Phase 7.

The restored oracle and candidate report 36 lines for both new markers and produce identical output. Disabling the dispatch call is rejected by the registered staged-pair differential (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=1976`). Rebuilding the restored source returns to an exact differential. The Phase 7 coverage floor now includes both markers. Public headers are unchanged.

Raw focused and full-gate logs are stored in `evidence/raw/phase7-pruner-dispatch-001969-20261017/`.
