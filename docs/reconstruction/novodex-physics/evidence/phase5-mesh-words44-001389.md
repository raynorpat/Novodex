# Phase 5 mutation proof: `phys_fn_001389`

`MeshShape::nxMeshGetWords44` copies six dwords from mesh offset `+0x44`. The registered `NxPhysicsObjectLayoutTests` oracle differential plants distinct sentinel words and compares the reconstructed six-word output digest.

Changing the copy source to `mesh+0x48` yields `meshwords44 candidate ok=0`, a different digest, and `layout candidate mismatches=1` (`build/phase5-001389-mutant.log`). Restoring `mesh+0x44` reproduces the oracle digest and words, zero mismatches, and `layout result=differential-pass` (`build/phase5-001389-restored.log`). Public headers were not changed.
