# Phase 7 row 000517: scene owner-pair entry

Oracle row phys_fn_000517 is the 24-byte entry at RVA 0x00010370 that loads the Scene pair-map count and current record pointer. The current CMake Release map resolves the combined C++ helper ?NxSceneRemoveOwnerPairRecords@@YAXPAXPBX@Z at VA 0x1005e810 in Scene.obj. The C++ helper also contains the adjacent oracle loop continuation phys_fn_000519; this closure targets only the entry's owner load, and row 000519 remains open.

The registered Phase 7 NxPhysicsSimulationTests fixture invokes row 000521, which forwards root-shape cleanup through this helper. A temporary mutation changed only the 000517 owner value read from the shape's +0xd4 field to zero. The oracle reports root_removed=1 while the candidate reports root_removed=0; the staged differential fails (oracle_exit=0, candidate_exit=1, stdout_delta=4007).

After restoring Physics/src/Scene.cpp byte-for-byte and rebuilding, both sides report root_removed=1 and no_root_preserved=1. The restored registered differential passes exactly (both exit 0, stdout_delta=0, exact stderr). The CMake candidate DLL SHA-256 is 3f38c6401b0fe2bb99774c7fa2bde71e6fddf577a8b2c370205df8b75fc57a20. No public header changed.

This closes one Phase 7 row as differential_falsified. Phase 7 now has 340 closed and 220 deferred rows; full DLL reconstruction remains open.
