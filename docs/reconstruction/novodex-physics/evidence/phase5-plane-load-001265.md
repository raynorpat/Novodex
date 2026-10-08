# Phase 5 mutation proof: `phys_fn_001265`

`PlaneShape::nxPlaneLoadFromDesc` reads the descriptor normal at `+0x4c`, reads the distance at `+0x58`, applies the plane equation, and then applies the base descriptor. The registered `NxPhysicsObjectLayoutTests` oracle differential drives this row on an initialized plane and checks the loaded normal against the pinned oracle.

Shifting the normal input from `descriptor+0x4c` to `descriptor+0x50` yields `planeload candidate ok=0`, `layout candidate mismatches=1`, and exit 1 (`build/phase5-001265-mutant.log`). Restoring `+0x4c` returns Y-normal bits `3f800000`, zero mismatches, and `layout result=differential-pass` (`build/phase5-001265-restored.log`). Public headers were not changed.
