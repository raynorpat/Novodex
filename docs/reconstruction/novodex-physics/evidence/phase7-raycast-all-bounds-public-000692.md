# `NxSceneInternal::raycastAllBounds` mutation proof

Date: 2026-10-14
Row: `phys_fn_000692` at RVA `0x000155f0`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

An immediate `return 0` mutation at the public Scene query row was rejected by the registered differential: both processes exited 0, `stdout_delta=5103`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-raycast-all-bounds-public-000692-20261014/`.
