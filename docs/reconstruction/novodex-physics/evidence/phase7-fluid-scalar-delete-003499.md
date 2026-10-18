# Fluid scalar deleting thunk (`phys_fn_003499`)

`phys_fn_003499` at RVA `0x85bb0` is the Fluid scalar deleting thunk. IDA shows it calls the Fluid destructor body at `0x85890`, tests scalar-delete flag bit 0, conditionally frees `this` through the Foundation SDK allocator, returns `this`, and removes its one-byte flag argument with `ret 4`.

`Physics/src/Scene.cpp` now keeps that thunk separate from the destructor body and routes the Fluid vtable entry through `nxFluidScalarDelete3499`. The linked Win32 Release map resolves the implementation to `?nxFluidScalarDelete3499@@YIPAXPAX0E@Z` at `0x1005ade0` in `Scene.obj`.

The registered `NxPhysicsSimulationTests` differential watches the exact allocation. Its flag-0 scratch-object call must retain the allocation; public `releaseFluid` must free the exact live Fluid allocation with flag 1. The candidate mutation changing `flags&1` to `flags&2` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=67`). The restored DLL pair passes with exact output (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

Fresh registered gates pass: Phase 5 at 2,894/2,894 assertions, Phase 6 at 1,751/1,751, and Phase 7 at 3,033/3,033. Full raw gate and differential logs are in `evidence/raw/phase7-fluid-scalar-delete-003499-20261018/`.
