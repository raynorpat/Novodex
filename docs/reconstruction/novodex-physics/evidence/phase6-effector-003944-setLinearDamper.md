# Phase 6 closure: NpSpringAndDamperEffector::setLinearDamper (phys_fn_003944)

`phys_fn_003944` at RVA `0x0008efe0` is implemented in `Physics/src/core/NpSpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential exercises the public effector wrapper and records its state.

In a throwaway git archive, omitted the internal setLinearDamper call from the public NpSpringAndDamperEffector::setLinearDamper wrapper. NxPhysicsEffectorTests exercises the public setter and observes its state; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=63 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
