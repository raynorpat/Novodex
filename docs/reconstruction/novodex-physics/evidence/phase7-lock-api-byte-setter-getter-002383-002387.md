# Lock-API byte setter and getter rows 002383 / 002387

Rows `phys_fn_002383` (RVA `0x0005b8b0`) and `phys_fn_002387` (RVA `0x0005b8e0`) now have candidate implementations in `Physics/src/ZzLockApiByteRows.cpp`, declared in the private `ObjectModel.h`. The setter follows `[this+4]` and stores byte value 1 at offset `+8`; the getter follows the same pointer and returns that byte. Public SDK headers are unchanged.

The leaf implementations live in `Physics/src/ZzLockApiByteRows.cpp`, which sorts after `PhysicsSDK.cpp` in the DLL's source glob. Keeping them out of `ObjectModel.cpp` preserves the link-sensitive `NxCreatePhysicsSDK` RVA at `0x00055e50`; the fresh Phase 5 allocator differential confirms the candidate allocator path remains exact.

`NxPhysicsObjectLayoutTests` compares the shipped oracle calls against the candidate helpers using separate receiver and pointee buffers. The getter checks both zero and `0xee` values; the setter compares the pointed record after both implementations run. Each marker contributes to candidate failure accounting and is registered in the Phase 5, 6, and 7 coverage gates.

The row mutations are independently detected. Changing the setter's store to zero produces one mismatch for row 002383 while 002387 remains exact; changing the getter to return constant zero produces one mismatch for row 002387 while 002383 remains exact. Both candidate runs exit 1 against an oracle exit of 0. Rebuilt restored controls report zero mismatches and the object-layout differential passes.

The fresh Phase 5, 6, and 7 gates use floors of 2,775, 1,675, and 2,891 assertions. The full DLL remains incomplete; Phase 7 now has 362 closed and 198 deferred code rows. Raw mutation, restoration, build, gate, and tool-suite logs are recorded under `evidence/raw/phase7-lock-api-byte-setter-getter-002383-002387-20261015/`.
