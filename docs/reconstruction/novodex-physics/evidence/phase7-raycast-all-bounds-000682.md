# `nxRaycastAllBoundsLoop` mutation proof

Date: 2026-10-14
Row: `phys_fn_000682` at RVA `0x00015150`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

Replacing the helper body with `return 0` was rejected by the registered raycast differential: both processes exited 0, `stdout_delta=5106`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-all-bounds-000682-20261014/`.
