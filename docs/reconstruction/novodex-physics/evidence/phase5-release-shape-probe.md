# Phase 5 releaseShape row probe

`phys_fn_000072` (`NpActorVtable::releaseShape`, RVA `0x2b40`) is exercised by
the existing shape-mutation target, but that target's release-down loops make
a global no-op mutation hang. Added the bounded
`NxPhysicsActorReleaseShapeProbeTests` target: it creates a static actor with
two box shapes, releases one handle, checks the remaining public handle, and
tears the actor down without looping on the shape count.

The clean Release staged-pair differential reports `stdout_delta=0` and exact
stderr. Replacing the row's `nxActorReleaseShape` call with `(void)shape` in a
temporary candidate build is caught by the bounded target (`oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=2`, exact stderr). The source was restored,
rebuilt, and the clean differential again reports `stdout_delta=0` and exact
stderr. The new Phase 5 coverage line is
`release_shape count=1 remaining_first=1 remaining_released=0`.

This closes the wrapper's tested public release dispatch. The existing broader
shape-mutation target remains responsible for group promotion, swap-removal,
last-shape teardown, and the documented Actor.cpp error paths.
