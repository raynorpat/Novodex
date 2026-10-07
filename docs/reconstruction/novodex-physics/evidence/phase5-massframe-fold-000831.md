# Mass-frame payload-fold falsification — 2026-10-07

`phys_fn_000831` (`MassFrame::nxMassFrameFoldPayload`, RVA `0x0001bdc0`) folds a `{Vec3 d; SymMat3 K}` payload into a mass frame. The registered `NxPhysicsShapeVtableTests` baseline passes: 626 shape-vtable cases, 24 Box slot-4 cases, and 201 mass-frame cases all report zero failures (`build/shape-vtable-000831-baseline.log`).

A throwaway archive mutation changed the first inertia output from `s` to `s + 1.0`. The same test then reported 18 mismatches and exited 1: two capsule slot-4 cases and sixteen cached-mesh slot-4 cases. Box mass, box sweep, box hull, and mass-frame-builder sections remained exact (`build/shape-vtable-000831-mutation.log`). The mutation did not touch the main checkout.

This closes the row's mutation-sensitivity requirement. The remaining Phase 5 rows and full-DLL work remain open. Public Physics headers are unchanged.
