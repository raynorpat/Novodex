# `nxRaycastAnyBoundsLoop` mutation proof

Date: 2026-10-14
Row: `phys_fn_000680` at RVA `0x00015060`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

Replacing the helper body with `return false` was rejected by the registered raycast differential: both processes exited 0, `stdout_delta=356`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-any-bounds-000680-20261014/`.
