# Actor kinetic-energy getter `phys_fn_000060`

Oracle row `phys_fn_000060` at RVA `0x00002900` takes the scene read lock and
returns zero for a missing dynamic record. For a dynamic actor it evaluates the
translational term first, using `(vz² + vy²) + vx²` times mass, then adds the
spin terms in `I2*w2*w2`, `I1*w1*w1`, `I0*w0*w0` order, halves the sum, and
rounds to `NxReal`. The candidate inlines this order in
`NpActorVtable::computeKineticEnergy`.

The baseline `NxPhysicsActorMomentumTests` staged-pair differential passed
with both children exiting 0, `stdout_delta=0`, and exact stderr. It covers
three energy inputs, including a midpoint case chosen to expose accumulation
order. A targeted mutation zeroed the translational term; the differential
caught it with equal zero exits, exact stderr, and `stdout_delta=14`. Restoring
the source returned the output to an exact match.

IDA confirms the oracle wrapper calls helper `phys_fn_000742` at RVA
`0x00016dd0` and rounds its return through a float local before unlocking.
The restored Release candidate DLL SHA-256 was
`6bce9022b37231b47682f8ea2eb76c1ec6fa9b9c680aaca15958916f2fb3a80e`. Public
Physics headers were not changed.
