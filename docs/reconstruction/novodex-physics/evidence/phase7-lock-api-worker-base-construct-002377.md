# Phase 7 continuation: lock-API worker base constructor (`phys_fn_002377`)

IDA at `0x1005b820` (RVA `0x5b820`) confirms the base constructor installs vtable `0x1010886c`, allocates 0x14 bytes through allocator slot +8 with type 0, stores the record pointer at owner+4, clears record dwords at +0/+4 and byte +8, then returns `this`. `nxLockApiWorkerBaseConstruct2377` implements this privately without public-header changes.

The registered object-layout fixture calls the oracle row and candidate helper with distinct patterned allocators. It checks return identity, vptr, allocation count/size/type, the first nine zeroed bytes, and untouched tail patterns. A byte-8-to-1 mutant reports `mismatches=1` and exits 1; restored source reports `mismatches=0` and `layout result=differential-pass`.

Phase 7 advances to 368 closed / 192 deferred. The row marker adds one assertion to each shared phase registration; fresh full gates are recorded in the raw evidence packet.
Fresh Phase 5, Phase 6 and Phase 7 gates pass at 2,781/2,781, 1,681/1,681 and 2,897/2,897 assertions. The full 816-test reconstruction-tool suite passes.
