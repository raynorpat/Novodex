# Phase 5 mutation proof: `phys_fn_001387`

`MeshShape::nxMeshGetWords5C` copies four dwords from mesh offset `+0x5c`. The registered `NxPhysicsObjectLayoutTests` oracle differential plants distinct sentinel words and compares the reconstructed output digest and boundary words.

Changing the copy source to `mesh+0x60` yields `meshrows candidate ok=0`, a different digest, and `layout candidate mismatches=1` (`build/phase5-001387-mutant.log`). Restoring `mesh+0x5c` reproduces the oracle digest and words, zero mismatches, and `layout result=differential-pass` (`build/phase5-001387-restored.log`). Public headers were not changed.
