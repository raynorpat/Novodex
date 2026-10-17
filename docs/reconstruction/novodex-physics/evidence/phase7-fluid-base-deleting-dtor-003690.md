# `NxFluid` base scalar deleting destructor (`phys_fn_003690`)

The oracle body at RVA `0x0008afe0` installs the primary `NxFluid` base vtable, returns the original object pointer, and calls the CRT free route only when delete flag bit 0 is set. The base vtable is at RVA `0x00115f48`; the concrete `NxFluid`-derived vtable is at RVA `0x00116008`. The public base declaration is inline in `Physics/include/fluids/NxFluid.h` and remains unchanged.

The candidate base destructor in `Physics/src/Scene.cpp` now installs `gNxFluidSolverBaseVtable` before applying the delete flag. The CMake linker map resolves it as `?nxFluidSolverBaseDeletingDestructor@@YIPAXPAX0E@Z` at `0x1005c210`.

`NxPhysicsFluidEmitterAbiTests` calls the oracle and candidate vtable entries with flags 0 and 1. It verifies the vptr transition, retained payload, return identity, x86 stack balance, and that each DLL frees its flag-one allocation. The restored differential reports `oracle_vptr=1 candidate_vptr=1 payload=1 return=1 flag1_free=1 mismatches=0`.

Removing the candidate base-vptr store is detected (`candidate_vptr=0`, `flag1_free=1`, `mismatches=1`), while the restored differential passes. Fresh Phase 5 and Phase 7 gates pass at `2,868/2,868` and `3,007/3,007` registered coverage assertions. The oracle-only baseline for `NxPhysicsFluidEmitterAbiTests` was updated for the added rows: 35 oracle lines, digest `cb301f70da580852b55936238fa650fd8ef4bfc49f6dac35aa9ad9a188a0f1dc`.

Raw differential, mutation, and gate transcripts are archived under [the raw evidence directory](raw/phase7-fluid-base-deleting-dtor-003690-20261017/).
