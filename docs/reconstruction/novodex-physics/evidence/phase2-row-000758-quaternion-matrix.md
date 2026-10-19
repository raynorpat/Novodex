# Quaternion-derived body matrix (`phys_fn_000758`)

The registered `NxPhysicsJointSlotTests` differential exercises `Row000758Fixture::row000758` while building joint-slot body records. The clean oracle/candidate run reports `stdout_delta=0` and exact stderr.

For the mutation control, the candidate assignment to matrix element `m[0]` was replaced with zero and `NxPhysics.dll` rebuilt. `NxPhysicsJointSlotTests` rejected the changed matrix transcript (`stdout_delta=2795`). Restoring the assignment and rebuilding returned an exact differential.

The restored full Phase 6 gate passed at **1,819/1,819 coverage assertions**, including the registered joint-slot target. Phase 6 is now recorded as passed, allowing this shared Phase 2 row to be discharged by Phase 6. No public headers changed. Raw baseline, mutant, restored, and full-gate transcripts are hashed in `evidence/raw/phase6-joint-slot-row-000758-20261018/manifest.json`.
