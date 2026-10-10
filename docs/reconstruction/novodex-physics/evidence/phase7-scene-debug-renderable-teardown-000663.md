# Scene debug-renderable teardown (`phys_fn_000663`)

With visualization scale set to 1.0, `NxScene::visualize` lazily creates the
Foundation-owned renderable at Scene `+0x6b8`. The fixture resets the global
scale to zero while retaining the renderable, records its 52-byte allocation,
then releases the Scene and verifies the allocator no longer tracks it. Oracle
and candidate both report `bytes=52 freed=1`; the focused staged-pair
differential is exact.

The `releaseDebugRenderable` call was replaced with a no-op and the candidate
DLL rebuilt. The fixture reports `debug_renderable bytes=52 freed=0`; the
oracle exits 0 and the candidate exits 1 (`stdout_delta=22`). Restoring and
rebuilding returns the differential to exact output. The Phase 3 gate passes
560/560, the Phase 7 gate passes 1,649/1,649, and all 810 tooling tests pass.

This closes only the debug-renderable release. `phys_fn_000663` remains
intermediate and the full DLL reconstruction remains open.
