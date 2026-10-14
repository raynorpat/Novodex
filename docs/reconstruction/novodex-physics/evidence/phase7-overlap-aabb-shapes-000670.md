# `NxSceneInternal::overlapAABBShapes` mutation proof

Date: 2026-10-14
Row: `phys_fn_000670` at RVA `0x000145f0`
Gates: registered `NxPhysicsSceneRaycastTests` and `NxPhysicsSimulationTests` staged-pair differentials (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `44d9a47e`.

For the mutation, inserted an immediate `return 0` at the start of `NxSceneInternal::overlapAABBShapes()` and rebuilt the Phase 7 candidate. The raycast differential showed `stdout_delta=5473` with both processes exiting 0 and exact stderr. The simulation differential also rejected the mutation: oracle exit 0, candidate exit 1, `stdout_delta=42`, and exact stderr. This falsifies the selected-pruner AABB overlap result path through actual public callers.

Restored `Physics/src/SceneRaycast.cpp` byte-for-byte and rebuilt. Both registered differentials then matched exactly (`stdout_delta=0`, `stderr_exact=True`), and the full Phase 7 gate passed with 1,793/1,793 coverage assertions. The restored candidate DLL SHA-256 was `7d8b2cb1810c69097e64ee899ca7082d7dd4827a98b0dcf345dd386824c55bc7`.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-overlap-aabb-000670-20261014/`. Rotated mesh-bound and near-boundary overlap cases tracked on neighboring rows remain open.
