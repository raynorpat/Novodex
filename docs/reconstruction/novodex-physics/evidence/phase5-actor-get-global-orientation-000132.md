# Actor global-orientation getter `phys_fn_000132`

Oracle row `phys_fn_000132` at RVA `0x000046c0` reads the actor orientation
under one read guard. Dynamic actors convert the quaternion in the nested
record using the oracle's x87 sequence and spill points; static actors copy
the cached matrix at body `+0x20`. The candidate vtable wrapper uses the same
single guard across both arms.

The baseline `NxPhysicsActorLifecycleTests` staged-pair differential passed:
both children exited 0, `stdout_delta=0`, and stderr matched exactly. A
temporary mutation added `1.0f` to the first returned matrix word. Static,
rotated, and quarter-turn public orientation cases caught it with equal zero
exits, exact stderr, and `stdout_delta=6`. Restoring the source returned the
transcript to `stdout_delta=0` with exact stderr.

The restored Release candidate DLL SHA-256 was
`0433cd9730abefcfe5875075476b790847d5b30deda0db2f049aff625397a49b`. Public
Physics headers were not changed.
