# Lock API worker scalar-delete row 002402

IDA shows a scalar deleting thunk at RVA `0x5ba40`. It always calls the worker-record destructor at `0x5b920`; when flag bit 0 is set, it frees the owner through the Foundation SDK allocator's virtual free slot; then it returns the original owner pointer.

The product-linked candidate lives in `Physics/src/ZzLockApiByteRows.cpp`, with a private declaration in `Physics/src/include/ObjectModel.h`. `NxPhysics.map` resolves `?nxLockApiWorkerScalarDelete2402@@YIPAXPAX0I@Z` at `0x1007a020`. Public NxPhysics headers are unchanged.

The registered fixture compares oracle and candidate for flags 0 and 1. It checks return identity, record/owner allocator releases, worker-handle closure, owner-field clearing, and whether the owner survives. Restored output reports `newreach row=002402 oracle_differential candidate=source mismatches=0` and `layout result=differential-pass`. Changing the candidate flag test from `flags&1` to `flags&2` is rejected with `mismatches=1`; the rebuilt restored control passes. Captured logs are in `evidence/raw/phase7-lock-api-scalar-delete-002402-20261018/`.

The first Phase 7 run recorded a one-off access violation in the oracle `NxPhysicsPopulatedSceneTeardownTests` process. Running that fixture directly then passed with the expected `release_delta=-39`, and a full Phase 7 rerun passed all 3,032 registered assertions. Both gate logs are retained in the raw evidence directory.
