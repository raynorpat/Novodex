# Phase 5 mutation proof: `phys_fn_001003`

`CapsuleShape::nxCapsuleZeroCenterRadius` writes three zero center components and the radius sum. The registered `NxPhysicsObjectLayoutTests` oracle differential drives this row on a freshly constructed capsule and compares the candidate's four output words and aggregate digest with the pinned oracle.

The baseline reports capsule digest `0b2ae445`, `aabbrows candidate ok=1`, and `layout candidate mismatches=0` (`build/phase5-001003-baseline.log`). Changing the first output store from `0.0f` to `1.0f` changes the capsule digest to `28111758`, reports `aabbrows candidate ok=0` and `layout candidate mismatches=1`, and exits 1 (`build/phase5-001003-mutant.log`). Restoring the zero store returns the original digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-001003-restored.log`). Public headers were not changed.
