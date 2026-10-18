# Box-shape `setName` write-lock behavior (`phys_fn_001063`)

IDA at oracle RVA `0x000233d0` shows the box-shape name wrapper reading its Scene write-lock link at handle offset `+0x10`, refusing a contended write with the `NpBoxShape.cpp:0x0f` invalid-operation diagnostic, and only changing the SDK pointer binding while holding the lock. The candidate handler `nxBoxHandleSetName` now implements that sequence in `Physics/src/NpActor.cpp`; the public headers remain unchanged.

The public differential holds the owning Scene write lock, calls `setName`, checks the original name pointer remains and one diagnostic is reported, then unlocks and verifies a later name change. The unguarded candidate mutation is detected (`stdout_delta=19`); the restored oracle/candidate output matches exactly. The rebuilt candidate symbol is `?nxBoxHandleSetName@@YIXPAX0PBD@Z` at `0x10036de0` in `NpActor.obj`.

Phase 3 now records 108 closed / 284 deferred functions. This row is discharged by the Phase 5 public setter fixture. Fresh Phase 3, 5, 6, and 7 gates pass at 984, 2,913, 1,769, and 3,052 required assertions. Raw red, green, mutation, restored, and phase-gate transcripts are compressed and hashed in [the raw evidence directory](raw/phase3-box-shape-setname-lock-001063-20261018/).
