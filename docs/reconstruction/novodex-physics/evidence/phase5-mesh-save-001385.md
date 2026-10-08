# Phase 5 mutation proof: `phys_fn_001385`

`MeshShape::nxMeshSaveState` writes the mesh-data word from `mesh+0xe4` to descriptor offset `+0x4c`, writes shape flags to `+0x50`, and delegates the base state. The registered `NxPhysicsObjectLayoutTests` oracle differential plants a mesh record and compares the resulting record digest and words.

Changing the source read from `mesh+0xe4` to `mesh+0xe0` yields `meshrows candidate ok=0`, a changed record digest, and `layout candidate mismatches=1` (`build/phase5-001385-mutant.log`). Restoring `mesh+0xe4` reproduces the oracle record digest and words, zero mismatches, and `layout result=differential-pass` (`build/phase5-001385-restored.log`). Public headers were not changed.
