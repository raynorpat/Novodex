# Phase 7 fluid cell-to-triangle association helper — phys_fn_003535

`phys_fn_003535` at RVA `0x871d0` hashes each signed 16-bit cell coordinate, searches the selected bucket's cell-record range, and, on a match, appends the current mesh and triangle indices at the record's prefix. It increments the association count and returns 1. The private CMake implementation is `nxFluidCollisionAppendCellIndex` in `Physics/src/FluidCollision.cpp`; public headers are unchanged.

`NxPhysicsFluidEmitterAbiTests` compares the pinned oracle row with the candidate symbol resolved from the candidate DLL map. The three cases cover a matching cell, two matches in separate buckets plus a missing cell, and empty input. It compares the complete bucket rows, cell records, association storage, return values, and x86 stack balance. The restored control reports `mismatches=0`.

Changing the candidate append from `triangleIndex` to `triangleIndex + 1` is caught in both nonempty cases (`mismatches=2`); empty input remains unchanged. The restored source SHA-256 is `5bd3d676b735eaa94ec52882cca6a699284eccb4d573e1fd4c137601d52c4835`. Raw mutant and control outputs plus a reproducible patch are retained in `evidence/raw/phase7-fluid-cell-index-003535/`.

The oracle's FindCDCell callback slot is bound to an independent linear lookup over the selected bucket range because the pinned UE3 binaries do not contain `FluidModel.dll`. Production callback integration remains open. The helper's caller rows `phys_fn_003537/003538` and top-level dirty static-collision builder `phys_fn_003505` remain open.

After merging commit `55ddd4a6`, fresh Phase 5 and Phase 7 gates pass at 2,650/2,650 and 1,677/1,677. All 810 reconstruction tooling tests pass, public-header verification passes for all 80 headers, and inventory validation reports 111 closed / 450 deferred with unexplained=0. Candidate DLL SHA-256 is `86d7928544efa2a6c3e2d434524e67f00109ca840ea9e2a524c49791fd97e9b7`; oracle DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Commands and raw-log hashes are retained in `evidence/raw/phase7-fluid-cell-index-003535/phase-gates.txt`.

The test-first run failed as expected because the candidate map did not yet contain 
xFluidCollisionAppendCellIndex; the restored implementation now resolves from the CMake-built DLL and matches the oracle. The red and green transcripts are retained beside the mutation outputs.

