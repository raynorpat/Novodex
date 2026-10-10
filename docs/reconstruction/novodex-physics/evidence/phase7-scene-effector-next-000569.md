# `NxSceneInternal::getNextEffector` mutation closure (`phys_fn_000569`)

`phys_fn_000569` is the 23-byte Scene row at RVA `0x000108c0`. It returns the effector at `Scene + 0x6c0`, advances the cursor to the node's `+0x18` next link, and returns null at the end. The Phase 7 `NxPhysicsEffectorTests` fixture exercises it in create, multi-effector cycle, release, actor-release, and live states; the candidate breakpoint was previously observed 20 times in this target.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the non-null path was changed to return null after advancing the cursor. The candidate then reported `iterator=none` in the states where the oracle reports effectors. The registered differential rejected the mutant with both child processes exiting zero, exact stderr, and `stdout_delta=14`. Mutant `NxPhysics.dll` SHA-256: `c76176f763e1f3f81afc8097f8031d4632411f1dbc074cf2d1daf2e4e8020d22`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `b8cea7cf84bbb9fb29fd6c245582d41763870591786a93a90cc8a4dde2690730`.

The full Phase 7 gate remains required to validate the ledger update. Phase 8 terminal closure remains open.
