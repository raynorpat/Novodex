# `NxFluid::setStaticCollisionAdhesion` (`phys_fn_003734`)

IDA confirms the wrapper at RVA `0x0008baa0` takes the Scene write lock, checks the shared callback reentry guard, and forwards manager, fluid, and scalar to `FluidSetStaticCollisionAdhesion`. The method has no feature-warning branch. The public vtable slot is reconstructed in `Physics/src/Scene.cpp`; public headers remain unchanged.

The licensed simulation fixture checks value bits `0x3e800000`, one callback, correct manager/fluid arguments, and zero errors. Before implementation, the candidate access-violated at the null slot while the oracle exited 0. Suppressing the callback in a temporary candidate build is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=52`); restored control matches exactly.

Fresh full phase gates pass: Phase 5 `2,690/2,690`, Phase 6 `1,624/1,624`, and Phase 7 `1,732/1,732`. Raw gate and mutation transcripts are in `evidence/raw/phase7-npfluid-static-adhesion-003734/`.
