# Lock-API word setter (`phys_fn_002398`)

IDA at RVA `0x5b9d0` shows `mov eax,[esp+4]; mov [ecx+0x10],eax; ret 4`: the row stores its argument at `this+0x10`, returns the same value, and uses the x86 thiscall cleanup. The candidate implementation is `NxLockApiWordStore2398::store` in `Physics/src/ZzLockApiByteRows.cpp`; its linker-map symbol is `?store@NxLockApiWordStore2398@@QAEII@Z`.

The registered object-layout differential calls the oracle RVA and candidate source method on separate 0xa5-filled 0x20-byte records for values `0`, `0x12345678`, and `0xffffffff`. It compares the return value and every record byte. The restored run reports `mismatches=0`. A mutation that stores `value ^ 1` is rejected with `mismatches=3` and runner exit 1.

Fresh Phase 5, 6, and 7 gates pass at `2891/2891`, `1748/1748`, and `3030/3030` coverage assertions. The source fixture pin was refreshed in `evidence/oracle-only-baselines.json`. Raw transcripts and their hashes are in [raw/phase7-lock-api-store-word-002398-20261018](raw/phase7-lock-api-store-word-002398-20261018/).
