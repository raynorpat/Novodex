# Scene release deleting-destructor path

IDA maps `phys_fn_001275` at RVA `0x256e0` to an 11-byte deleting-destructor
adapter: it returns for a null object, loads the object's vtable, passes flag
`1`, calls slot 0, and returns. `NxSceneInternal::vtable()` installs
`nxSceneDelete` at that slot. `PhysicsSDK::releaseScene` removes the Scene from
its unsorted array and invokes `NxSceneInternal::scalarDeletingDestructor(1)`,
which delegates to the same teardown routine.

The SDK fixture now reports allocator counts immediately after both explicit
scene releases. Oracle and restored candidate agree at
`step=scenes_released.allocator ... free_calls=18`. A temporary no-op mutation
of `NxSceneInternal::scalarDeletingDestructor` produced `stdout_delta=4` with
oracle and candidate exit 0; the candidate reported `free_calls=0`. A temporary
no-op mutation of `PhysicsSDK::releaseScene` produced `stdout_delta=8`, left
both scenes registered (`count=2`, `survivor_lookup=0`), and made the candidate
exit 1. Earlier independent mutations of the public `NpPhysicsSDK::releaseScene`
wrapper and `createScene` wrapper were also detected by `NxPhysicsSDKTests`.

The required-coverage registry now requires the allocator checkpoint in both
Phase 2 and Phase 3. Phase 2 passes with four SDK coverage assertions; Phase 3
passes with 531 total assertions, including the SDK lifecycle target. The
ledgers now close Phase 2 rows `phys_fn_000236` and `phys_fn_000468` as
discharged by Phase 3, and dynamically gate Phase 3 row `phys_fn_001275`. Phase
2 now records 60 closed and 83 deferred function rows; Phase 3 records 63
closed and 329 deferred. The broader Scene creation dependency tree remains
open. Public Physics headers are unchanged.
