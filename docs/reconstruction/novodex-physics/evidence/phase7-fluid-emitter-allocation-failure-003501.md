# Phase 7 `phys_fn_003501`: public emitter allocation failure

The `NxPhysicsSimulationTests` fixture now reaches the 0x78-byte internal emitter-record allocation through the public `NxFluid::createEmitter` API. A one-shot allocator failure must return null, retain all three existing internal handles and public wrapper entries in their original arrays, and avoid both the rest-particle-distance and emitter-create backend callbacks. The test then continues through its existing successful create/release checks.

The focused staged-pair differential matches the pinned DLL exactly: both sides report `null=1 fail-count=1 count=3 array-match=1 callbacks-unchanged=1`, with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. A temporary candidate mutation changed the allocation-failure return from null to a non-null sentinel. The new assertion rejected it (`null=0`, `array-match=0`; candidate exit 1, `stdout_delta=72`, stderr differed). Restoring the source and rebuilding returned the focused differential to an exact match.

The fresh full Phase 7 CMake/Win32 Release gate passed with 2,913/2,913 registered coverage assertions. This evidence covers the public allocation-failure arm only; `phys_fn_003501` and its helper dependencies remain open for full contract and dependency review. Public Physics headers are unchanged.
