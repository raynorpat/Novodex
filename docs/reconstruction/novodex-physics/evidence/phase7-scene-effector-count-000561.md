# `NxSceneInternal::getNbEffectors` mutation closure (`phys_fn_000561`)

`phys_fn_000561` is the seven-byte Scene getter at RVA `0x00010870`, implemented by `NxSceneInternal::getNbEffectors` in `Physics/src/Scene.cpp`. It returns the count at `Scene + 0x6c4`. The registered Phase 7 `NxPhysicsEffectorTests` fixture observes zero, one, and two live effectors across creation, release, and actor teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the getter was changed to read the adjacent joint-count field at `+0x6c8`. In the two-effector lifecycle cases the candidate reported zero; the runner rejected the candidate with both child processes exiting zero, exact stderr, and `stdout_delta=14`. Mutant `NxPhysics.dll` SHA-256: `84318d8d343642a96906dd0feb2730620548ef7dbca8677cd7fb8877cf3bbfc1`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `8b7a93a8c459c1d0567258d62309ed017d14c3e04838719f62404745d70968bd`.

The whole Phase 7 gate is required to validate the ledger update. Phase 8 terminal closure remains open.
