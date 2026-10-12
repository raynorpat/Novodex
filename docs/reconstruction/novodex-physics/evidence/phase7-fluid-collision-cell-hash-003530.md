# Phase 7 fluid static-collision cell hash — `phys_fn_003530`

Reconstructed RVA `0x86e00` as `nxFluidCollisionAccumulateCellCounts` in `Physics/src/FluidCollision.cpp`. The helper hashes signed-16-bit XYZ coordinates into the 12-byte bucket records. It searches the bucket's contiguous cell range, increments the sample count for a matching coordinate, or appends a 16-byte record and increments that bucket's occupancy. New records preserve the uninitialized upper half of the Z word, matching the oracle's byte behavior. No public Physics header changed.

The direct `NxPhysicsFluidEmitterAbiTests` differential uses seven signed-coordinate inputs with duplicate coordinates in multiple buckets, one seeded existing cell, and an empty input range. It compares all bucket words and all nine cell-record slots, including untouched sentinels, checks expected sample counts and bucket ranges, return value, and x86 stack balance. The restored control reports three passing cases and zero mismatches.

A deliberate no-op mutation returns before processing cells. Both nonempty cases are rejected, while empty input remains equivalent (`mismatches=2`). Restoring and rebuilding the candidate returns to exact control. Raw logs are in `evidence/raw/phase7-fluid-collision-cell-hash-003530/`.

Oracle RVA `0x86e00` calls an indirect `FindCDCell` extension callback. The supplied UE3 binaries do not include `FluidModel.dll`, so the test binds that oracle callback slot to an independent linear search over the selected bucket range. Production callback integration and Unreal-level static-collision validation remain open, as do the dirty builder and full-DLL closure.


Fresh mainline verification: Phase 5 passes at 2,648/2,648 registered assertions; Phase 7 passes at 1,675/1,675. All 810 reconstruction-tool tests pass. Inventory validation and both runners’ immutable 80-header checks pass. Concise results: `evidence/raw/phase7-fluid-collision-cell-hash-003530/phase-gates.txt`.
