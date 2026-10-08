# Phase 5 mutation proof: `phys_fn_000977`

`BoxShape::BoxShape` initializes its three default hull dimensions to 1.0f after constructing the embedded collision object and setting final vtables. The registered `NxPhysicsObjectLayoutTests` oracle differential checks the constructed object's digest, dimensions, sentinel, untouched poison, and collision-object links against the pinned oracle.

The baseline reports digest `ac5ed12f`, `boxshape candidate ok=1`, and zero layout mismatches (`build/phase5-000977-baseline.log`). Changing the default X dimension at +0xe4 from 1.0f to 2.0f changes the digest to `6a79821e`, reports `boxshape candidate ok=0` and `layout candidate mismatches=4`, and exits 1 (`build/phase5-000977-mutant.log`). Restoring 1.0f returns the original digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-000977-restored.log`). Public headers were not changed.
