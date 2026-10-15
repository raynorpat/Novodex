# Phase 4 Pruner process-pool constructor: `phys_fn_005438`

`NxPrunerProcessPool::NxPrunerProcessPool` at RVA `0x000ef270` initializes the persistent registration pool with capacity two, clears its counts, allocates the parallel object/forward/reverse/generation arrays, and initializes their sentinel and generation values. The registered `NxPhysicsSceneRaycastTests` target reaches the constructor on first Scene Pruner registration, then grows and uses the pool through destruction and SDK release.

For row-specific falsification, an unconditional return was inserted at constructor entry and the Win32 Release DLL rebuilt. Initial registration and handle-reuse output remained plausible, but the candidate crashed during SDK teardown (`candidate_exit=-1073741819`, `stdout_delta=2804`, `stderr_exact=True`); the oracle exited 0. Restoring the source, forcing recompilation, and rebuilding returned the focused staged pair to exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

Mutation/restored transcripts and hashes are in [the raw evidence directory](raw/phase4-pruner-pool-constructor-005438-20261015/). Allocation failure and large-capacity boundary behavior remain open.
