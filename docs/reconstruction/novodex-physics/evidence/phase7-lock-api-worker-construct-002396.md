# Phase 7 continuation: derived lock-API worker constructor (`phys_fn_002396`)

IDA at `0x1005b9a0` (RVA `0x5b9a0`) confirms the constructor delegates to base row 002377, stores its three stack arguments at owner offsets +8, +0xc and +0x14, installs vtable `0x10108894`, returns `this`, and leaves +0x10 untouched. The private helper `nxLockApiWorkerConstruct2396` implements that sequence without changing public headers.

The registered object-layout fixture compares oracle and candidate with distinct patterned allocator shims. It checks return identity, allocation size/type, worker-record initialized bytes and untouched tail, vtable, all three arguments, and the untouched +0x10 sentinel. Flipping bit 0 of the third argument reports `mismatches=1` and exits 1; restored source reports `mismatches=0` and passes the layout differential.

Phase 7 advances to 369 closed / 191 deferred. The row adds one assertion to each shared Phase 5/6/7 layout registration; the fresh full gate logs are in the raw evidence packet.
