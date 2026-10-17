# Fluid emitter invalid-descriptor path (`phys_fn_003501`)

The public `NxFluid::createEmitter` fixture now covers the early descriptor rejection path with `dimensionX = -1`. It compares the return, exact `NXE_INVALID_PARAMETER` diagnostic from `Fluid.cpp:206`, zero attempts to allocate the private `0x78`-byte emitter record, unchanged public and internal emitter arrays, and unchanged FluidModel callback counts. The public headers and product behavior are unchanged.

The restored staged oracle/candidate differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). A candidate-only mutation changing the diagnostic code from `NXE_INVALID_PARAMETER` to `NXE_NO_ERROR` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=87`).

Fresh Phase 5, 6, and 7 gates pass at 2,860/2,860, 1,727/1,727, and 2,994/2,994 coverage assertions. The gate-target structural tests pass 39/39. Phase 5 also required linking `TriangleMeshTopology.cpp` into the object-layout and shape-vtable test executables after the mesh-raycast slot reconstruction began using its lazy-normal helper. The raycast coverage markers for down/no-normal hits now match the exact values printed by both staged DLLs.

This evidence covers the descriptor-validation branch only; it does not close `phys_fn_003501`, whose remaining allocation-growth and backend-helper behavior is still tracked separately. Raw differential and gate transcripts are gzip-compressed under [evidence/raw/phase7-fluid-emitter-desc-validation-003501-20261017](raw/phase7-fluid-emitter-desc-validation-003501-20261017/).
