# Scene controller-cache teardown slice (`phys_fn_000663`)

The Scene deleting destructor calls `phys_fn_002320` after `releaseEffectors`
and before cleaning its joint lists. The recovered helper walks the cache at
`Scene+0x5a8`, saves each `+0x30` link, and dispatches the node's slot-0
deleting destructor. A controller created through `NxScene::createController`
leaves a 76-byte proxy allocation on that list; destroying the Scene while the
controller remains attached must release it.

`nxSceneDelete` now calls `nxDestroyCachedList(self)` at that oracle-confirmed
position. `NxPhysicsPopulatedSceneTeardownTests` creates a Scene and controller,
confirms the generated actor remains in the Scene, then releases the Scene
without explicitly releasing the controller. Its oracle transcript is:

```text
teardown controller_owner actors_before=1 outstanding_before=49 outstanding_after=15 delta=-34
```

The registered Phase 3 staged-pair differential matches exactly (both exits 0,
`stdout_delta=0`, exact stderr). The Phase 3 gate passes 548/544 coverage
assertions, and Phase 7 passes 1454/1447.

Mutation check: temporarily omitting `nxDestroyCachedList(self)` leaves 16
candidate allocations instead of 15 and the focused staged-pair differential
rejects it (`stdout_delta=2`; both processes exit 0 and stderr is exact). The
call was restored and the focused differential returned to `stdout_delta=0`.

This verifies only the controller-cache branch of `phys_fn_000663`; the Scene
destructor row remains open for its other ownership paths.
