# Phase 5 `Prunable` owner world-AABB dispatch — `phys_fn_001269`

Date: 2026-10-08

`phys_fn_001269` at RVA `0x00025510` dispatches a prunable owner's world-AABB callback through owner vtable slot 9 (`+0x24`). The existing `NxPhysicsSceneRaycastTests` fixture exercises this path when it moves a static shape after the static tree has been built and queries that shape at its new location. This test target is registered in the Phase 5 staged-pair set and pins the moved-shape ray result and hit details.

For row-specific falsification, a temporary mutation changed `shapeOwnerWorldAABB` to call owner vtable slot 8. The pinned oracle exited 0; the candidate exited with `-1073741571` (`STATUS_STACK_OVERFLOW`), and the runner recorded a 2,783-byte stdout delta with exact stderr. The mutation was reverted. Because copying the original source preserved an old timestamp, the first incremental build reused the mutant binary; after forcing `ObjectModel.cpp` to rebuild, the restored `NxPhysics.dll` was staged and the raycast target passed with both exits 0, `stdout_delta=0`, and exact stderr. Source contents were checked against the saved pre-mutation copy.

This closes the specific owner world-AABB adapter row through its existing Phase 5 scene-raycast differential. The adjacent owner-notify adapter `phys_fn_001271` and the remaining Phase 5 rows stay open. No public Physics header was changed.

Logs: `build/phase5-owner-world-aabb-mutation/differential.log` and `build/phase5-owner-world-aabb-mutation/restored-clean.log`.
