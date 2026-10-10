# Scene destructor destroys retained body records (`phys_fn_000602`)

IDA's pinned-oracle function at RVA `0x00011060` walks the body-record pointer
range `Scene+[0x56c, 0x570)`. For each non-null entry it calls
`DynamicBody::destruct` (`phys_fn_000776`) and frees the record with the
Foundation allocator. The Scene destructor calls this pass after the two
retained-joint lists (`phys_fn_000606`).

`nxSceneDestroyBodyRecords` now reproduces that order. The registered
`NxPhysicsPopulatedSceneTeardownTests` fixture creates one dynamic actor, saves
its 608-byte record, clears the record link from the actor's pose, then releases
the Scene. The ordinary actor path removes the actor and root, while the record
remains in the Scene's body-record range for `phys_fn_000602` to destroy.

The pinned oracle reports `teardown retained_body_record bytes=608 freed=1` and
exits 0. Before the helper was added, the candidate reported `freed=0` and
exited 1. A no-op mutation at the start of `nxSceneDestroyBodyRecords` was also
caught (`freed=0`, candidate exit 1, `stdout_delta=2`). Restoring the helper
returns to exact output (`stdout_delta=0`, `stderr_exact=True`, both exits 0).
The row is registered with the Phase 3 and Phase 7 differentials; their
coverage floors increase by one each.
