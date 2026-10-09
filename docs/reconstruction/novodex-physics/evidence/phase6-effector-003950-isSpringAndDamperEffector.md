# Phase 6 closure: NpSpringAndDamperEffector::isSpringAndDamperEffector (phys_fn_003950)

`phys_fn_003950` at RVA `0x0008f0d0` is implemented in `Physics/src/core/NpSpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential exercises the public effector wrapper and records its state.

In a throwaway git archive, changed NpSpringAndDamperEffector::isSpringAndDamperEffector to return null instead of this. NxPhysicsEffectorTests records the result alongside the wrapper identity; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=63 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
