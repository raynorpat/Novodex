# `NxSceneInternal::releaseEffectors` mutation closure (`phys_fn_000575`)

`phys_fn_000575` is the 68-byte Scene row at RVA `0x00010970`. It repeatedly destroys the head effector through its deleting slot until the Scene list at `+0x5a4` is empty. The registered Phase 7 `NxPhysicsEffectorTests` fixture leaves effectors alive for `sdk.releaseScene`, which reaches this row during teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, an immediate return was inserted at the start of `releaseEffectors`, skipping every effector destructor. The registered differential rejected the mutant (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2`, exact stderr); the scene-release allocator record shows four fewer frees in the mutant (51 instead of 55). Mutant `NxPhysics.dll` SHA-256: `6ef2c564f91955ebb2262051ceb4c6615767b81e9b406fdcac4d224fa2a05931`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `eaea6b6b64b7bccc7f00f30c432652ad9fd241b1be2c6f06e998ea28d87d307b`.

Evidence: `build/mutation-000575/mutant-differential.log` and `build/mutation-000575/restored-differential.log`. The row is now closed in the Phase 7 ledger. Phase 8 terminal closure remains open.
