# Phase 6 closure: SpringAndDamperEffector constructor (phys_fn_003960)

`phys_fn_003960` at RVA `0x0008f180` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector lifecycle and state.

In a throwaway git archive, changed the constructor’s public-wrapper allocation request from 0x18 to 0x1c bytes. NxPhysicsEffectorTests observes allocation sizes and constructed state; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=78 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
