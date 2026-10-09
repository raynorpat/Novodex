# Phase 6 closure: SpringAndDamperEffector::setBodies (phys_fn_003962)

`phys_fn_003962` at RVA `0x0008f200` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector lifecycle and state.

In a throwaway git archive, omitted the call to setBodyRecords from SpringAndDamperEffector::setBodies. NxPhysicsEffectorTests observes body pointers, observer lists, and anchor state; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=78 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
