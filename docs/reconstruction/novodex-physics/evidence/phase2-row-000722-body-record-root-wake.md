# Dynamic body record root and wake value (`phys_fn_000722`)

The registered `NxPhysicsBodyCreationTests` exercises the row through six public actor-creation cases and records the root-wake word at `record+0x1cc` for default, live-parameter, full-descriptor, flat-tensor, kinematic, and reused records. The restored candidate matches the oracle in every case (`stdout_delta=0`, `stderr_exact=True`).

For mutation testing, the candidate store to `root+0x1cc` was changed to `root+0x1c8`. All six recorded values diverged and the registered target rejected the mutant (`stdout_delta=24`). Restoring the store returned to the exact oracle output. The current candidate DLL map resolves `Row000722Fixture::row000722` to this implementation. Raw baseline, mutant, and restored transcripts and hashes are in [the evidence archive](raw/phase2-row-000722-20261018/manifest.json).
