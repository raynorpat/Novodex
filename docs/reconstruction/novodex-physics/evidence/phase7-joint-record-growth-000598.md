# Joint record growth row 000598 (`phys_fn_000598`)

The registered `NxPhysicsJointSlotTests` creates enough joints to reach `NxSceneInternal::growJointRecords`. Replacing only this row with an empty body causes the candidate to access-violate (`candidate_exit=-1073741819`) while the oracle exits 0; the staged differential reports `stdout_delta=1427` with exact stderr. Restoring `Scene.cpp` byte-for-byte returns an exact differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).

The full Phase 7 gate passed after restoration at 1,975/1,975 assertions. The mutation, restored control, and full gate transcripts are preserved in `evidence/raw/phase7-joint-record-growth-000598-20261014/`. No public headers or production implementation sources changed.
