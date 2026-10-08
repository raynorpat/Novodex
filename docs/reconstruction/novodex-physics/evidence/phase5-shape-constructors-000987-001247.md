# Phase 5 mutation proofs: `phys_fn_000987` and `phys_fn_001247`

The registered `NxPhysicsObjectLayoutTests` oracle differential drives the candidate CapsuleShape and PlaneShape constructors on poisoned buffers and compares their initialized layout with the pinned oracle.

For `phys_fn_000987`, the baseline capsule constructor digest is `ba7317e3` and the candidate matches. Changing its half-height initializer at +0xe0 from 0.0f to 1.0f yields five candidate mismatches and exits 1 (`build/phase5-000987-mutant.log`). Restoring zero returns the baseline digest and zero mismatches (`build/phase5-000987-restored.log`).

For `phys_fn_001247`, the baseline plane constructor digest is `abed37e0` and distance word is zero. Changing its distance initializer at +0xec from 0.0f to 1.0f yields three candidate mismatches and exits 1 (`build/phase5-001247-mutant.log`). Restoring zero returns the baseline digest and zero mismatches (`build/phase5-001247-restored.log`). Public headers were not changed.
