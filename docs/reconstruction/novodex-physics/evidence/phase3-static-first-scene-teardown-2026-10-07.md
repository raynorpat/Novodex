# Static-first populated Scene teardown

`NxPhysicsPopulatedSceneTeardownTests` adds a small Phase 3 lifecycle fixture
for a static box actor created before a dynamic box actor, followed by
`NxPhysicsSDK::releaseScene`. A counting allocator records the live allocation
count immediately before and after Scene release. This isolates the actor-walk
case where a static actor precedes a body-bearing actor.

The oracle and candidate both report:

```text
teardown static_first outstanding_before=51 outstanding_after=12 delta=-39
```

The focused staged-pair run exits 0 on both sides with `stdout_delta=0` and
`stderr_exact=True`. The twelve remaining allocations belong to the still-live
SDK/Foundation pair; the assertion measures the Scene release delta, not total
process cleanup.

A sensitivity check temporarily skipped the Scene's live-actor release call.
The candidate then reported `delta=-30` while the oracle remained at `delta=-39`,
and the target failed. The production implementation was restored and rebuilt
before the green staged-pair run.

This fixture supplements the broader Phase 5 populated-Scene transcript, which
checks exact ordered teardown frees and SDK cleanup. It does not close the
remaining Scene simulation, query, error-path, or full-DLL reconstruction work.
Public Physics headers are unchanged.
