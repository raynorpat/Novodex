# World inverse inertia tensor (`phys_fn_000746`)

The registered `NxPhysicsActorCMassTests` differential exercises the world-tensor helper through rotated center-of-mass setters and checks all nine inverse-inertia words. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate `out[0]` calculation was replaced with zero and `NxPhysics.dll` rebuilt. The registered center-of-mass differential rejected the candidate (`stdout_delta=210`). Restoring the ordered tensor calculation and rebuilding returned an exact differential.

The target is part of the passing Phase 5 gate, so the shared Phase 2 row is discharged by Phase 5. Raw baseline, mutant, and restored transcripts with hashes are in `evidence/raw/phase5-world-inertia-row-000746-20261018/manifest.json`.
