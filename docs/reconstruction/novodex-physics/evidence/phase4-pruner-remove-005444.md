# Phase 4 Pruner process-pool removal: `phys_fn_005444`

`NxPrunerProcessPool::remove` at RVA `0x000ef5c0` retires a generation-tagged registration, packs the dense object array, and returns the slot to the free set. The registered `NxPhysicsSceneRaycastTests` target destroys a Scene with static and dynamic pruners, then creates replacement pruners and observes the actual DLL-owned handle words.

For row-specific falsification, an unconditional return was inserted at function entry and the Win32 Release DLL rebuilt. The oracle reused indices 2 and 3 with generation 1 (`static=00010002.00000001`, `dynamic=00010003.00000001`). The mutant candidate retained the old handles and allocated indices 4 and 5 (`static=00000004.00000001`, `dynamic=00000005.00000001`), failing the assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). Restoring the source, forcing recompilation, and rebuilding returned the staged pair to exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The build and differential transcripts with hashes are in [the raw evidence directory](raw/phase4-pruner-remove-005444-20261015/). Allocation failure and generation wraparound remain open edge cases.
