# `NxFluid::setStaticCollisionRestitution` (`phys_fn_003730`)

IDA shows the wrapper at RVA `0x0008b9c0` acquiring the Scene write lock, checking the shared API reentry guard, and forwarding the backend manager, fluid, and scalar to `FluidSetStaticCollisionRestitution`. It has no feature-unavailable warning branch. The vtable slot is now implemented in `Physics/src/Scene.cpp`; public headers are unchanged.

The licensed public simulation fixture checks scalar bits `0x3f000000`, one callback, correct manager/fluid arguments, and zero errors. Before implementation, the candidate access-violated at the null slot while the oracle exited 0. Suppressing the callback in a temporary candidate build is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=52`); restored control matches exactly.

Fresh full phase gates pass: Phase 5 `2,689/2,689`, Phase 6 `1,623/1,623`, and Phase 7 `1,731/1,731`. Raw gate and mutation transcripts are in `evidence/raw/phase7-npfluid-static-rest-restitution-003730/`.
