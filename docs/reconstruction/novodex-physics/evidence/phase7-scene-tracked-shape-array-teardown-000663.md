# Scene tracked-shape array teardown (`phys_fn_000663`)

Creating one static box actor and one dynamic box actor grows the retained
tracked-shape array at Scene `+0x6a4` through the SDK allocator. The fixture
captures its 8-byte allocation after actor creation, releases the Scene, and
checks the allocator for that same pointer. The oracle and candidate both
report `bytes=8 freed=1`; the focused staged-pair differential is exact.

The candidate's `nxGetSdkAllocator()->free(entries)` in `nxSceneDelete` was
replaced with a no-op statement and the DLL rebuilt. The fixture reports
`tracked_shape_array bytes=8 freed=0`; the oracle exits 0 and the candidate
exits 1 (`stdout_delta=24`). Restoring and rebuilding the candidate returns the
differential to exact output. The full Phase 3 and Phase 7 gates pass at
559/559 and 1,648/1,648, and all 810 tooling tests pass.

This closes only the tracked-shape array release. `phys_fn_000663` remains
intermediate and the full DLL reconstruction remains open.
