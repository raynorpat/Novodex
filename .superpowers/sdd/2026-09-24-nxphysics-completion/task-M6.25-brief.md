# Task M6.25 brief — CCD swept bounds (`phys_fn_000740`)

Base: `e3107010`

The public CCD scene did not falsify row 000740, so `NxPhysicsCcdSimulationTests` now contains a direct DLL probe with a synthetic body record and vtable-backed sphere target. The probe verifies the valid bit and all six swept bounds. A row-only immediate-return mutant is rejected (`candidate_exit=1`, `stdout_delta=6`); the restored control is exact.

The new assertion is registered in Phase 6 and Phase 7, and the dedicated CCD target is now part of Phase 7. Phase 7 coverage is 1,801 assertions. Public headers and production source remain unchanged. Evidence is in `docs/reconstruction/novodex-physics/evidence/phase7-ccd-swept-bounds-000740.md`.
