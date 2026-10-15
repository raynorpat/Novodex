# Phase 4 SDK process-pool release shim: `phys_fn_005446`

`nxOpcodeReleasePool` at RVA `0x000ef660` forwards SDK teardown to `nxPrunerProcessPoolDestroy`, releasing the process pool, its four arrays, and the global pointer. The registered `NxPhysicsSceneAllocatorTests` fixture records allocator events at `sdk_release`.

For row-specific falsification, the shim was changed to a no-op and the Win32 Release DLL rebuilt. The oracle reports five SDK-allocator frees during SDK release, with sizes `4,4,4,8,1c`; the mutant candidate reports none (`b_free_sizes=none`). Both processes exit 0, stderr matches, and the staged differential reports `stdout_delta=6`. Restoring the shim, forcing recompilation, and rebuilding restores the five frees and exact output (`stdout_delta=0`, exact stderr).

This differential target is registered in Phases 5 and 7; Phase 5 must pass for the Phase 4 ledger to accept this discharge. Raw transcripts and hashes are in [the evidence directory](raw/phase4-pruner-release-pool-005446-20261015/).
