# CMass quaternion update (`phys_fn_000756`)

The registered `NxPhysicsActorCMassTests` differential exercises `setCMassGlobalPose` and `setCMassGlobalOrientation`, printing the four quaternion words at record `+0x124` across general rotations and all three pivot arms. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate quaternion X word was overwritten with zero after the row conversion and `NxPhysics.dll` rebuilt. The registered CMass differential rejected the changed quaternion outputs (`stdout_delta=40`). Removing the mutation and rebuilding returned an exact differential.

The target belongs to the passing Phase 5 gate, so this shared Phase 2 row is discharged by Phase 5. Raw baseline, mutant, and restored transcripts with hashes are in `evidence/raw/phase5-cmass-quaternion-row-000756-20261018/manifest.json`.
