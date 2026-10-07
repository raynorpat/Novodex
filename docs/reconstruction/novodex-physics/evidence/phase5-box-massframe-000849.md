# Box shape mass accumulator falsification — 2026-10-07

`phys_fn_000849` (`BoxShape::nxBoxComputeMassFrame`, RVA `0x0001c8c0`) builds a box mass frame from half-extents, folds the local-pose payload, translates the center of mass, scales by density when required, and merges into the destination. The registered `NxPhysicsShapeVtableTests` now calls Box shape vtable slot 4 for three dimension sets, identity and rotated/translated poses, densities 1 and 2, and both clear and skip flags. The pinned oracle and candidate match all 24 cases (`shape vtable boxmass oracle_digest=82843962 cases=24 failures=0`; `build/shape-vtable-000849-baseline.log`).

A throwaway archive mutation omitted `local.nxMassFrameScale(density)` from `BoxShape::nxBoxComputeMassFrame`. The oracle comparison reported 12 mismatches, one for each density-2 case, and the target exited 1. The base shape-vtable, box-sweep, box-hull, capsule, and mass-frame-builder sections remained exact (`build/shape-vtable-000849-mutation.log`). The mutation did not touch the main checkout.

This closes the row's mutation-sensitivity requirement. The remaining Phase 5 object rows and full-DLL work remain open. Public Physics headers are unchanged.
