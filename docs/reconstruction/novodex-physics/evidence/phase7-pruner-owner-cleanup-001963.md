# Phase 7 pruner-owner cleanup: `phys_fn_001963`

`phys_fn_001963` (RVA `0x0004bf10`) snapshots the owner pointers in section 1
and section 2 of the static pruner and the Scene-selected dynamic pruner, then
calls each owner's scalar deleting destructor with flag 1. Its C++ reconstruction
had dereferenced the object to the vtable but attempted to call the vtable itself.
Dispatching through vtable slot 0 fixes the candidate's access violation and
matches the oracle's destructor sequence.

`NxPhysicsPopulatedSceneTeardownTests` creates and releases one static and one
dynamic actor so both pruner objects retain their pool storage, seeds one
synthetic prunable into section 1 of the static pool and one into section 2 of
the selected dynamic pool, then releases the Scene. The probe owners record the
virtual destructor and deleting flag without freeing their stack storage. Oracle
and restored candidate both report:

```text
teardown pruner_owner static_calls=1 dynamic_calls=1 flags=1/1 selected=2 outstanding_after=15
```

For mutation falsification, I omitted the vtable-slot-0 call, rebuilt `NxPhysics`,
and staged that mutant with the registered Phase 7 differential. The oracle
passed; the mutant exited 1 with both owner call counts and deleting flags at
zero. The gate rejected it with `stdout_delta=2` and `stderr_exact=False`. After
restoring the dispatch and rebuilding, the focused oracle and candidate controls
both passed with exact teardown output and identity audits.

The fault was caused by one missing pointer indirection in the candidate's
virtual call expression. The private layout and public headers are unchanged.
