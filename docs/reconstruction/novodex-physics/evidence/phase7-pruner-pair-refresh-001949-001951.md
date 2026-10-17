# Coherent-pruner refresh and pool counts (`phys_fn_001949`, `phys_fn_001951`)

IDA identifies `phys_fn_001949` as the changed-object path that clears a dirty bit, refreshes bounds, and updates the coherent sweep-and-prune object. `phys_fn_001951` fills the static/dynamic pruner counts and backing-array pointers. The candidate folds both operations into `nxSceneRefreshPairs` in `Physics/src/ContactPairManager.cpp`, mapped at `0x10018070` in `ContactPairManager.obj`.

The registered `NxPhysicsSimulationTests` differential rejects both row-specific mutations. Omitting `UpdateObject` leaves the moved-apart candidate with two pairs while the oracle has one (`stdout_delta=674`). Zeroing the pruner counts breaks the trigger enter/stay/leave fixture (`stdout_delta=4099`). Restored controls match exactly for both mutations (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The complete compressed mutation and restored-run transcripts, mutation instructions, image hashes, and file hashes are in [raw/phase7-pruner-pair-refresh-001949-001951-20261017](raw/phase7-pruner-pair-refresh-001949-001951-20261017/). These row closures cover the integrated candidate behavior and do not claim the entire pair-refresh subsystem is complete.

Fresh registered gates pass with Phase 5 at 2,861/2,861, Phase 6 at 1,728/1,728, and Phase 7 at 2,997/2,997. The complete reconstruction-tool suite passes 817 tests. Both the repository and UE3 Physics public-header manifests pass for all 80 files.
