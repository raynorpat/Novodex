# Lock API worker start rows 002379 and 002381

`phys_fn_002379` is now implemented in the production source and linked into `NxPhysics.dll`. It reads vtable slot 1, calls it with the original receiver, discards the callback result, and returns zero. The four-case fixture exercises distinct receiver/vtable pairs and verifies the selected slot, identity, one call, and return contract.

`phys_fn_002381` reads the worker record at `this+4`. With state zero it calls `CreateThread` using the receiver and the recovered worker entrypoint, stores the returned handle at record offset zero, writes state one at offset four, and returns the handle. For a nonzero state it returns the record pointer without creating another thread. The fixture compares the oracle and candidate on both branches and waits for the callback event and thread completion.

The restored oracle/source run reports `slot1wrapper candidate cases=4 failures=0 mismatches=0 digest=05167dcd`, `newreach row=002381 oracle_differential candidate=source mismatches=0`, and `layout result=differential-pass`.

Two mutations were detected. Changing the starter idle condition to `state==3` is rejected at row 002381 with `mismatches=1`. Changing the product worker entrypoint return from zero to one is rejected by the four-case slot-1 fixture with `mismatches=4`. The restored control again reports zero mismatches. Full captured output and build logs are in `evidence/raw/phase7-lock-api-worker-start-002379-002381-20261018/`.

The implementations are in `Physics/src/ZzLockApiByteRows.cpp`; declarations are private to `Physics/src/include/ObjectModel.h`. The public NxPhysics headers are unchanged.
