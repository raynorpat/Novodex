# Phase 6 closure: NpSpringAndDamperEffector::getLinearDamper (phys_fn_003948)

`phys_fn_003948` at RVA `0x0008f090` is implemented in `Physics/src/core/NpSpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential exercises the public effector wrapper and records its state.

In a throwaway git archive, omitted the internal getLinearDamper call from the public NpSpringAndDamperEffector::getLinearDamper wrapper. NxPhysicsEffectorTests observes the getter outputs; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=65 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
