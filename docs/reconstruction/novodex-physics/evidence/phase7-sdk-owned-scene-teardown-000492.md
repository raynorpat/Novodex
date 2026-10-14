# Phase 7 continuation: SDK-owned Scene teardown

The `PhysicsSDK` destructor (`phys_fn_000492`, oracle RVA `0x0000fb70`) now walks the remaining `mScenes` entries and calls each internal Scene's scalar deleting destructor before releasing the SDK-owned arrays and Foundation state. Public `releaseScene` remains unchanged; this covers scenes still registered when callers release the SDK.

`NxPhysicsPopulatedSceneTeardownTests` now leaves one empty Scene registered for `NxPhysicsSDK::release` and verifies that its 40-byte wrapper allocation is freed. Before the change, the candidate reported `freed=0` and exited 1 while the oracle reported `freed=1` and exited 0. With the loop restored, the focused staged-pair differential passes exactly (`stdout_delta=0`, `stderr_exact=True`). A no-op mutation of the loop is rejected (`candidate_exit=1`, `stdout_delta=2`).

This is a partial reconstruction of `phys_fn_000492`, not row closure. During the populated teardown fixture the oracle returns seven allocator blocks that the candidate still retains (12, 12, 8, 8, 16, 16, and 16 bytes). The destructor's cleanup calls for `phys_fn_004834`, `phys_fn_004828`, and `phys_fn_004149` are not yet wired into the candidate. The `phys_fn_004149` field-release helper exists in `ObjectModel.cpp`; the other two cache cleanup paths still need reconstruction and ownership tracing. Keep `phys_fn_000492` and its scalar deleting thunk `phys_fn_000494` open until these calls and the complete SDK release contract are accounted for.

The raw red, restored, and mutation differential transcripts and their SHA-256 manifest are in `evidence/raw/phase7-sdk-owned-scene-teardown-000492-20261014/`. The fresh full Phase 7 gate also passed at 1,980/1,980 coverage assertions after this change.


## 2026-10-14 cache-release follow-up

The seven retained blocks from the initial increment have since been traced and released. The OPCODE pool release already existed as `nxOpcodeReleasePool`; a mutation confirmed it is reached by SDK teardown. This continuation adds Scene shape-pair hash and trigger-pair storage cleanup and explicit release of the three BoxPruning radix-sort caches. The populated teardown differential now ends with zero tracked allocator blocks on both oracle and candidate. See [the cache-release evidence](phase7-sdk-cache-release-000492.md). The SDK destructor row remains open pending full row closure and scalar-deleting-thunk evidence.
