# Scene collision-bounds visualization row 000638 (`phys_fn_000638`)

The pinned oracle decompilation for RVA `0x000128e0` matches the existing `nxSceneVisualizeCollisionBounds` implementation: it walks sections 1 and 2 of the static and selected dynamic pruners, checks shape visualization flag bit 3, optionally limits output to compound-group shapes (type 5), resolves each current world AABB, and submits it using the requested color. The caller ignores the renderable call’s return register.

Replacing only this helper with an immediate no-op changes the registered Phase 7 `NxPhysicsSceneVisualizeTests` transcript by 2,744 bytes (both exits 0, exact stderr). Restoring `SceneVisualize.cpp` byte-for-byte and forcing a fresh CMake rebuild returns exact output. The formal Phase 7 gate passes at 1,975/1,975 coverage assertions. The decompilation mapping, mutant, restored control, gate transcript, and hashes are in `evidence/raw/phase7-scene-visualize-bounds-000638-20261014/`. No public headers changed.


The same formal Phase 7 gate passed after commit `3f4bbdfc` was fast-forwarded to local `main`, again at 1,975/1,975 coverage assertions. Its transcript is `evidence/raw/phase7-scene-visualize-bounds-000638-20261014/post-merge-phase7-gate.log.gz`.


The empty-scene collision-AABB regression now covers the oracle helper’s null-pruner behavior. The new case is red before the guard (`oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=1703`, exact stderr); with the guard, both sides emit the same empty renderable and the focused differential is exact. The full Phase 7 gate passes at 1,977/1,977 assertions.


After merge commit `fa169984`, the fresh main-checkout Phase 7 gate also passes at 1,977/1,977 assertions. The transcript is `evidence/raw/phase7-scene-visualize-bounds-000638-20261014/post-merge-null-pruner-phase7-gate.log.gz`.
