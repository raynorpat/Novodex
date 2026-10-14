# Contact friction parameters row 000861 (`phys_fn_000861`)

The existing simulation friction-contact fixture reaches `cpmFrictionParams0861`. The oracle and restored candidate report a nonzero friction force (`3d5ae33a`) alongside the same normal force. Replacing only row 000861 with an empty body preserves the normal force but sets friction force to zero; the registered staged differential rejects it with both processes exiting zero and `stdout_delta=266`. Restoring `Physics/src/ContactPairManager.cpp` byte-for-byte returns exact output (`stdout_delta=0`, `stderr_exact=True`).

The full Phase 7 gate passes at 1,975/1,975 coverage assertions. Raw mutation, restored-control, and gate transcripts and the row-only patch are recorded in `evidence/raw/phase7-friction-params-000861-20261014/`. No public headers or production implementation sources changed.
