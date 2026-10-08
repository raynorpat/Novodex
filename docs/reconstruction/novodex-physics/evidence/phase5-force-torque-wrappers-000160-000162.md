# Phase 5 local force and torque entry points

The pinned oracle and clean Win32 Release candidate were compared through `NxPhysicsActorForceTests`. Clean and restored staged pairs both exit 0 with `stdout_delta=0` and exact stderr. Each isolated selector mutation was rebuilt and rejected by the registered differential; both process pairs exit 0 with exact stderr.

- `phys_fn_000160` (`addLocalForce`): changing the accumulation torque selector from false to true produces `stdout_delta=16` (`build/m2-local-force-160-mutant.log`).
- `phys_fn_000162` (`addLocalTorque`): changing the accumulation torque selector from true to false produces `stdout_delta=16` (`build/m2-local-force-162-mutant.log`).

Restored baselines are exact (`build/m2-local-force-160-restored.log`, `build/m2-local-force-torque-restored.log`). No public Physics headers changed.
