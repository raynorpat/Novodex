# Phase 6 closure: SpringAndDamperEffector apply (phys_fn_003979)

`phys_fn_003979` at RVA `0x0008f700` is the slot-3 effector simulation path in `Physics/src/core/SpringAndDamperEffector.cpp`. The public three-step fixture configures nonzero spring and damper parameters, then records the body velocity after each step.

Row-specific falsification (Phase 6 closure packet, 2026-10-09): In a fresh throwaway git archive from the current commit, changed SpringAndDamperEffector::apply to a no-op and rebuilt NxPhysics.dll. The registered NxPhysicsSimulationTests staged-pair differential observes the public three-step effector simulation: the oracle reports second/third-step vx 3e8e38e3/3f0a9508, while the mutant reports 00000000/00000000 (stdout_delta=4); both processes exit 0 and stderr is exact. Restored control passed with both exits zero, stdout_delta=0, and exact stderr.

The mutation was confined to a fresh throwaway git archive and restored before the control run. The row is recorded as `differential_falsified` under `NxPhysicsSimulationTests`.
