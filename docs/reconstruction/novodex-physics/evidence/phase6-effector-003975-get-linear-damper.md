# Phase 6 closure: SpringAndDamperEffector::getLinearDamper (phys_fn_003975)

`phys_fn_003975` at RVA `0x0008f690` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector parameters through the public API and internal record.

In a throwaway git archive, changed SpringAndDamperEffector::getLinearDamper to return zero for velStretchSaturate. NxPhysicsEffectorTests observes the public damper getter outputs and exercises the effector; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
