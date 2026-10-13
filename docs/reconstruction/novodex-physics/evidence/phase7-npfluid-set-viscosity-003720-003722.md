# `NxFluid` viscosity accessors (`phys_fn_003720`, `phys_fn_003722`)

IDA confirms the public viscosity getter at RVA `0x0008b760` reports `NpFluid.cpp:128` when the feature is unavailable, otherwise acquires the wrapper read lock and calls `FluidGetViscosity`. The public setter at RVA `0x0008b7d0` reports the unavailable-feature warning at line 137, takes the Scene write lock, checks the shared callback reentry guard at line 140, and forwards manager, fluid, and value to `FluidSetViscosity`. Both slots are implemented in `Physics/src/Scene.cpp`; public headers are unchanged.

The existing unlicensed public getter fixture rejects a no-diagnostic mutation with `stdout_delta=2` while both processes exit zero; restored control is exact. The licensed setter fixture checks value bits `0x3ec00000`, one callback, correct manager/fluid arguments, and zero errors. A no-forward mutant is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=52`); restored control matches exactly.

Fresh full phase gates pass with the required setter marker: Phase 5 `2,687/2,687`, Phase 6 `1,621/1,621`, and Phase 7 `1,729/1,729`. Raw gate and mutation transcripts are in `evidence/raw/phase7-npfluid-set-viscosity-003722/`.
