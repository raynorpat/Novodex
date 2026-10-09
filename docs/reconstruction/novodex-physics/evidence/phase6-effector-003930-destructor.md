# Phase 6 closure: ActorPairEffector destructor (phys_fn_003930)

`phys_fn_003930` at RVA `0x0008ede0` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the body-record observer lists through the public effector lifecycle.

In a throwaway git archive, skipped removing mBody[0] as an observer in ActorPairEffector::~ActorPairEffector. The registered NxPhysicsEffectorTests staged-pair differential exercised effector release; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=19 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
