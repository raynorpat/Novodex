# Interface-wall deleting destructor mutation — `phys_fn_000042`

Date: 2026-10-08

`phys_fn_000042` installs the interface-wall vtable before conditionally freeing the object through the linked CRT. The registered `NxPhysicsObjectLayoutTests` actorctor fixture calls the destructor with flag 0 on a canary-backed stack object and checks that the wall vtable is installed without disturbing the adjacent member word.

Changing the vtable store from `0x101043d0` to `0x101043d4` is detected: `actorctor candidate ok=0` with the wall-vtable observation false, and the aggregate layout differential reports one mismatch. Restoring the correct table returns the actorctor digest `19f4915a` and zero layout mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,565/2,565 registered coverage assertions (`build/phase5-wall-dtor-final.log`). No public Physics headers changed.

Logs: `build/phase5-wall-dtor-mutation.log` and `build/phase5-wall-dtor-restored.log`.
