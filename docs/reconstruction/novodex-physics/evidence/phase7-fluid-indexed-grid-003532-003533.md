# Phase 7 fluid collision indexed-mesh builder — phys_fn_003532/003533

The census records two adjacent rows at RVA `0x86f00` (`phys_fn_003532`, 17 bytes) and `0x86f11` (`phys_fn_003533`, 700 bytes). They are a single function entry and contiguous continuation, not two callables. The private implementation is `nxFluidCollisionBuildIndexedGrid` in `Physics/src/FluidCollision.cpp`; no public Physics header changed.

The wrapper returns 0 for a null output array. Otherwise it walks every indexed triangle, transforms its three vertices by the mesh's 3x4 transform, clears the cell count, enumerates cells through `phys_fn_003524`, and accumulates bucket/cell records through `phys_fn_003530`. Empty input returns 1 while preserving the seeded cell state.

`NxPhysicsFluidEmitterAbiTests` calls oracle RVA `0x86f00` and the candidate symbol resolved from the candidate DLL map. The three cases cover a two-triangle asymmetric affine mesh, empty input with a seeded output count, and null output. The probe compares cell storage/count/capacity, bucket rows, complete cell records, return values, and x86 stack balance. The restored run reports all three cases and `mismatches=0`.

A temporary candidate mutation skipped the indexed-triangle loop. The asymmetric nonempty case detected the missing cells and buckets (`mismatches=1`); empty and null-output behavior remained unchanged. The original source was restored byte-for-byte (SHA-256 `3eaabe1c32786c60c1af25aa8e53743455e039435f1cc86773bcce7da235cda6`) and the rebuilt control returned `mismatches=0`. Raw outputs are retained in `evidence/raw/phase7-fluid-indexed-grid-003532-003533/`.

The oracle directory does not include `FluidModel.dll`; the direct fixture binds absent SAT, array-growth, and `FindCDCell` callback slots to independent references. This closes the two census rows' wrapper semantics but leaves integration with the production FluidModel callbacks and Unreal static-collision validation open.

At closure registration, Phase 7 accounts for 110 closed / 451 deferred rows and the registered assertion floor rises to 1,676. The fresh full gates are run after merging this checkpoint to mainline.
