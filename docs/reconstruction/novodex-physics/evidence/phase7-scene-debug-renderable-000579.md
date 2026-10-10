# `NxSceneInternal::getDebugRenderable` mutation closure (`phys_fn_000579`)

`phys_fn_000579` is the 55-byte Scene row at RVA `0x00010a10`. It lazily creates the Scene's `NxDebugRenderable` through the Foundation interface, stores it at `Scene + 0x6b8`, and returns the cached pointer. The registered Phase 7 `NxPhysicsSceneVisualizeTests` fixture exercises the empty, scale, world-axis, and populated-geometry visualization stages and releases the Scene afterward.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, an immediate null return was inserted at the start of `getDebugRenderable`. The visualization fixture then crashed in the candidate (`oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=869`, exact stderr). Mutant `NxPhysics.dll` SHA-256: `cf1b5d1b82094c2db7161a20caa299fdda7ff1bc5e5113a5d0afc2f13e2f705e`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `db8d30e0e823ebc91c6f69182d009e518f8e589795ee72a1b2eaf2001abc46a9`.

Evidence: `build/mutation-000579/mutant-differential.log` and `build/mutation-000579/restored-differential.log`. The row is now closed in the Phase 7 ledger. Phase 8 terminal closure remains open.
