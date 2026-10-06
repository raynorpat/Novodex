# Grounded controller sweep from floor contact

The isolated controller scene now covers a horizontal sweep that starts with
the controller resting on the floor and moves slightly downward into a low
static wall. The floor top and the controller's expanded lower face coincide
at the initial position, so the floor query has a vertical contact at sweep
time zero while the wall query has a later side contact.

Before the fix, the swept-AABB code updated its hit axis only when an entering
time was strictly greater than the initial `enter = 0`. It therefore kept the
default X axis for the floor's zero-time vertical contact, stopped at the
starting position, and reported only side flag `0x4`. The oracle's separate
vertical and horizontal probes advance to `(0.5, 0.5, 0)` and report flags
`0x5`. The candidate now retains the axis for a zero-time boundary contact
when displacement points into the face; it matches the oracle exactly.

The regression first ran RED (`stdout_delta=2`) and then GREEN with an exact
simulation transcript (`stdout_delta=0`, `stderr_exact=True`;
`build/controller-grounded-sweep-green.log`). The complete Phase 7 gate passes
all 11 registered differentials at 1,358/1,358 assertions
(`build/controller-grounded-sweep-phase7.log`), and Phase 5 passes its object
and vtable gates at 2,042/2,042 (`build/controller-grounded-sweep-phase5.log`).
The existing initial-overlap escape/inward cases also remain exact. This closes
the tested floor-contact normal case only; generalized controller penetration
recovery, successful step-over motion, and broader multi-contact behavior
remain open. Public Physics headers are unchanged.

The full Release Viewer selection passes all 48 registered cases, covering all
39 checked-in scenes and both focused physics tests; 43 pass and the same five
signature-verified oracle asset cases skip. `ViewerPhysicsStep` and
`ViewerPhysicsContact` pass.
