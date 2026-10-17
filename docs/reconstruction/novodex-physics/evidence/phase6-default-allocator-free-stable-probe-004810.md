# SDK default allocator free probe (`phys_fn_004810`)

The Phase 6 allocator fixture intermittently failed because it required the next allocation to reuse the exact address just freed by the candidate CRT. In 12 fresh-process runs, one candidate allocation did not reuse that address; the other 11 did. The oracle happened to reuse its address in all runs. The failure was caused by allocator address-selection policy, not by the candidate's `free` implementation.

The fixture now patches only the candidate DLL's imported CRT `free` thunk while freeing a marked 0x80-byte block. The hook records whether the row forwards that exact pointer and calls the original CRT function, so the block is really released. A candidate no-op mutation is rejected (`forwarded_calls=0`, `pointer_match=0`, `mismatches=1`, candidate exit 1); the restored candidate forwards once and passes. The candidate's own allocation address is no longer treated as a contract.

The restored layout differential passes with the pinned oracle digest `3acf0dc7`, and the oracle proof manifest is rebound to the edited fixture source. Fresh Phase 5, 6, and 7 gates pass at 2,878/2,878, 1,738/1,738, and 3,017/3,017 assertions. All 39 gate-registry tests pass. The focused red/restored runs, mutation patch, proof artifact, and complete gate transcripts are retained under [evidence/raw/phase6-default-allocator-free-stable-probe-004810-20261017](raw/phase6-default-allocator-free-stable-probe-004810-20261017/).

This repairs the test's observation of `phys_fn_004810`; it does not alter the production allocator implementation or public Physics headers. Full DLL reconstruction remains open.
