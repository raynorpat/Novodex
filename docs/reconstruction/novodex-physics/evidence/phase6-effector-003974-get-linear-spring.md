# Phase 6 closure: SpringAndDamperEffector::getLinearSpring (phys_fn_003974)

`phys_fn_003974` at RVA `0x0008f660` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector parameters through the public API and internal record.

In a throwaway git archive, changed SpringAndDamperEffector::getLinearSpring to return zero for distRelaxed. NxPhysicsEffectorTests observes the public spring getter outputs and exercises the effector; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
