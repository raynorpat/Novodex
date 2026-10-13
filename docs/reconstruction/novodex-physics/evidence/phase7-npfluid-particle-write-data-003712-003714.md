# Phase 7 public NxFluid particle write-data accessors (`phys_fn_003712`, `phys_fn_003714`)

The public solver vtable now routes `getParticlesWriteData` and `setParticlesWriteData` through the reconstructed wrappers in `Physics/src/Scene.cpp`. IDA confirms that the getter copies eleven dwords (44 bytes) from internal-fluid+0x28 to the hidden aggregate return buffer under the Scene read lock. The setter takes the Scene write lock, checks the shared callback reentry guard, copies the eleven dwords from the input structure to internal-fluid+0x28, and unlocks. The getter slot uses the MSVC x86 hidden-result-pointer ABI and returns with the expected one stack argument.

The registered `NxPhysicsSimulationTests` fixture seeds the internal record with eleven distinct words, reads it through the public getter, writes a second `NxParticleData` record through the public setter, reads it back, and restores the original bytes. The required marker is `simulation fluid public-api particle-write-data initial=1 stored=1 roundtrip=1`.

Both rows are independently mutation-falsified. A no-op setter is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=61`); a zero-return getter is also rejected with `stdout_delta=61`. The restored staged differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Mutation and control transcripts are preserved in `evidence/raw/phase7-npfluid-particle-write-data-003712-003714/`.

Fresh Phase 5, 6, and 7 gates pass at 2,697/2,697, 1,631/1,631, and 1,739/1,739 coverage assertions, respectively. The restored focused differential is exact. Raw gate, red baseline, mutation, and restored logs are compressed under `evidence/raw/phase7-npfluid-particle-write-data-003712-003714/`.
