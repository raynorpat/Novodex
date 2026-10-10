# Scene joint iterator reset mutation closure (`phys_fn_000563`)

`phys_fn_000563` is the 13-byte Scene row at RVA `0x00010880`. Its contract is to copy the joint-list head from `Scene + 0x59c` into the iteration cursor at `Scene + 0x6bc`, then return. The candidate map resolves `NxSceneInternal::resetJointIterator` in `Scene.obj`, implemented in `Physics/src/Scene.cpp`.

The registered Phase 7 `NxPhysicsJointStagedPairTests` baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the reset assignment was changed to clear the cursor instead of copying the list head. The joint lifecycle transcript then differed by 296 bytes; both child processes still exited zero and stderr remained exact, so the staged differential rejected the mutant.

After restoring the source assignment and rebuilding, the focused differential returned `stdout_delta=0` with exact stderr. The complete Phase 7 gate also passed: all 15 staged targets matched, 1,639 coverage assertions passed against a 1,475 floor, and both immutable public-header checks passed. The mutation is specifically aimed at this row and changes the iterator behavior observed by the fixture. Phase 8 full-DLL terminal closure remains open.
