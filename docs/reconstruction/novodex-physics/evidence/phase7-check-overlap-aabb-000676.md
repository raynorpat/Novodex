# `NxSceneInternal::checkOverlapAABB` mutation proof

Date: 2026-10-14
Row: `phys_fn_000676` at RVA `0x00014930`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `9377c7b2`.

For the mutation, changed `NxSceneInternal::checkOverlapAABB()` to return false for every query. The registered raycast differential rejected it with both processes exiting 0 but `stdout_delta=14` and exact stderr. In the candidate, the fixture's overlapping static-and-dynamic query changed from `scene_overlap aabb=0 type=3 result=1` to `result=0`; the mutated full Phase 7 gate also failed its required scene-raycast coverage marker.

Restored `Physics/src/SceneRaycast.cpp` byte-for-byte and rebuilt. The raycast differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`), and the full Phase 7 gate passed with 1,793/1,793 coverage assertions. The restored candidate DLL SHA-256 was `ec7804a77f8279b29bd7c7c41a0e20a1712e447979065ad5b61508087089a862`.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-check-overlap-aabb-000676-20261014/`.
