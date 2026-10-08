# Actor scalar deleting destructor mutation — `phys_fn_000118`

Date: 2026-10-08

`phys_fn_000118` writes the actor table, restores the member subobject table, resets the member table, installs the interface-wall table, and conditionally releases the actor through the SDK allocator. The registered `NxPhysicsObjectLayoutTests` actorctor fixture checks both direct and +8-adjustor deleting-destructor calls, their table transitions, and allocator deltas.

Changing the final wall-vtable store from `0x101043d0` to `0x101043d4` breaks both destructor table observations and is detected as one aggregate layout mismatch. Restoring the correct table returns actorctor digest `19f4915a` with zero layout mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,565/2,565 registered coverage assertions (`build/phase5-actor-dtor-final-retry.log`). The first full attempt encountered a nonreproducible oracle access violation in `NxPhysicsSceneRaycastTests`; the direct oracle/candidate target rerun and the full gate retry both passed. No public Physics headers changed.

Logs: `build/phase5-actor-dtor-mutation.log` and `build/phase5-actor-dtor-restored.log`.
