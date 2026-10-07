# Phase 5 `updateMassFromShapes` mutation

`phys_fn_000164` (`NpActorVtable::updateMassFromShapes`, RVA `0x6520`) is
exercised by `NxPhysicsActorShapeMutationTests` across box, sphere, capsule,
multi-shape groups, planes, triggers, invalid inputs, and dynamic actors. The
test reports each record's mass/inertia, mass frame, refreshed world frame,
and update counter.

As a row-targeted mutation, the first increment of the record's `+0x198`
update counter in `updateMassFromShapes` was removed. Oracle and mutant both
exited zero with exact stderr; the registered differential reported
`stdout_delta=80` (`build/update-mass-counter-red.log`). Restoring the increment
and rebuilding produced `stdout_delta=0` with exact stderr
(`build/update-mass-counter-green.log`). The mutation touches the mass-frame
refresh sequence and does not change production behavior.
