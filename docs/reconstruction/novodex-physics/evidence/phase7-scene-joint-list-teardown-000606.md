# Phase 7 Scene retained-joint teardown: `phys_fn_000606`

`phys_fn_000606` is the Scene destructor's two-list joint cleanup at RVA
`0x000110f0`. After actor, effector, cached-controller, and body-record
cleanup, it walks the joint heads at Scene offsets `+0x59c` and `+0x5a0`,
clears each joint's scene/link state, dispatches its deleting destructor with
flag 1, and continues from the saved next link.

The registered `NxPhysicsPopulatedSceneTeardownTests` differential now creates
a dynamic actor and a fixed joint, then releases the Scene while the joint is
still registered. On both the pinned oracle and restored candidate, the
release frees 35 tracked blocks (`release_delta=-35`); the focused staged-pair
run has zero stdout delta and exact stderr.

For mutation falsification, the candidate's joint-list loop was temporarily
disabled by making its range condition false, then `NxPhysics.dll` was rebuilt
and the registered Phase 7 staged-pair test rerun against the pinned oracle.
The mutant freed only 33 blocks (`release_delta=-33`), producing
`stdout_delta=2` with both processes exiting zero and exact stderr. Restoring
the loop and rebuilding returned the pair to `stdout_delta=0`.

This closes the retained-joint list cleanup row only. The enclosing
`phys_fn_000663` Scene destructor still has additional ownership and teardown
paths to reconstruct and falsify.
