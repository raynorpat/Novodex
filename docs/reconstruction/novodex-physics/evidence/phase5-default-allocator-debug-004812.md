# Phase 5 row 004812: default allocator debug slot

`SdkDefaultAllocator::mallocDEBUG` was emitting a CRT allocation, while the pinned DLL's default allocator vtable slot 1 points to the five-argument null-return body at RVA `0x000b4070` (`phys_fn_004812`). The new `NxPhysicsObjectLayoutTests` probe resolves the candidate's `gSdkDefaultAllocator` from the CMake linker map and calls oracle and candidate vtable slot 1 for sizes 1 and `0x40`. It frees any candidate allocation through the candidate allocator's slot 3.

The test first reproduced the mismatch against the pre-fix candidate (`d6cb20975e4f1ec4a28992abb2f8e345ffc42bdc3c5ffb87cba7250e5d40c15c`): oracle returned null twice, candidate returned non-null twice, and the target exited 1 with `mismatches=2`. `SdkDefaultAllocator::mallocDEBUG` now returns null. The restored candidate (`4c93f0c7a3c03c0e525c162614efb78faee09ecd282c234472a9774a90e04fba`) reports `oracle_null=2 candidate_null=2 mismatches=0`; the oracle/candidate target is exact (`stdout_delta=0`, exact stderr).

The coverage assertion is registered for Phase 5, 6, and 7. Fresh full gates pass at 2,783/2,783, 1,683/1,683, and 2,899/2,899. The approved `NxPhysicsCoreDumpTests` date/time assertion was rerun after the allocator correction and is exact (`stdout_delta=0`, exact stderr). All 10 portable fixture source captures containing `NxSdkAllocatorAccess.inl` were refreshed and their compressed/raw hashes verified. No public SDK headers changed. The complete DLL reconstruction remains active.

Raw transcripts, candidate identities, and compressed/uncompressed hashes are in [the manifest](raw/phase5-default-allocator-debug-004812-20261015/manifest.json).
