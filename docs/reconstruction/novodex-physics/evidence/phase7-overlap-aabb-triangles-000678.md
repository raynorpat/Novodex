# `NxSceneInternal::overlapAABBTriangles` mutation proof

Date: 2026-10-14
Row: `phys_fn_000678` at RVA `0x00014990`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `dc9ba623`, with the census-to-function mapping correction recorded in this change.

For the mutation, inserted an immediate `return 0` at the start of `NxSceneInternal::overlapAABBTriangles()`. The registered raycast differential rejected it with both processes exiting 0, `stdout_delta=6`, and exact stderr. The hit fixture lost all three extracted triangles, and the translated-mesh query lost its two triangles.

Restored `Physics/src/SceneRaycast.cpp` byte-for-byte and rebuilt. The raycast differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`), including hit/miss, corner miss, output order, prefilled array, and translated mesh cases. The full Phase 7 gate passed with 1,793/1,793 coverage assertions.

This mutation verifies entry into the row and its exercised cases; selected-pruner traversal and broader edge/order behavior remain tracked as reconstruction gaps. Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-overlap-aabb-triangles-000678-20261014/`.
