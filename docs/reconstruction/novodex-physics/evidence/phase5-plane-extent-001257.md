# Phase 5 mutation proof: `phys_fn_001257`

`PlaneShape::nxPlaneExtentRow` returns zero center words and the positive maximum finite float for the plane's unbounded reach. The registered `NxPhysicsObjectLayoutTests` oracle differential drives the function on a constructed plane and compares the candidate output with the pinned oracle.

The baseline reports digest `517c20a1`, `planeext candidate ok=1`, and zero layout mismatches (`build/phase5-001257-baseline.log`). Replacing the +FLT_MAX output word with zero changes the digest to `69691905`, reports `planeext candidate ok=0` and `layout candidate mismatches=1`, and exits 1 (`build/phase5-001257-mutant.log`). Restoring +FLT_MAX returns the original digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-001257-restored.log`). Public headers were not changed.
