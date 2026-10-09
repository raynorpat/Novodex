# Phase 6 row closure: phys_fn_003952

phys_fn_003952 (RVA 0x0008f0f0, 4 bytes) is NpSpringAndDamperEffector::getInternal. Its contract is a direct return of the internal SpringAndDamperEffector pointer stored at +0x14.

## Mutation and registered gate

A throwaway git archive copy of the current source was configured and built with CMake for NxPhysics and NxPhysicsEffectorTests. In that copy only, getInternal was changed to return mInternal + 4 bytes. The registered Phase 6 staged-pair differential exercises the getter while the effector harness inspects internal state.

The oracle completed normally; the mutated candidate was rejected after an access violation:

differential target=NxPhysicsEffectorTests oracle_exit=0 candidate_exit=-1073741819 both_exit_zero=False stdout_delta=49 stderr_exact=True

The restored clean worktree passed the same registered differential:

differential target=NxPhysicsEffectorTests oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=0 stderr_exact=True

The restored run is retained at build/phase6-row003952-restored.log. The mutation was isolated to the throwaway archive; no production source change was needed.

This closes the row as differential_falsified under NxPhysicsEffectorTests. Phase 6 remains pending.
