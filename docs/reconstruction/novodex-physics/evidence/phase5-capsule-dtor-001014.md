# Capsule scalar deleting destructor mutation — `phys_fn_001014`

Date: 2026-10-08

The `CapsuleShape::nxCapsuleScalarDeletingDtor` row at RVA `0x000225e0` runs the collision-object deleting destructor, the ShapeBase destruction chain, and a conditional self-free when flag bit 0 is set. The registered Phase 5 oracle differential `NxPhysicsShapeVtableTests` covers both flag 0 (one allocator free) and flag 1 (two allocator frees).

For row-specific falsification, the final self-free guard was temporarily changed to false. The pinned oracle remained at digest `ed1294b6` over 629 cases; the candidate reported `mismatches=1` and exited 1. Restoring the flag check returns the target to `shape vtable oracle_digest=ed1294b6 cases=629 mismatches=0`, exit 0. The source was restored and rebuilt before the clean verification.

The harness's aggregate shape-vtable result now labels this comparison count `mismatches` rather than `failures`, making it available to the closure verifier's oracle-differential count format. The same registered coverage line remains required by Phase 5. No public Physics headers changed.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,564/2,564 registered coverage assertions (`build/phase5-capsule-dtor-final-pass.log`). The approved full Viewer selection passes all 48 CTest entries, exercising all 39 available scenes; 43 pass and five known signature-verified pinned-oracle asset cases skip (`build/viewer-all-scenes-approved-design-final.log`).

Mutation logs: `build/phase5-capsule-dtor-mutation/mutation-final.log` and `build/phase5-capsule-dtor-mutation/restored-final.log`.
