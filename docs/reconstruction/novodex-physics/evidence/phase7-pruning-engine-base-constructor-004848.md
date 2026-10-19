# Phase 7: pruning-engine base constructor (`phys_fn_004848`)

IDA at RVA `0x000b4fe0` shows the base constructor clears its +0 vtable placeholder and four pruner pointers at `+0x1c..+0x28`, writes the empty-world min/max sentinels at `+0x04..+0x18`, and returns `this`. The candidate now maps this row to `NxScenePruningEngineBase::ConstructPruningEngineBase`; the enclosing `phys_fn_001980` calls it before initializing the rest of the engine. The non-x87 path retains its platform-specific pruning-collection construction.

`NxPhysicsSceneRaycastTests` checks the four null slots and six bound words immediately after Scene creation. The focused staged oracle/candidate differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Changing the candidate `+0x1c` store to 1 is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2813`); restoring the store restores exact output.

Fresh Phase 4/5/7 gates pass at 563/563, 2,969/2,969, and 3,107/3,107 coverage assertions. The focused baseline, mutation, restored, and gate transcripts are SHA-256 pinned in `evidence/raw/phase7-pruning-engine-base-constructor-004848-20261018/`. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows; public-header verification passes all 80 headers. Full DLL reconstruction remains open.
