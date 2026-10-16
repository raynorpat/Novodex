# Phase 7: null initial particle-count pointer (`phys_fn_003503`)

This evidence closes one additional branch of Fluid initialization. It does not close the full `phys_fn_003503` row: emitter handling, callback-table coverage, and `ParticleArrayGrow` failure behavior still need their branch-by-branch audit.

## Oracle behavior

IDA decompilation of `0x00084a80`, the initial-particle helper called by `phys_fn_003503` at `0x00086080`, checks the descriptor's `numParticlesPtr` before calling `FluidGetParticles`, querying maximum capacity or cell size, or allocating a `ParticleArray`. A null count pointer therefore skips particle initialization while the enclosing Fluid initialization still succeeds and publishes the internal and public manager-array entries.

The candidate already had this null-pointer guard. `NxPhysicsFluidNullInitialParticleCountTests` now drives the branch with `numParticlesPtr = 0`, checks successful creation and both manager-array counts, and verifies that neither the backend particle callback nor `ParticleArrayAlloc` ran. Its registered differential marker is:

```text
fluid initial-particles null-count returned=1 internal=1 public=1 callbacks=0 array-allocs=0 exact=1
```

The oracle and candidate matched exactly (`stdout_delta=0`, `stderr_exact=True`). Removing the candidate's `numParticlesPtr` guard caused the focused candidate differential to fail while the oracle passed (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). The source mutation was restored and `NxPhysics` rebuilt before the full gate.

## Gate result

The target is registered in Phase 7 and stages the FluidModel callback fixture for the differential run. The official Phase 7 gate passed with `coverage_assertions_evaluated=2956 floor=2956` and `phase_gate=7 status=pass`. The gate-target unit tests pass all 39 tests. Public headers were not changed.
