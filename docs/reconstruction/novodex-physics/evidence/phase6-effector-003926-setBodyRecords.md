# Phase 6 closure: ActorPairEffector setBodyRecords (phys_fn_003926)

`phys_fn_003926` at RVA `0x0008ed60` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. The registered `NxPhysicsEffectorTests` staged-pair differential observes the body-record observer lists through the public effector lifecycle.

In a throwaway git archive, skipped removing mBody[0] as an observer in ActorPairEffector::setBodyRecords. The registered NxPhysicsEffectorTests staged-pair differential exercised the body swap; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=29 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
