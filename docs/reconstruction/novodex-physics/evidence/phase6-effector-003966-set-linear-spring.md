# Phase 6 closure: SpringAndDamperEffector::setLinearSpring (phys_fn_003966)

`phys_fn_003966` at RVA `0x0008f4f0` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector parameters through the public API and internal record.

In a throwaway git archive, changed SpringAndDamperEffector::setLinearSpring to store zero for mDistRelaxed. NxPhysicsEffectorTests observes internal spring parameters and then exercises the effector; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
