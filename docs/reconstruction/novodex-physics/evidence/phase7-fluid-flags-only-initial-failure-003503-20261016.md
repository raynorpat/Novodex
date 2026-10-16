# Phase 7: flags-only initial-particle failure (`phys_fn_003503`)

`NxParticleData::isValid()` accepts `numParticlesPtr` with a non-null `bufferFlags` and nonzero flag stride even when `bufferPos` is null. The oracle initializer at RVA `0x00084a80` nevertheless reads the position pointer for every positive particle count after invoking `FluidGetParticles` and allocating the internal ParticleArray.

`NxPhysicsFluidFlagsOnlyInitialParticleFailureTests` creates a one-particle descriptor with only flags supplied. Oracle and candidate both catch `EXCEPTION_ACCESS_VIOLATION` (`0xc0000005`), return no fluid, leave both manager counts at zero, and report one particle callback plus one array allocation. The exact marker is:

```text
fluid initial-particles flags-only caught=1 exception=c0000005 returned=0 internal=0 public=0 callbacks=1 array-allocs=1 valid=1
```

The restored differential matches (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). A mutation that returns early when `bufferPos` is null is rejected: the candidate returns a fluid, publishes one internal and one public manager entry, and the differential fails (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`, `stderr_exact=False`). The temporary mutation was removed and the clean candidate rebuilt before subsequent gates.

This target is registered in Phase 7 and the differential runner stages the `FluidModel` callback fixture for it. The full callback ordering and function body remain open; this closes only the flags-only descriptor failure branch. Phase 5 also passes at its 2,825/2,825 registered floor after the Phase 5 baseline update. The raw restored run, mutation run, restored control, Phase 5 and Phase 7 gates, tooling checks, inventory validation, and header-manifest check are preserved under `evidence/raw/phase7-fluid-flags-only-initial-failure-003503-20261016/`.
