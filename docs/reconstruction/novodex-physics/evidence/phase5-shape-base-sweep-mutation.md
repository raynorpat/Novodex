# Base `NxShape` sweep-slot mutation

Phase 5 row `phys_fn_001035` (`0x00022dd0`) is the base-shape slot-7 sweep
stub. The pinned listing is `xor al, al; ret 8`: it returns false without
changing the caller's output word. `PlaneShape`'s final vtable reuses this
base slot.

The existing `NxPhysicsShapeVtableTests` runs three plane slot-7 records,
including signed zero and a sentinel word, against the pinned oracle. With the
candidate implementation changed from `return false` to `return true`, the
oracle differential reported `shape vtable oracle_digest=ed1294b6
cases=626 failures=3` and exited 1. This demonstrates the registered cases
detect a wrong return for each record. After restoring the exact source bytes
and rebuilding, the target reported `cases=626 failures=0` and exited 0; the
mass-frame, box-sweep, and box-hull comparisons also passed.

Command used for the restored verification:

```powershell
cmake --build build --config Release --target NxPhysicsShapeVtableTests
build\Release\NxPhysicsShapeVtableTests.exe D:\FlamingEnt__\Unreal_3\Binaries <pinned-oracle-sha256>
```

The Phase 5 ledger now records 40 closed / 165 deferred function rows. This
closes only the base-shape stub row; the plane's broader behavior and all
remaining Phase 5 rows are still open. No public Physics headers changed.
