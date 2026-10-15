# Phase 4 Pruner process-pool replacement: `phys_fn_005440`

`NxPrunerProcessPool::replace` at RVA `0x000ef330` frees the current arrays and adopts a new set during capacity growth; the same routine releases the arrays at process-pool teardown. The registered `NxPhysicsSceneRaycastTests` fixture creates overlapping Scenes, grows the two-entry pool, releases a Scene, reuses generation-tagged handles, and finally releases the SDK.

For row-specific falsification, an unconditional return was inserted at `replace` entry and the Win32 Release DLL rebuilt. The oracle passed. The mutant candidate retained the expected static/dynamic registration and generation-reuse markers, then crashed during final pool cleanup (`candidate_exit=-1073741819`, `stdout_delta=13`, `stderr_exact=True`). Restoring the source, touching it to force recompilation, and rebuilding returned the focused staged pair to exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The raw mutant and restored build/differential logs and their hashes are in [the evidence directory](raw/phase4-pruner-replace-005440-20261015/). Pool growth at larger capacities, allocation failure, and wraparound remain open beyond this helper's exercised lifecycle.
