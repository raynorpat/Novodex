# Force-at-position torque (`phys_fn_000791`)

The registered `NxPhysicsActorForceTests` differential exercises the shared `Row000791Fixture::row000791` path through `NxActor::addForceAtPos` and checks linear force, angular torque, and force mode results. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate X component of `d x force` was replaced with zero and `NxPhysics.dll` rebuilt. The `force atpos global_force` marker changed from torque X `3f800000` to `0`; **stdout_delta=24** and the registered gate rejected the mutant. Restoring the cross-product expression and rebuilding returned an exact differential.

The target belongs to the already-passed Phase 5 gate, so this shared Phase 2 row is discharged by Phase 5. Raw baseline, mutant, and restored transcripts with hashes are in `evidence/raw/phase5-actor-force-torque-row-000791-20261018/manifest.json`.
