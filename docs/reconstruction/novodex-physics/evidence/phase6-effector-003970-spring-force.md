# Phase 6 closure: SpringAndDamperEffector::springForce (phys_fn_003970)

`phys_fn_003970` at RVA `0x0008f540` computes the spring force used by the internal effector tick. The registered `NxPhysicsSimulationTests` staged-pair differential exercises the public effector over two real simulation steps with a world anchor 2.0 units from the dynamic body.

In a throwaway git archive, changed SpringAndDamperEffector::springForce to return zero for every distance. The registered NxPhysicsSimulationTests staged-pair differential exercises the public effector over two real simulation steps; its setup has a world anchor 2.0 units from the body and a nonzero spring force. The oracle exited 0 and the mutated candidate hit an access violation, with stdout_delta=3702 and exact stderr. The restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation remained only in the throwaway archive. The row is recorded as `differential_falsified` under `NxPhysicsSimulationTests`.
