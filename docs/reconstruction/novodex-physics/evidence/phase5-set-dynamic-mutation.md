# Phase 5 `setDynamic` mutation

`phys_fn_000122` (`NpActorVtable::setDynamic`, RVA `0x3840`) is driven by
`NxPhysicsActorShapeMutationTests` through a static actor with one shape,
multi-shape groups, density-derived mass, a pre-existing dynamic body, and a
shape-less actor. The harness reads the actor record and static/dynamic
pruner counts and then applies force and velocity operations to converted
actors.

As a row-targeted mutation, static-pruner re-registration was disabled after
the static-to-dynamic record build (`if(false && readd)`). The staged-pair
differential caught the missing dynamic pruner entry with
`stdout_delta=36`; both processes exited zero and stderr matched exactly
(`build/set-dynamic-readd-red.log`). Restoring the registration and rebuilding
returned the target to `stdout_delta=0` with exact stderr
(`build/set-dynamic-readd-green.log`).
