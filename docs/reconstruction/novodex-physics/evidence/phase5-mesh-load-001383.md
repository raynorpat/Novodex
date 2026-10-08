# Phase 5 mutation proof: `phys_fn_001383`

`MeshShape::nxMeshLoadFromDesc` follows the descriptor wrapper to the inner mesh data, stores that pointer in the shape, increments the inner mesh reference count at `+0x74`, loads shape flags, and applies the base descriptor. The registered `NxPhysicsObjectLayoutTests` oracle differential verifies the bound pointer and refcount.

Changing the refcount increment from `+1` to `+2` yields `meshload candidate ok=0`, `layout candidate mismatches=1`, and exit 1 (`build/phase5-001383-mutant.log`). Restoring `+1` returns the expected binding and refcount, zero mismatches, and `layout result=differential-pass` (`build/phase5-001383-restored.log`). Public headers were not changed.
