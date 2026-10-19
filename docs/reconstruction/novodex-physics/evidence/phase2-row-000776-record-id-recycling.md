# DynamicBody record ID recycling (`phys_fn_000776`)

The registered `NxPhysicsBodyCreationTests` differential covers `DynamicBody::destruct` through public actor release. It releases actor C, creates another actor, and prints the recycled record ID. The pinned oracle and restored candidate both report `bodycreate reused_id=2`; the clean run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate's `nxU32VectorPushBack(scene + 0x6f8, bodyWord(this, 0x11c))` call was omitted and `NxPhysics.dll` rebuilt. The `phys_fn_000776` oracle reported ID 2 while the mutant reported ID 5; **stdout_delta=10** and the registered differential rejected the mutant. Restoring the call and rebuilding returned an exact differential.

The row is discharged by the passing Phase 5 gate, which registers `NxPhysicsBodyCreationTests`; it also drives Phase 7. No public header changed. Raw baseline, mutant, and restored transcripts and hashes are in `evidence/raw/phase2-row-000776-20261018/manifest.json`.
