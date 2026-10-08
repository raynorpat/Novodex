# Actor construction tail mutation — `phys_fn_000044`

Date: 2026-10-08

`phys_fn_000044` completes the actor construction chain by installing the dynamic actor vtable after the wall table, zero owner, initialized member subobject, and body pointer have been written. The registered `NxPhysicsObjectLayoutTests` actorctor fixture checks all five observations.

Changing the final vtable store from `0x10104530` to `0x10104534` is detected: the first construction assertion fails and the aggregate layout differential reports one mismatch. Restoring `0x10104530` returns actorctor digest `19f4915a` and zero layout mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,565/2,565 registered coverage assertions (`build/phase5-actor-ctor-final.log`). No public Physics headers changed.

Logs: `build/phase5-actor-ctor-mutation.log` and `build/phase5-actor-ctor-restored.log`.
