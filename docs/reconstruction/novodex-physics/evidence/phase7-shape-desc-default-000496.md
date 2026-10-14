# Phase 7 row 000496: `NxShapeDesc::setToDefault`

`phys_fn_000496` at oracle RVA `0x0000fd10` initializes the base shape descriptor: its embedded matrix becomes identity, `shapeFlags` becomes `NX_SF_VISUALIZATION` (`8`), and the group, material index, user data, and name fields are cleared. These stores match the inline implementation in the unchanged public header [NxShapeDesc.h](../../../../Physics/include/NxShapeDesc.h).

The CMake Win32 Release map resolves the emitted candidate method as `?setToDefault@NxShapeDesc@@UAEXXZ` at candidate RVA `0x0006d4d0` in `Scene.obj`. `NxPhysicsObjectLayoutTests` loads a copy of the built candidate DLL under a distinct module name, resolves that method through the adjacent `NxPhysics.map`, and calls the actual DLL code on a poisoned `NxSphereShapeDesc`. It compares bytes +8 through +0x47 against the oracle call to row 000496. The registered output is `tmplinit row=phys_fn_000496 candidate_dll_vs_oracle bytes=64 mismatches=0`.

The implementation-level mutation patches the first byte of the loaded candidate method to `ret`, leaving the poisoned fields untouched. The same fixture reports `mismatches=64`, increments the differential failure count, and exits 1. The restored control reports zero mismatches and exits 0. The prior source-inline check is also retained as a separate assertion.

Fresh Phase 5, 6, and 7 gates pass at floors 2,735, 1,660, and 1,991. The refreshed oracle-only proof retains digest `774d629d`. The Phase 7 ledger now closes this row as `oracle_differential_falsified`. No public Physics headers changed. Full DLL reconstruction remains open.
