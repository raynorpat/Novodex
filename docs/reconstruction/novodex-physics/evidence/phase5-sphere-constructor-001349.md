# Phase 5 mutation proof: `phys_fn_001349`

`SphereShape::SphereShape` initializes radius to zero, allocates and constructs the embedded collision object, and sets the shape sentinel. The registered `NxPhysicsObjectLayoutTests` oracle differential drives the constructor on a poisoned buffer and compares its digest, radius, sentinel, and collision-object back-pointers with the pinned oracle.

The baseline reports digest `37ea7205`, radius word `00000000`, and zero layout mismatches (`build/phase5-001349-baseline.log`). Changing the radius initializer at +0xe0 to 1.0f yields `layout candidate mismatches=4` and exits 1 (`build/phase5-001349-mutant.log`). Restoring zero returns the baseline digest, zero mismatches, and `layout result=differential-pass` (`build/phase5-001349-restored.log`). Public headers were not changed.
