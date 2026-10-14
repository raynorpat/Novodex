# Phase 7: ParticleArray append behavior (`phys_fn_003503`)

The registered public `NxFluid::addParticles` differential now starts with a two-particle initial array, then adds two more particles. Both oracle and candidate preserve the first two records, write the new records at the existing array-count offset, call `ParticleArrayGrow` once, and leave the stored array count at the current batch size (`2`). The exact marker is `fluid add-particles append caught=0 array-count=2 original-preserved=1 appended-positions=1 callbacks=2 allocations=1 grows=1`.

A mutation changing the final count store to `existingArrayCount + count` is rejected (`array-count=4`, candidate exit 1, `stdout_delta=2`). Restored oracle/candidate output matches exactly. The complete Phase 7 gate passes at 1,779/1,779 assertions; the tooling suite passes all 814 tests.

Raw differential, mutant, and compressed gate transcripts are in `evidence/raw/phase7-fluid-particle-array-append-003503-20261014/`. This characterizes the append storage and count behavior only; `phys_fn_003503` remains open for the remaining callback, emitter, and failure paths.
