# Phase 6 closure: SpringAndDamperEffector::setLinearDamper (phys_fn_003968)

`phys_fn_003968` at RVA `0x0008f520` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector parameters through the public API and internal record.

In a throwaway git archive, changed SpringAndDamperEffector::setLinearDamper to store zero for mVelStretchSaturate. NxPhysicsEffectorTests observes internal damper parameters and then exercises the effector; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
