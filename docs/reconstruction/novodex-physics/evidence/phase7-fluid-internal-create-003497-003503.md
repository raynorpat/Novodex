# Phase 7: internal fluid construction and initialization (`phys_fn_003497`, `phys_fn_003503`)

This evidence records the constructor/default-state and initial-particle slices of the enabled fluid creation path. It does not close either row or claim full callback-table reconstruction.

## Oracle behavior

IDA decompilation of `0x00085b00` (`phys_fn_003497`, 175 bytes) shows the internal `Fluid` constructor installing its primary vtable, zeroing the empty emitter and particle-related members, initializing its six world-bound words to `(+FLT_MAX,+FLT_MAX,+FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX)`, storing the manager link, allocating the `FluidSolver` helper, then setting the helper's read-lock/base vtable and back-pointer. The constructor finishes with the `+0x74` word zero and `+0x80` backend handle sentinel `-1`; the initialization method later replaces the latter with `FluidCreate`'s result.

IDA decompilation of `0x00086080` (`phys_fn_003503`, 303 bytes) shows `Fluid` initialization copying descriptor properties and write-data fields, forwarding the licensed feature flags and descriptor values to `FluidCreate`, creating descriptor emitters, processing initial particle input through `ParticleArray`, resetting the manager refresh byte, and invoking initial particle/state setup. The candidate now copies strided position, velocity, and lifetime records into the 92-byte callback-owned array and reproduces the null-allocation path: the oracle dereferences the failed `ParticleArrayAlloc` result while writing the first record, before appending either manager array entry.

## Differential evidence

The registered `NxPhysicsSimulationTests` fixture creates seven enabled fluids under license product 804. Its constructor marker checks the observable default internal arrays, user-data slot, six bound sentinels, manager association, retained backend handle, solver vtables, solver back-pointer, and public wrapper identity for all seven fluids. It matches the pinned oracle exactly (`stdout_delta=0`, `stderr_exact=True`):

```text
simulation fluid constructor initialized=1 fluids=7 arrays=1 bounds=1 solver-links=1
```

A temporary candidate mutation setting the constructor-initialized word at `fluid+0x74` to one is rejected by this registered differential (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2`). Restoring and rebuilding returns to an exact differential. Raw restored and mutation gate transcripts are archived under `evidence/raw/phase7-fluid-constructor-003497-003503-20261013/`.

Fresh Phase 5, 6, and 7 gates pass at 2,707/2,707, 1,640/1,640, and 1,769/1,769 required coverage assertions. The reconstruction tooling suite passes all 814 tests. The six compressed candidate, mutation, restored-control, and phase-gate transcripts are archived in the same raw-evidence directory.

The initial-particle allocation-failure fixture makes `ParticleArrayAlloc` return null for a valid two-particle descriptor. A caught oracle/candidate differential confirms both DLLs raise `EXCEPTION_ACCESS_VIOLATION` (`c0000005`) before either manager array changes (`internal=0 public=0`); a pre-fix candidate returned a fluid and published both arrays with a null particle-array pointer. The normal strided-particle copy differential still matches exactly. This slice passed Phase 7 at 1,773/1,773 before the next callback-failure case was added. Raw differential and Phase 7 transcripts are archived under `evidence/raw/phase7-fluid-particle-array-003503-20261013/`.

The `FluidGetParticles`-null fixture makes the backend callback return null. IDA at `0x00084a80` shows the oracle reads `Particles+4` immediately after `FluidGetParticles`, before querying maximum capacity or allocating `ParticleArray`. The candidate now performs the same unconditional read. The caught differential matches the oracle's `c0000005` before particle-array allocation or manager publication (`callbacks=1 array-allocs=0 internal=0 public=0`); the old null-as-empty fallback is rejected by a temporary mutation. The full Phase 7 differential passes at 1,774/1,774. The new differential, red baseline, rejected mutation, and full gate transcripts are archived under `evidence/raw/phase7-fluid-get-particles-null-003503-20261013/`.

The tests prove the enumerated constructor/default-state, initial-particle copying, null-allocation, and null-backend-stream slices, not every instruction in either row. `phys_fn_003503` still needs a branch-by-branch audit of callback ordering, particle-array growth failure, and the full callback table. Keep both rows open pending those cases and row-specific mutation evidence.
