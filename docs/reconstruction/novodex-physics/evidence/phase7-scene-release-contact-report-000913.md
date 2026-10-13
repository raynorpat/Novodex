# Scene-release contact report row `phys_fn_000913`

The existing implementation calls `cpmFireContactReports0913` during Scene destruction, after advancing the scene stamp and retiring stale pair nodes, but before actor teardown. The Phase 7 closure ledger had this row deferred because it had no row-specific mutation.

`NxPhysicsSimulationTests` now creates a separate plane/sphere contact scene with start, touch, and end notifications. It runs until the first real start-touch callback, then releases the Scene while the pair remains active. The callback observes the still-live actor wrappers and the final end-touch event:

```text
simulation scene-release-contact start=1 before=1 calls=2 events=00000004 end_touch=1 actors=1
```

Oracle and restored candidate both exit 0 with `stdout_delta=0` and exact stderr. A row-specific loop suppression mutation (`count = 0`) was rebuilt into `NxPhysics.dll`; its SHA-256 was `47f3be8488bde4ed038717e7906c390af6575aeac74146c3118fccee873a2483`. The oracle still reports the final end-touch callback, while the mutant candidate reports only the initial start-touch callback and exits 1. The staged differential rejects it (`stdout_delta=439`, `stderr_exact=False`). Restored candidate DLL SHA-256: `f614fa9f32321fe8ba289b48a30a74ebe903cc7dee4626910ce136bee5dbb666`.

The mutation source patch and compressed build/run transcripts are in [raw evidence](raw/phase7-scene-release-contact-report-000913-20261013/).

The Phase 7 registry now requires the end-touch marker from `NxPhysicsSimulationTests`; its coverage floor increases by one. This closes `phys_fn_000913` only. Rows `phys_fn_000917`, `000919`, and `000921` remain deferred for their own row-specific mutations. Public Physics headers are unchanged.
