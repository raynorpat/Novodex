# Phase 6 closure: NpSpringAndDamperEffector constructor (phys_fn_003958)

`phys_fn_003958` at RVA `0x0008f150` is implemented in `Physics/src/core/NpSpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the effector lifecycle and state.

In a throwaway git archive, changed the constructor to store the internal effector pointer plus four bytes. NxPhysicsEffectorTests exercises the public factory and inspects the wrapper-to-internal link; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
