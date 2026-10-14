# `NxSceneInternal::overlapSphereShapes` mutation proof

Date: 2026-10-14
Row: `phys_fn_000672` at RVA `0x000147d0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `dc9ba623`, with the census-to-function mapping correction recorded in this change.

For the mutation, inserted an immediate `return 0` at the start of `NxSceneInternal::overlapSphereShapes()`. The registered raycast differential rejected it with both processes exiting 0, `stdout_delta=5465`, and exact stderr. The fixture's caller-buffer collection and callback cases changed from two shapes (`s_box.s_plane`) to zero.

Restored `Physics/src/SceneRaycast.cpp` byte-for-byte and rebuilt. The raycast differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`), with two collected shapes and one callback carrying two entities. The full Phase 7 gate passed with 1,793/1,793 coverage assertions.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-overlap-sphere-shapes-000672-20261014/`.
