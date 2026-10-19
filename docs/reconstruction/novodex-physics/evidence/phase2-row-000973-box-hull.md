# Box hull rebuild (`phys_fn_000973`)

The registered `NxPhysicsSceneRaycastTests` differential creates and resizes box shapes, then checks ray hit results against the oracle. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate +X box-face distance was replaced with zero and `NxPhysics.dll` rebuilt. The registered raycast differential rejected the changed hull behavior (`stdout_delta=18`). Restoring the face-plane calculation and rebuilding returned an exact differential.

The target is part of the passing Phase 5 gate, so this shared Phase 2 row is discharged by Phase 5. Raw baseline, mutant, and restored transcripts and hashes are in `evidence/raw/phase5-box-hull-row-000973-20261019/manifest.json`.
