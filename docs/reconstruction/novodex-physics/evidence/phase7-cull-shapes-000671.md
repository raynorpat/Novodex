# `NxSceneInternal::cullShapes` mutation proof

Date: 2026-10-14
Row: `phys_fn_000671` at RVA `0x000146e0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

The registered scene-query fixture exercises one-plane culling on both half-spaces, caller-buffer results, and callback results. In a throwaway mutation, `NxSceneInternal::cullShapes()` returned immediately. The differential rejected the mutant with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=5459`, and exact stderr; the cull result, callback count, and reported shapes disappeared.

Restoring the function and rebuilding returned `stdout_delta=0` with exact stderr. The mutant and restored runner logs are compressed under `evidence/raw/phase7-cull-shapes-000671-20261014/`.
