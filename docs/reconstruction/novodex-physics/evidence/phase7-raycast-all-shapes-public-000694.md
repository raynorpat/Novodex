# `NxSceneInternal::raycastAllShapes` mutation proof

Date: 2026-10-14
Row: `phys_fn_000694` at RVA `0x000156c0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

The image returns zero from this API regardless of the loop count, so the mutation suppressed the `nxRaycastAllShapesLoop` dispatch while preserving that return contract. The registered differential rejected the mutant: both processes exited 0, `stdout_delta=5140`, and stderr matched exactly. Restoring the dispatch matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-all-shapes-public-000694-20261014/`.
