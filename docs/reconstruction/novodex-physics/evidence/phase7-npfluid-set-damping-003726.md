# `NxFluid::setDamping` reconstruction (`phys_fn_003726`)

IDA confirms the wrapper at RVA `0x0008b8e0` acquires the Scene write lock, checks the shared callback reentry guard, then forwards the scalar to `FluidSetDamping` with the backend manager and fluid. Unlike the stiffness and viscosity setters, this method has no feature-unavailable branch. The public vtable slot is implemented in `Physics/src/Scene.cpp`; public headers are unchanged.

The licensed simulation fixture checks value bits `0x3e800000`, one callback, correct manager/fluid arguments, and zero errors. Before implementation the candidate access-violated at the null slot while the oracle exited 0. A no-forward callback mutation is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=52`); the restored differential matches exactly.

Fresh full phase gates pass: Phase 5 `2,688/2,688`, Phase 6 `1,622/1,622`, and Phase 7 `1,730/1,730`. Raw gate and mutation transcripts are in `evidence/raw/phase7-npfluid-set-damping-003726/`.
