# Public broad-phase selector mapping — `phys_fn_000544`

IDA's listing and decompilation for `NxPhysics.dll+0x00010750` show that the
three public broad-phase selectors select pruning-engine modes 1, 2, and 3.
Other selector values report the `Scene::createBroadPhase` invalid-type error
at source line `0x63e`. The candidate applies the selector and optional scene
bounds in `nxSceneApplyDescriptorFlags`, reached from
`NxSceneInternal::initialise`.

The registered `NxPhysicsSimulationTests` differential checks the three
selector-to-mode results, and its public scene fixtures also compare bounded
and unbounded scene behavior. The clean candidate pair matches the pinned
oracle exactly:

```text
simulation broadphase selector=0 mode=1
simulation broadphase selector=1 mode=2
simulation broadphase selector=2 mode=3
differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

For mutation falsification, a throwaway `git archive` of committed HEAD
`34ecced5` changed the coherent selector's engine mode from 3 to 2. The same
registered Phase 7 differential caught the wrong mode:

```text
oracle:    simulation broadphase selector=2 mode=3
candidate: simulation broadphase selector=2 mode=2
differential target=NxPhysicsSimulationTests oracle_exit=0 candidate_exit=0 stdout_delta=2 stderr_exact=True
```

The invalid-enum diagnostic is not exercised by the current public selector
matrix. The separate ground-plane construction rows remain open. Public
Physics headers were not changed.
