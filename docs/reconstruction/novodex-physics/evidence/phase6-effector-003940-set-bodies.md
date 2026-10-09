# Phase 6 closure: NpSpringAndDamperEffector::setBodies (phys_fn_003940)

`phys_fn_003940` at RVA `0x0008eef0` is implemented in `Physics/src/core/NpSpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential exercises this row through the public effector lifecycle.

In a throwaway git archive, omitted the call from NpSpringAndDamperEffector::setBodies to the internal setBodies method. The registered NxPhysicsEffectorTests staged-pair differential reached the setter through the public API; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=61 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
