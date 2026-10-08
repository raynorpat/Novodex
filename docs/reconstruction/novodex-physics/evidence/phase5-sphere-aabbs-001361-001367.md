# Phase 5 mutation proofs: `phys_fn_001361` and `phys_fn_001367`

The registered `NxPhysicsObjectLayoutTests` oracle differential drives both sphere bounds functions on a freshly constructed sphere and compares their outputs and digests with the pinned oracle.

For `phys_fn_001361`, the baseline world-AABB digest is `e2ba14a5` and the candidate matches. Adding 1.0f to the first world-AABB output changes the candidate digest to `07257e18`, produces `aabbrows candidate ok=0` and `layout candidate mismatches=1`, and exits 1 (`build/phase5-001361-mutant.log`). Restoring the subtraction returns the baseline digest and zero mismatches (`build/phase5-001361-restored.log`).

For `phys_fn_001367`, the baseline local-AABB digest is `33c61825` with the expected negative-zero minimum words. Changing the first negative-radius store to positive radius changes the digest to `f40ecfa5`, produces `sphlocal candidate ok=0` and `layout candidate mismatches=1`, and exits 1 (`build/phase5-001367-mutant.log`). Restoring the negation returns the baseline digest and zero mismatches (`build/phase5-001367-restored.log`). Public headers were not changed.
