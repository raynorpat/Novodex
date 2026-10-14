# Phase 7: ParticleArrayGrow failure ordering (`phys_fn_003503`)

The public `NxFluid::addParticles` path was exercised with an already-created zero-capacity `ParticleArray`. The fixture makes the `ParticleArrayGrow` callback return zero while adding two valid input particles, then catches the resulting access violation. Oracle and candidate both report `caught=1 exception=c0000005 count=0 callbacks=2 allocations=1 grows=1 manager=1/1`; the failed operation leaves both manager fluid arrays intact and does not publish a partial particle count.

A candidate mutation that suppresses the `ParticleArrayGrow` call is rejected: its grow count falls to zero and the registered target exits 1 with a two-byte transcript delta. After restoring the call, the focused oracle/candidate differential is exact (`stdout_delta=0`, exact stderr). The full Phase 7 gate passes at 1,778/1,778 registered coverage assertions.

Raw restored, mutation, and full-gate transcripts are in `evidence/raw/phase7-fluid-particle-array-grow-failure-003503-20261014/`. This proves only the failed-grow branch through the public addParticles wrapper. `phys_fn_003503` remains open for its other callback ordering, manager-publication, capacity, and emitter interactions.
