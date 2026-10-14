# `NxSceneInternal::checkOverlapSphere` mutation proof

Date: 2026-10-14
Row: `phys_fn_000672` at RVA `0x000147d0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `c6cd384a`.

For the mutation, inserted an immediate `return false` at the start of `NxSceneInternal::checkOverlapSphere()`. The registered raycast differential rejected it with both processes exiting 0, `stdout_delta=66`, and exact stderr. The staged fixture observes primitive overlap results and mesh bounds-only behavior for normal and 90-degree rotated meshes, including the rotated-bound tangent hit and nearby miss; the mutant returned false for every case.

Restored `Physics/src/SceneRaycast.cpp` byte-for-byte and rebuilt. The raycast differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`); the rotated mesh-bound, tangent, and outside markers all match. The full Phase 7 gate passed with 1,793/1,793 coverage assertions. The restored candidate DLL SHA-256 was `018ce2a8f6355f4d03dfac0aaa5198fad4c76670ba5f6372c83db14f14ca51f9`.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-check-overlap-sphere-000672-20261014/`.
