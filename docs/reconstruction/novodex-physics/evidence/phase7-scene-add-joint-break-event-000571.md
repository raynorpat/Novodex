# Scene add-joint-break-event mutation closure (`phys_fn_000571`)

`phys_fn_000571` is the 22-byte Scene row at RVA `0x000108e0`. It prepends a `JointBreakEvent` to the Scene list at `Scene + 0x620`: the event's `+4` link receives the previous head, then the Scene head is updated to the event. The candidate map resolves `NxSceneInternal::addJointBreakEvent` in `Scene.obj`, implemented in `Physics/src/Scene.cpp`.

The registered Phase 7 `NxPhysicsSimulationTests` baseline matched the pinned oracle. For the row-specific mutation, the insertion was changed to clear the Scene event-list head after storing the previous head in the event. The low-force broken-joint fixture then failed its callback-count assertion; the candidate exited 1 and the transcript differed by 801 bytes. The stderr difference is the expected assertion failure.

After restoring the head insertion and rebuilding, the full Phase 7 gate passed all 15 registered targets with exact staged-pair outputs, 1,639 coverage assertions against a 1,475 floor, and both immutable public-header checks. This closes the event producer independently from `processJointBreakEvents` (`phys_fn_000577`). Phase 8 full-DLL terminal closure remains open.
