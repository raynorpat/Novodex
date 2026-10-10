# Scene teardown flushes the final contact report (`phys_fn_000663`)

IDA confirms that the Scene deleting destructor increments the scene stamp,
retires stale pruning pairs through `phys_fn_001953`, then conditionally calls
`phys_fn_000913` when `Scene+0x6b4` has a contact-report callback. The hash
argument is the embedded contact-pair hash at `Scene+0x2c`. This happens before
the actor-array teardown helper `phys_fn_000596`, so callback actors are still
alive.

`nxSceneDelete` now dispatches `cpmFireContactReports0913` in that exact order.
The public fixture creates a touching plane/sphere pair with start, touch and
end notifications, runs one simulation/fetch cycle, then releases the Scene
while the pair remains attached. The oracle's final callback is the end-touch
event:

```text
teardown contact_report before_release=6 after_release=7 events=0000000e
```

The restored oracle/candidate staged-pair differential is exact (both exit 0,
`stdout_delta=0`, exact stderr). A call-omission mutation is caught: the oracle
delivers callback 7 with `events=00000004`, while the candidate stays at six
callbacks and exits 1 (`stdout_delta=5`). Restoring the call returns to exact
output.

Phase 3 passes at 549/549, Phase 5 at 2621/2621, and Phase 7 at 1455/1455
registered assertions. The Phase 3 and Phase 7 registries both require the
contact-report transcript marker. The Viewer all-scenes selection passes 48/48
tests across all 39 scene entrypoints; five established pinned-oracle asset
cases are skipped by their existing signatures. `ViewerPhysicsStep` and
`ViewerPhysicsContact` both pass. This closes only the destructor's pending
contact-report dispatch branch; `phys_fn_000663` remains open for its other
ownership paths. No public Physics headers changed.
