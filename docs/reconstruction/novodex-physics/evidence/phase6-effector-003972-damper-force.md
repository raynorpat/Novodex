# Phase 6 closure: SpringAndDamperEffector damper force (phys_fn_003972)

`phys_fn_003972` at RVA `0x0008f5e0` implements `SpringAndDamperEffector::damperForce` in `Physics/src/core/SpringAndDamperEffector.cpp`. The public simulation fixture configures nonzero compression and stretch damper limits and advances the scene three times. The third step begins after spring force has produced relative velocity, so the transcript observes the damper calculation.

For row `phys_fn_003972`, in a fresh throwaway git archive from the current commit, copied the approved three-step public simulation fixture and registered coverage marker, then changed SpringAndDamperEffector::damperForce to return zero. NxPhysicsSimulationTests reached the effector with nonzero relative velocity on its third simulation step; oracle third-step vx was 3f0a9508 and the mutant was 3f0de49b (stdout_delta=2), with both processes exiting 0 and exact stderr. Restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation was confined to a fresh throwaway git archive and restored before the control run. The row is recorded as `differential_falsified` under `NxPhysicsSimulationTests`.
