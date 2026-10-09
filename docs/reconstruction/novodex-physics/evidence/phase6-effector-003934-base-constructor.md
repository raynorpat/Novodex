# Phase 6 closure: Effector constructor (phys_fn_003934)

`phys_fn_003934` at RVA `0x0008ee80` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential exercises this row through the public effector lifecycle.

In a throwaway git archive, changed the Effector constructor to store a null scene pointer. The registered NxPhysicsEffectorTests staged-pair differential reached this base constructor through the public effector factory; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=61 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
