# Public `NxFluid::getDamping()`

The primary `NxFluid` vtable now exposes `getDamping()` at slot 15. The oracle acquires the wrapper's scene read lock, follows the internal-fluid and backend-manager links, calls `FluidGetDamping(manager, fluid)`, then releases the lock. Unlike stiffness and viscosity, damping has no FluidKey feature-warning branch.

## Differential and mutation evidence

The enabled FluidModel fixture accepts only its real manager and fluid handles, returns `0.375f`, and counts callback invocations. The simulation differential checks the returned IEEE-754 word and exactly one callback. Before adding slot 15, the oracle returned `0x3ec00000` with one callback while the candidate returned the fallback `0xffc00000` with none; candidate exit was 1. With the implementation restored, both processes return `0x3ec00000`, report one call, exit 0, and match stdout and stderr exactly.

A no-forward mutant that returns `0.0f` instead is rejected: candidate exit 1, `stdout_delta=52`, with the candidate marker `value=00000000 calls=0`. The registered marker is `simulation fluid public-api damping value=3ec00000 calls=1`.

Fresh Phase 5 and Phase 7 gates pass at 2,681/2,681 and 1,714/1,714 assertions. The compressed red, restored, mutation, and gate transcripts are in [the raw evidence directory](raw/phase7-npfluid-damping-getter-20261013/).

No public header changed. Fluid damping and collision scalar setters, particle data, the remaining public `NxFluid` methods, and full-DLL closure remain open. Static and dynamic collision getters are covered separately in [their evidence](phase7-npfluid-collision-getters.md).
