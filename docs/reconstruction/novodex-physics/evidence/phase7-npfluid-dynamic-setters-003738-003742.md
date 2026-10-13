# Dynamic-collision NxFluid setters

Rows `phys_fn_003738` (RVA `0x8bb90`) and `phys_fn_003742` (RVA `0x8bcb0`) implement the public dynamic-collision restitution and adhesion setters. Both wrappers preserve the oracle's unavailable-feature warning, then acquire the Scene write lock, honor callback reentry, and forward the manager, fluid, and value to the matching FluidModel callback.

`NxPhysicsSimulationTests` exercises both public setters on the licensed test fluid. It checks the exact warning code, file, line, and message, the scalar bits (`0x3f400000` and `0x3e000000`), callback count, and manager/fluid arguments. The focused oracle/candidate differential passes with `stdout_delta=0` and exact stderr.

The setters were mutation-tested independently by disabling each callback forward. Each mutant was rejected (`candidate_exit=1`, `stdout_delta=52`); each restored control matched exactly. The Phase 5 and Phase 7 gates require both setter markers. Fresh Phase 5, 6, and 7 gates pass at 2,692/2,692, 1,626/1,626, and 1,734/1,734 coverage assertions. The fixture captures the kernel-getter error delta immediately after that getter so the two later expected warnings do not contaminate its existing zero-error marker.

Raw build, mutation, differential, and phase-gate logs are under `evidence/raw/phase7-npfluid-dynamic-setters-003738-003742/`.
