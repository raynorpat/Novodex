# Phase 7 row 000496: `NxShapeDesc::setToDefault` mapping

`phys_fn_000496` at oracle RVA `0x0000fd10` initializes the base shape descriptor: its embedded matrix becomes identity, `shapeFlags` becomes `NX_SF_VISUALIZATION` (`8`), and the group, material index, user data, and name fields are cleared. These stores match the inline implementation in the unchanged public header [NxShapeDesc.h](../../../../Physics/include/NxShapeDesc.h).

The CMake Win32 Release map resolves the candidate method as `?setToDefault@NxShapeDesc@@UAEXXZ` at RVA `0x0006d4d0` in `Scene.obj`. `NxPhysicsObjectLayoutTests` now calls the candidate method after poisoning bytes +8 through +0x47, then compares all 64 bytes with the oracle invocation of row 000496. Its registered line is `tmplinit row=phys_fn_000496 candidate_vs_oracle bytes=64 mismatches=0`.

The assertion is mutation-tested at the harness boundary: a throwaway mutation that omits the candidate call produced 64 mismatches and a nonzero test exit. The restored test reports zero mismatches. Fresh Phase 5, 6, and 7 gates passed at coverage floors 2,734, 1,659, and 1,990, respectively. The refreshed oracle-only proof retained digest `774d629d`.

This proves the candidate-side inline source compiled into the test target against the oracle behavior. It does not yet mutate the implementation in the candidate DLL itself, so the inventory row remains reconstructed and the Phase 7 closure ledger keeps it deferred until an implementation-level DLL mutation is caught. No public Physics headers changed. Full DLL reconstruction remains open.
