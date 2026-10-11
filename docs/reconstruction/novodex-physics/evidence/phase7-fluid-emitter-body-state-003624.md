# Phase 7 fluid emitter body-state dispatch (`phys_fn_003624`)

`phys_fn_003624` at RVA `0x89930` refreshes emitter world pose through
`phys_fn_003605`, then returns early when the emitter has no frame actor. With
an actor, it reads the body at `actor+8`, copies position from `body+0x158`,
linear velocity from `body+0x34`, and angular velocity from `body+0x40`, then
calls the three FluidModel exports in that order. The candidate implementation
is `nxNpFluidEmitterUpdateBodyState` in `Physics/src/NpFluidEmitter.cpp`.

The registered `NxPhysicsFluidEmitterAbiTests` differential calls the pinned
oracle row and the map-resolved candidate helper. Its no-actor case confirms
the world-pose refresh and early return. Its attached-body case uses asymmetric
non-binary vectors and compares every callback record, callback ordering,
return value, and x86 stack balance. The restored output is exact:

```
fluid emitter body-state update cases=2 oracle_callbacks=5 candidate_callbacks=5 mismatches=0
```

A focused mutation that reads position from `body+0x154` rather than `+0x158`
is rejected with `mismatches=1`. Restoring the source returns the focused
differential to zero mismatches. The callback fixture provides the three body
state exports, and the Phase 7 gate registers this test's coverage floor.

The tested contract is limited to the no-actor and attached-body callback
paths. The extension-backed post-simulation lifecycle remains open. No public
Physics header changed.
