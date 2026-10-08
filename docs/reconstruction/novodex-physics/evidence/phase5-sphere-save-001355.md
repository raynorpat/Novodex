# Phase 5 mutation proof: `phys_fn_001355`

`SphereShape::nxSphereSaveState` writes the sphere radius into the descriptor record at +0x4c, then delegates to the base save-state row. The registered `NxPhysicsObjectLayoutTests` oracle differential drives it on a freshly constructed sphere and compares the poisoned record with the pinned oracle.

The baseline reports digest `f9280a78` and zero layout mismatches (`build/phase5-001355-baseline.log`). Changing the radius destination to +0x48 changes the candidate digest to `bad266b8` and yields `layout candidate mismatches=1`; the mutant exits 1 (`build/phase5-001355-mutant.log`). Restoring +0x4c returns the baseline digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-001355-restored.log`). Public headers were not changed.
