# Phase 6 row closure: phys_fn_004097

phys_fn_004097 (0x00095e50, 1176 bytes) maps to Joint::refreshBodyFrame in Physics/src/core/Joint.cpp. The routine refreshes a joint's world-space body frame and is shared by the joint families represented in the rotated multi-family joint differential.

## Mutation and gate

A throwaway git archive copy of the current source was configured and built with CMake for NxPhysics and NxPhysicsJointTests. In that copy only, the assignment storing the refreshed world-anchor X coordinate was changed to add 1.0 before the store. The registered staged-pair differential was run against the pinned oracle transcript.

The gate caught the mutation while both executables exited successfully:

```text
differential target=NxPhysicsJointTests oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=404 stderr_exact=True
```

The normal restored worktree build passed the same differential with stdout_delta=0, both executables exiting zero, and exact stderr. The mutation was isolated to the throwaway archive; no source mutation remains. Clean run output is retained at build/phase6-row004097-stagedpair-restored.log; the archived staged-pair mutation transcript is recorded as stdout_delta=404 in the closure ledger and was verified from the throwaway archive run log.

This closes the row as differential_falsified under NxPhysicsJointTests. The rest of Phase 6 remains pending.
