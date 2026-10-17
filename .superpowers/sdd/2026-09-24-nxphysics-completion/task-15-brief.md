# Task 15 brief — `phys_fn_000668`

Base: `1257210ae7a4be9e3b5db77d0d926ff90dd368f8`.

IDA identifies the oracle row at RVA `0x145c0` as a 34-byte `__thiscall` Scene scalar-deleting thunk. It calls `phys_fn_000663`, tests flag bit 0, conditionally frees the internal Scene through the Foundation allocator, returns `this`, and uses `ret 4`. Its vtable reference is `0x101066f4`.

The candidate is `NxSceneInternal::scalarDeletingDestructor` in `Physics/src/Scene.cpp`. The dedicated `NxPhysicsSceneScalarDeleteTests` target creates two public Scenes, obtains the internal allocation at `+0x24`, and calls the oracle RVA or candidate linker-map symbol. Flag 0 preserves the 1,808-byte object; flag 1 frees it; both return the object pointer and balance the stack.

Verification: the initial candidate probe was red for `result_self=0`; changing the private method return type to `void*` and returning `this` made the staged oracle/candidate control exact. A candidate free-disabled mutation is rejected on flag 1 (`candidate_exit=1`, `stdout_delta=2`). Phase 5 passes 2,860/2,860, Phase 7 passes 2,996/2,996, inventory validation reports zero unexplained rows, both public header trees match the pinned hashes, and the tooling suite passes 817 tests.

The full DLL reconstruction remains open. Do not alter public headers.
