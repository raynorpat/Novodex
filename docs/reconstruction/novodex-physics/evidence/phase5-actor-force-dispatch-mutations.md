# Phase 5 actor force and torque dispatch mutation evidence

Three reconstructed actor vtable rows are independently falsified by the
registered `NxPhysicsActorForceTests` oracle differential:

- `phys_fn_000054`, `addForceAtPos` at RVA `0x000026f0`: swap the force and
  position arguments to `nxNpActorForceAtPos`; caught with `stdout_delta=18`.
- `phys_fn_000056`, `addForce` at RVA `0x000027a0`: change the accumulator's
  torque selector from `false` to `true`; caught with `stdout_delta=36`.
- `phys_fn_000058`, `addTorque` at RVA `0x00002850`: change the accumulator's
  torque selector from `true` to `false`; caught with `stdout_delta=30`.

The baseline and each restored rebuild match exactly. For all three mutants,
both processes exit zero and stderr matches, so the detected failures are
oracle/candidate simulation transcript differences. Each source mutation is
restored byte-for-byte before the next row is tested.

Logs are in the ignored worktree `build/` directory. Mutants:
`phase5-add-force-at-position-mutant.log`, `phase5-add-force-mutant.log`, and
`phase5-add-torque-mutant.log`. Restored controls:
`phase5-add-force-at-position-restored.log`, `phase5-add-force-restored.log`,
and `phase5-add-torque-restored.log`.
