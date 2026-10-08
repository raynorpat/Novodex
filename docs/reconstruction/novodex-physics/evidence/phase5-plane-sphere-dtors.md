# Plane and sphere deleting destructor mutations — `phys_fn_001263`, `phys_fn_001375`

Date: 2026-10-08

`NxPhysicsShapeVtableTests` already calls the oracle and candidate plane and sphere deleting destructors through their installed shape tables with flags 0 and 1. The harness compares allocator free deltas, including the embedded collision object's deletion and the shape's flag-1 self-free.

For `phys_fn_001263` (`PlaneShape::nxPlaneScalarDeletingDtor`, RVA `0x00025420`), the flag-1 self-free was temporarily suppressed. The registered target reported `shape vtable oracle_digest=ed1294b6 cases=629 mismatches=1` and exited 1. Restoring the guard returned to zero mismatches.

For `phys_fn_001375` (`SphereShape::nxSphereScalarDeletingDtor`, RVA `0x00027c30`), the same targeted mutation independently produced one mismatch and exit 1. Restoring the guard again returned to the pinned 629-case digest with zero mismatches. No public Physics headers changed.

The fresh Win32 Release Phase 5 gate passes all 19 targets and all 2,565 registered coverage assertions with both destructors restored (`build/phase5-shape-dtor-final.log`).

Logs: `build/phase5-plane-dtor-mutation.log`, `build/phase5-sphere-dtor-mutation.log`, and `build/phase5-shape-dtor-restored.log`.
