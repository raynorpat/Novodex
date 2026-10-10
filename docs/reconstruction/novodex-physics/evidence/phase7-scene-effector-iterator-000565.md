# `NxSceneInternal::resetEffectorIterator` mutation closure (`phys_fn_000565`)

`phys_fn_000565` is the thirteen-byte Scene row at RVA `0x00010890`. It copies the effector-list head from `Scene + 0x5a4` into the iteration cursor at `Scene + 0x6c0`. The Phase 7 `NxPhysicsEffectorTests` fixture observes iteration order in one- and two-effector lifecycle states, including release and actor teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the reset assignment was changed to clear the cursor to null. The candidate then reported `iterator=none` in the create, cycle, release, actor-release, and live states where the oracle reports the linked effectors. The registered differential rejected the mutant with both child processes exiting zero, exact stderr, and `stdout_delta=14`. Mutant `NxPhysics.dll` SHA-256: `0699857de65c749b4a83795a872312b580cf2b68f99641871916a3ddb58e8896`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `a28439dce15708b30ae7fd502ace5be25a76981abd27f6e39b83b62b9b6151e3`.

The full Phase 7 gate remains required to validate the ledger update. Phase 8 terminal closure remains open.
