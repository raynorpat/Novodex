# Prunable owner-notify adapter mutation — `phys_fn_001271`

Date: 2026-10-08

The adapter at RVA `0x00025520` dispatches the owner's vtable slot 10 with the original owner and AABB arguments. ShapeBase construction installs it in the `gPrunableOwnerNotify` callback. The prior constructor check established only that the global was non-null; it did not execute the dispatch.

`NxPhysicsShapeVtableTests` now invokes the oracle and candidate adapters against a synthetic owner whose slots 9 and 10 have distinct callbacks. Both restored implementations call slot 10 once and forward the same owner and AABB pointer. The required marker is `shape vtable owner_notify oracle_slot=10 candidate_slot=10 oracle_calls=1 candidate_calls=1 owner_forwarded=1 box_forwarded=1 mismatches=0`.

For row-specific falsification, `shapeOwnerNotify` was temporarily changed to dispatch slot 9. The target reported `oracle_slot=10 candidate_slot=9` and `mismatches=1`, then exited 1. Restoring slot 10 returned the target to zero mismatches. No public Physics headers changed.

The fresh Win32 Release Phase 5 gate passes all 19 targets and 2,565/2,565 registered coverage assertions, including this required owner-notify marker (`build/phase5-owner-notify-final.log`). Inventory validation and both 80-file public-header checks pass in the same run.

Logs: `build/phase5-owner-notify-baseline.log`, `build/phase5-owner-notify-mutation.log`, and `build/phase5-owner-notify-restored.log`.
