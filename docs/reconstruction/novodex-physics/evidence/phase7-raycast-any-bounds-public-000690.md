# `NxSceneInternal::raycastAnyBounds` mutation proof

Date: 2026-10-14
Row: `phys_fn_000690` at RVA `0x00015520`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

An immediate `return false` mutation at the public Scene query row was rejected by the registered differential: both processes exited 0, `stdout_delta=2721`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-any-bounds-public-000690-20261014/`.
