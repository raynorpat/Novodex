# CollisionObject base scalar deleting destructor (`phys_fn_001199`)

IDA at RVA `0x00024850` shows the scalar deleting destructor restoring the `CollisionObject` base vtable, freeing the object only when flag bit 0 is set, and returning the original root pointer. The CMake-built candidate emits `??_GCollisionObject@@UAEPAXI@Z` from `ObjectModel.cpp`; its map entry is at `0x100464f0` in `build/Release/NxPhysics.map`.

The registered `NxPhysicsShapeVtableTests` fixture checks both flags. With flag 0, the oracle and candidate restore their base vtables, return their original object pointers, and preserve the x86 stack. With flag 1, it verifies the oracle call succeeds on an object allocated from the oracle module's own heap, while the candidate's SDK allocator records one free. The oracle uses `HeapFree` with its `hHeap` global at RVA `0x00128da4`, so the fixture uses that heap instead of the test executable's CRT allocator.

A candidate-only mutation that made `CollisionObject::operator delete` a no-op changes the registered marker to `collision base dtor cases=2 mismatches=1`; the test process exits 1. After restoring the allocator call, the focused oracle/candidate differential exits 0 on both sides with identical output (`stdout_delta=0`, `stderr_exact=True`). The marker is registered in the Phase 5 oracle-differential target and contributes one assertion to its floor.

Raw restored, mutation, and fresh gate logs are in `evidence/raw/phase3-collision-base-scalar-delete-001199-20261018/`.
