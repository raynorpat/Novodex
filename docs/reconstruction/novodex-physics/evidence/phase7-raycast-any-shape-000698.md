# `nxRaycastAnyShapeLoop` mutation proof

Date: 2026-10-14
Row: `phys_fn_000698` at RVA `0x00015890`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

Replacing the helper body with `return false` was rejected by the registered raycast differential: both processes exited 0, `stdout_delta=288`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-any-shape-000698-20261014/`.
