# Phase 7 pair-flag scene getters

Rows `phys_fn_000311` and `phys_fn_000315` are implemented in `NpScene.cpp` and are now mutation-falsified by the registered `NxPhysicsPairFlagTests` differential.

The baseline fixture matched the pinned UE3 oracle exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). The actor getter is observed after a valid actor pair is set to `NX_IGNORE_PAIR`; replacing its return with zero changes the transcript by 4 bytes while both processes still exit successfully. The shape getter was previously only exercised for a same-shape invalid input, which returns zero and cannot detect a no-op. The fixture now sets `NX_NOTIFY_ON_TOUCH` on two distinct shapes, asserts the returned flags and pair count, and pins `pairflag shape flags=00000008 count=2` on both staged pairs. Replacing the shape getter return with zero fails the assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=14`). Restoring the source returns to an exact differential.

The complete Phase 7 gate passes with the added required line (`coverage_assertions_evaluated=1704 floor=1704`); the valid shape flags remain `0x8` after the later contended setter is rejected. The full gate transcript is in `../raw/phase7-npscene-pair-flag-getters-000311-000315-20261012/phase7-gate.log`.

Mutation diffs are retained in this directory; focused command transcripts are in `../raw/phase7-npscene-pair-flag-getters-000311-000315-20261012/`. No public headers changed.
