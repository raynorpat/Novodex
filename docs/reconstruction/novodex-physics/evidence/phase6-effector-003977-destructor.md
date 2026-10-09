# Phase 6 closure: SpringAndDamperEffector destructor (phys_fn_003977)

`phys_fn_003977` at RVA `0x0008f6c0` is implemented in `Physics/src/core/SpringAndDamperEffector.cpp`. It releases the public Np wrapper before the internal effector is destroyed.

In a throwaway git archive, changed SpringAndDamperEffector::~SpringAndDamperEffector to skip deleting its public Np wrapper. The registered NxPhysicsEffectorTests staged-pair differential exercises effector release, release/create cycles, and scene cleanup; the oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=76 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsEffectorTests`.
