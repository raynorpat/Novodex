# Box and mesh deleting destructor mutations — `phys_fn_000979`, `phys_fn_001399`

Date: 2026-10-08

`NxPhysicsShapeVtableTests` drives the box and mesh deleting destructors through their installed oracle and candidate tables with flags 0 and 1.

For `phys_fn_000979` (`BoxShape::nxBoxScalarDeletingDtor`, RVA `0x00021940`), suppressing the candidate's flag-1 self-free produced one mismatch in the 629-case shape-vtable differential. Restoring the guard returned to the pinned digest with zero mismatches.

For `phys_fn_001399` (`MeshShape::nxMeshScalarDeletingDtor`, RVA `0x00028e80`), the test also binds a synthetic mesh record and checks its reference count. Suppressing the flag-1 self-free produced `frees=2/1` and one mismatch while both oracle and candidate decremented the reference count from 7 to 6. Restoring the guard returned to the pinned digest with zero mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 targets and 2,565/2,565 registered coverage assertions (`build/phase5-shape-dtor-final.log`). No public Physics headers changed.

Logs: `build/phase5-box-dtor-mutation.log`, `build/phase5-mesh-dtor-mutation.log`, and `build/phase5-shape-dtors-restored.log`.
