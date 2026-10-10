# Scene get-next-joint mutation closure (`phys_fn_000567`)

`phys_fn_000567` is the 23-byte Scene row at RVA `0x000108a0`. It reads the current iterator cursor from `Scene + 0x6bc`; when non-null, it advances that cursor through `Joint + 0x10` (`mNextJoint`) and returns the current joint. At end of list it returns null. The candidate map resolves `NxSceneInternal::getNextJoint` in `Scene.obj`, implemented in `Physics/src/Scene.cpp`.

The registered Phase 7 `NxPhysicsJointStagedPairTests` baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the cursor assignment was changed to clear the cursor instead of advancing it to the next joint. Joint enumeration then diverged by 90 bytes; both child processes exited zero and stderr remained exact, so the staged differential rejected the mutant.

After restoring the linked-list advance and rebuilding, the focused differential returned `stdout_delta=0` with exact stderr. The complete Phase 7 gate passed all 15 registered targets; 1,639 coverage assertions passed against a 1,475 floor, and both immutable public-header checks passed. The mutation directly targets iterator advancement and the fixture exercises joint order and termination. Phase 8 full-DLL terminal closure remains open.
