# Phase 7 row 000519: scene owner-pair loop continuation

Oracle row phys_fn_000519 is the 78-byte loop continuation at RVA 0x00010390. It compares both owner keys, releases an eligible payload, removes matching records, and advances the cursor only for retained records. The candidate map resolves the combined C++ helper ?NxSceneRemoveOwnerPairRecords@@YAXPAXPBX@Z at VA 0x1005da80; its entry row 000517 is independently closed.

The registered Phase 7 NxPhysicsSimulationTests fixture reaches this continuation through row 000521. A temporary mutation suppressed only the NxRemoveCollisionPairRecord operation. The candidate then reported root_removed=0 while the oracle reported root_removed=1, and the staged differential failed (oracle_exit=0, candidate_exit=1, stdout_delta=4007).

After restoring Physics/src/Scene.cpp byte-for-byte and rebuilding, both sides report root_removed=1 and no_root_preserved=1. The restored registered differential passes exactly (both exit 0, stdout_delta=0, exact stderr). The CMake candidate DLL SHA-256 is cce9d2ee309ccd0d7043f19a86de5e55f655ec5743d1314e1c64f17f86d9735f. No public header changed.

This closes one Phase 7 row as differential_falsified. Phase 7 now has 341 closed and 219 deferred rows; full DLL reconstruction remains open.
