# Phase 5 mutation proof: `phys_fn_001251`

`PlaneShape::nxPlaneSaveState` writes the plane normal and the negated distance into the shape record. The registered `NxPhysicsObjectLayoutTests` oracle differential drives it on a freshly constructed plane and compares the poisoned record byte-for-byte with the pinned oracle; the zero-distance case distinguishes the sign bit.

The baseline reports saved-record digest `7629841d`, `neg_d=80000000`, and zero layout mismatches (`build/phase5-001251-baseline.log`). Removing the sign flip stores +0 instead of -0 and is caught with `layout candidate mismatches=1`; the mutant exits 1 (`build/phase5-001251-mutant.log`). Restoring the negation returns the oracle digest, negative-zero word, zero mismatches, and `layout result=differential-pass` (`build/phase5-001251-restored.log`). Public headers were not changed.
