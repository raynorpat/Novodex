# `NxSceneInternal::releaseEffector` mutation closure (`phys_fn_000594`)

`phys_fn_000594` is the 126-byte Scene row at RVA `0x00010e80`. It checks the SDK re-entry guard, unlinks the effector, runs its deleting destructor when non-null, decrements the Scene effector count, resets the iterator to the list head, and clears the re-entry flag. The registered Phase 7 `NxPhysicsEffectorTests` fixture exercises this public release path across normal release and actor teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, an immediate return was inserted at the start of `releaseEffector`. The candidate left the effector count at 1 with iterator `np` and performed no frees; the oracle reached count 0 with iterator `none` and freed the effector. The registered differential rejected the mutant (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=48`, exact stderr). Mutant `NxPhysics.dll` SHA-256: `4daea3be1a46e4ae61f3513b5b892550bf5b98aae2acee778b811a44a4820e77`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `aa3af50af03a6ed0bbed79bad6ebd319f53aab0f064e4f1cc81c04245dd0f7ff`.

Evidence: `build/mutation-000594/mutant-differential.log` and `build/mutation-000594/restored-differential.log`. The row is now closed in the Phase 7 ledger. Phase 8 terminal closure remains open.
