# Prunable owner-query callback `phys_fn_000965`

The three-byte oracle row at RVA `0x000213e0` is a cdecl callback that ignores its owner pointer and returns zero. The candidate implements it as `shapeOwnerQuery` in `Physics/src/ObjectModel.cpp`; the linker map resolves `?shapeOwnerQuery@@YAIPAX@Z` in `ObjectModel.obj`.

`NxPhysicsSceneRaycastTests` now resolves the oracle RVA or candidate map symbol and calls the callback with both a null pointer and an opaque non-null pointer. Returning one instead of zero in the candidate is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2806`). Restoring the implementation gives an exact staged-pair differential (`stdout_delta=0`, exact stderr).

The Phase 4 gate passes with the new marker at `542/542`. The row was owned by Phase 2 but deferred as homeless shared code because its ordinary caller belongs to a later phase; this direct probe discharges that deferral through Phase 4.

Raw logs and checksums: `raw/phase4-row-000965-20261015/manifest.json`.
