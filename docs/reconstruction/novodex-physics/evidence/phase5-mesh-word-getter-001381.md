# Phase 5 mutation proof: `phys_fn_001381`

`MeshShape::nxMeshGetMeshWord` dereferences the mesh-data pointer at shape offset `+0xe0` and returns the dword at mesh offset `+0xe4`. The registered `NxPhysicsObjectLayoutTests` oracle differential plants a sentinel at that location and compares the reconstructed getter.

Changing the getter to read `mesh+0xe0` yields `morerows candidate okMesh=0`, `layout candidate mismatches=1`, and exit 1 (`build/phase5-001381-mutant.log`). Restoring `mesh+0xe4` yields `okMesh=1`, zero mismatches, and `layout result=differential-pass` (`build/phase5-001381-restored.log`). Only the private implementation header was probed; public NxPhysics headers were not changed.
