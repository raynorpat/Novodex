# Actor deleting-destructor adjustor thunk mutation — `phys_fn_000116`

Date: 2026-10-08

`phys_fn_000116` is the member-table this-adjustor thunk. It subtracts eight bytes to recover the actor base and forwards the scalar deleting flag to `phys_fn_000118`. The registered `NxPhysicsObjectLayoutTests` actorctor fixture calls the thunk through a member pointer and checks the adjusted wall/member tables and allocator free count.

Suppressing the flags argument leaves the adjusted vtable transitions intact but prevents the flag-1 free (`adjFreed=0`), producing one aggregate layout mismatch. Restoring flags returns actorctor digest `19f4915a` and zero layout mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,565/2,565 registered coverage assertions (`build/phase5-actor-dtor-thunk-final.log`). No public Physics headers changed.

Logs: `build/phase5-actor-dtor-thunk-mutation.log` and `build/phase5-actor-dtor-thunk-restored.log`.
