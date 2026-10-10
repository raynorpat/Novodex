# `phys_fn_000649` Scene auxiliary manager destructor

The oracle function at RVA `0x13000` destroys the 0xa8-byte auxiliary manager stored at Scene+0x48. It frees ten array pointers through `nxFoundationSDKAllocator` in descending order at offsets `0x90` through `0x00`, clearing each pointer and its two metadata words while leaving each inter-array four-byte gap unchanged. The helper row `phys_fn_000624` performs the repeated three-word array cleanup in the original binary; the candidate expresses the complete observed order directly in `nxSceneAuxDestroy`.

`NxPhysicsObjectLayoutTests` is registered as a Phase 7 oracle differential. It creates independent populated oracle/candidate scratch managers and swaps in a counting allocator. The baseline reports `oracle_frees=10`, `candidate_frees=10`, and `mismatches=0`.

For mutation validation, omitting the candidate allocator free produced `oracle_frees=10`, `candidate_frees=0`, and `layout candidate mismatches=1`; the test exited 1. Preserving metadata word +4 produced 29 differing bytes and also exited 1. The restored differential reports zero mismatches and exits 0. The Phase 7 gate passed after registration, including all existing staged-pair, static-proof, oracle-differential, and coverage checks.
