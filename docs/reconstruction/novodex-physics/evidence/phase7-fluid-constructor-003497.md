# `phys_fn_003497` fluid constructor defaults

IDA at oracle RVA `0x00085b00` identifies the 0x84-byte Fluid constructor: it installs the vtable, initializes empty member arrays and the AABB sentinels, stores the manager, constructs the solver/read-lock pair, clears `+0x74`, and seeds `+0x80` to -1 before initialization.

The current registered `NxPhysicsSimulationTests` differential creates seven fluids and checks the constructor's arrays, bounds, manager/solver links, solver vtables and backend handle. The restored control reports `simulation fluid constructor initialized=1 fluids=7 arrays=1 bounds=1 solver-links=1` with `stdout_delta=0`. Mutating the candidate to store `1` at `fluid+0x74` after the zero initialization is detected (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2`); restoring the constructor returns to exact output. The candidate symbol resolves at `0x10060050` in the current map.

This closes the observable constructor-default slice for `phys_fn_003497` under the Phase 7 row-mutation ledger. The full-image instruction/code-generation audit and solver-base constructor audit remain Phase 8 work. No public header changed. Raw mutation/control transcripts and hashes are in `evidence/raw/phase7-fluid-constructor-003497-20261016/`.
