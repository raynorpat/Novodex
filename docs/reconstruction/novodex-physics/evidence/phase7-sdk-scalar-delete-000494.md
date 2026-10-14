# Phase 7 row 000494: PhysicsSDK scalar deleting destructor

`phys_fn_000494` is the 34-byte scalar deleting destructor at oracle RVA `0x0000fce0`. The candidate emits it as the MSVC thunk `??_GPhysicsSDK@@UAEPAXI@Z` at RVA `0x000541c0`. The thunk calls `PhysicsSDK::~PhysicsSDK`, then frees the 56-byte SDK object only when the scalar delete flag is set.

The populated teardown fixture calls the SDK vtable's deleting-destructor slot with flag zero. It verifies that the call returns the same object and that the 56-byte allocation remains live, then releases the test allocation itself. The pinned oracle and restored candidate both report `teardown sdk_scalar_delete flag=0 bytes=56 preserved=1 result_self=1`; the focused differential exits zero on both sides with `stdout_delta=0` and exact stderr.

For the row-specific negative control, the candidate thunk's `test al, 1` was changed to `or al, 1` at candidate RVA `0x000541c0` (raw file offset `0x000535ed`). This forces the flag-zero path to free the allocation. The fixture rejects that candidate with `FAIL PhysicsSDK scalar deleting destructor freed storage for flag zero`, `preserved=0`, `result_self=1`, and exit 1 (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). The staged mutant differential reports `stdout_delta=2` and non-exact stderr. The mutant candidate SHA-256 was `931744b5e0e0b36bf9c8f5ea4af9b0ef16b6f8eaeb55ad436b61615292ee0b97`; the restored candidate SHA-256 is `089bc262987fa599810c88c45ef3620b49943c415c8b78d7897a6884fb9cbe46`.

All nine populated-SDK teardown invariants are registered in both Phase 3 and Phase 7. Fresh Phase 3 passes at `954/954`; fresh Phase 7 passes at `1,989/1,989`. The coverage registry suite passes all 39 tests. Raw transcripts and their hashes are recorded in `evidence/raw/phase7-sdk-scalar-delete-000494-20261014/`.

This closes the scalar deleting thunk only; `phys_fn_000492` and the full-DLL reconstruction remain open. No public Physics headers changed.
