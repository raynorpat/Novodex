# Candidate box primary vtable, first live dispatch slice

The Phase 5 layout gate remains red on its explicit `CANDIDATE-MISSING family=vtables` marker. This change supplies the first candidate-owned shape table instead of altering that marker. The oracle's box constructor at RVA `0x21870` installs the 17-slot primary table at RVA `0x106ab8` (see `object_model.json` and `phase5-object-model.md` §3z18). `BoxShape` now installs a 17-slot table at offset zero, in the same slot order, pointing to the existing candidate member implementations. Its `ShapeBase` member occupies offset zero, so base rows receive the same `this` address. The MSVC Win32 pointer-to-member representation is checked as one pointer at compile time.

The new `NxPhysicsBoxVtableTests` executable isolates this test from the Phase 5 layout harness's oversized `wmain`, whose stack-frame corruption is documented in the CMake comments. It validates the oracle SHA-256 before loading it, supplies the required SDK allocator shim, constructs oracle and candidate boxes, and checks that all 17 candidate slots point into the candidate executable. It then calls slots 10 and 11 through each object's installed vtable for three dimension/position fixtures and compares all four returned words bitwise. Slot 14's identity return is checked on both objects. Command and result:

```powershell
cmake --build build --config Release --target NxPhysicsBoxVtableTests
.\build\Release\NxPhysicsBoxVtableTests.exe 'D:\FlamingEnt__\Unreal_3\Binaries' '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
# box vtable cases=24 failures=0; exit 0
```

A wrong SHA exits 2. The existing `NxPhysicsObjectLayoutTests` still reports `boxshape candidate ok=1` and `CANDIDATE-MISSING family=vtables`, exiting 1 as intended. This slice does not assert semantic closure of box slots 3, 5, or 7, which remain provisional, or any of the other seven registered vtables. Completing the gate still requires their candidate tables, exact dispatch checks, and replacement of the unconditional marker with a candidate-side assertion.
