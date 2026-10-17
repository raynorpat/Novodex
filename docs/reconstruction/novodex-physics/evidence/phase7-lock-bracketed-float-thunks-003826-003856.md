# Lock-bracketed float thunk rows `003826`–`003856`

The Phase 7 object-layout differential already exercised the six three-argument wrappers at `003826`, `003828`, `003836`, `003840`, `003844`, and `003848`, plus `003854` and `003856`. IDA confirms each wrapper acquires the lock at `self+0x10`, calls its row-specific thunk using `self+0x14`, preserves the float result across the unlock, and returns it. The first six and last two share the reconstructed `nxLockedThunkFloat` candidate body.

The zero-failure lines are now required assertions in `gate_targets.ps1`, so a failure in either family makes the Phase 5, Phase 6, and Phase 7 gates red. Fresh gates pass at 2,848/2,848, 1,722/1,722, and 2,981/2,981 assertions, respectively.

Mutation sensitivity was measured in a throwaway `git archive` copy. Flipping one bit in the candidate helper's first forwarded argument caused its oracle differential to report row-local mismatches for all eight target rows. The first family reported nine failures because the same helper probe also covers three rows outside this Phase 7 set; the second family reported two failures, exactly `003854` and `003856`. The oracle-side candidate failure guard rejected the mutant. Restoring the exact source and forcing a rebuild returned both family markers to zero, and the staged oracle/candidate differential passed with exact output.

Raw baseline, mutation, restored-control, and Phase 7 gate logs are compressed under [evidence/raw/phase7-lock-bracketed-float-thunks-003826-003856-20261017](raw/phase7-lock-bracketed-float-thunks-003826-003856-20261017/).
