# Fluid particle null-count early return — `phys_fn_003423`

The IDA pseudocode for oracle RVA `0x00084a80` returns immediately when the `NxParticleData::numParticlesPtr` field is null. The existing callback-backed `NxFluid::addParticles` fixture always supplied a non-null counter, so this branch was not previously observed.

`NxPhysicsFluidNullParticleCountTests` creates a fluid with an empty zero-capacity particle array, then calls `addParticles` with a copied `NxParticleData` whose count pointer is null. Oracle and candidate both preserve array count zero and make no additional backend, allocation, or growth calls: `callbacks=1 allocations=1 grows=0`. Removing only the candidate's null-count guard is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`); the restored differential is exact.

The new target is part of the Phase 7 gate. That gate passes at 2,910/2,910 coverage assertions. `phys_fn_003423` remains partial/open for the cell-size-zero branch and a complete instruction-level audit. Public headers are unchanged.

Raw mutation, restored-control, and Phase 7 gate transcripts are retained in `evidence/raw/phase7-physfn-003423-null-count-20261015/`.
