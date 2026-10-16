# Phase 7: absent initial velocity and lifetime buffers (`phys_fn_003503`)

This closes the optional-stream branch of the initial-particle helper. It does not close the entire `phys_fn_003503` initialization row; the remaining callback, emitter, and failure interleavings still require instruction-level review.

IDA decompilation of `sub_10084A80` at RVA `0x00084a80` shows every allocated 92-byte particle record first receives zeroed velocity words at offsets `+16..+24` and a zero lifetime word at `+88`. The velocity-copy pass runs only when both the velocity buffer and its word stride are nonzero; the lifetime pass has the same independent guard. Position, velocity, and lifetime streams are therefore independently optional after `NxParticleData::isValid()` accepts the descriptor.

`NxPhysicsFluidNullInitialOptionalBuffersTests` creates two particles with valid positions and null velocity/lifetime buffers. Its callback-backed `ParticleArray` storage is prefilled with `0xa5`, making the oracle's default stores observable. The oracle and candidate produce the exact marker:

```text
fluid initial-particles null-optional count=2 positions=1 zero-velocity-life=1
```

The restored differential matches exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Removing the candidate's velocity-field zeroing store leaves the poisoned values in the record; the test rejects that mutation (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). A rebuilt restored control is exact. The raw restored, mutation, restored-control, full-gate, tooling, and public-header verification transcripts and their hashes are in `evidence/raw/phase7-fluid-null-initial-optional-buffers-003503-20261016/`.

The target is registered in Phase 7, with the callback fixture staged by the differential runner. The full Phase 7 gate passes at 2,957/2,957 assertions, the gate-target suite passes 39/39 tests, and the pinned public Physics header manifest passes for all 80 files. No public headers changed.
