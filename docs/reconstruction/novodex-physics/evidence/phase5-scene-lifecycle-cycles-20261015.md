# Phase 5 repeated public Scene lifecycle — 2026-10-15

`NxPhysicsSceneLifecycleTests` exercises the public SDK scene array through eight complete cycles. Each cycle creates three scenes, releases the middle scene, appends a new scene, then releases the first and last remaining elements. The fixture checks scene count, array order, replace-with-last behavior, and stable wrapper identity throughout.

The restored oracle and candidate runs agree exactly:

```text
scene lifecycle cycles=8 creates=32 releases=32 stable=1 mismatches=0
oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

A temporary candidate-only mutation made `PhysicsSDK::releaseScene` a no-op. The oracle still passed; the candidate exited 1 with `stable=0 mismatches=47` and a two-line output delta. After restoring and rebuilding the source, the exact differential above passed again. The resulting candidate `NxPhysics.dll` SHA-256 is `b04154b3ef9ed2e1b6dd187947faf36c552210aa47705fae498d78f62acb68a4`.

The Phase 5 gate passed at 2,741/2,741 coverage assertions. The full reconstruction tooling suite passed all 816 tests. This packet adds coverage and refreshes stale gate/proof bookkeeping; it does not close a production function row. Public SDK headers were not changed.

Compressed full transcripts and their SHA-256/size manifest are in `evidence/raw/phase5-scene-lifecycle-cycles-20261015/`.
