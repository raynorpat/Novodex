# Phase 5 mutation proof: `phys_fn_001365`

`SphereShape::nxSphereZeroCenterRadius` writes three zero center components and the sphere radius. The registered `NxPhysicsObjectLayoutTests` oracle differential calls the row on a freshly constructed sphere and checks all four outputs and its combined digest against the pinned oracle.

The baseline reports digest `f9280a78`, `sphererows candidate ok=1`, and zero layout mismatches (`build/phase5-001365-baseline.log`). Changing the first zero-center output to 1.0f makes the candidate fail and yields `layout candidate mismatches=1`; the mutant exits 1 (`build/phase5-001365-mutant.log`). Restoring zero returns the baseline digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-001365-restored.log`). Public headers were not changed.
