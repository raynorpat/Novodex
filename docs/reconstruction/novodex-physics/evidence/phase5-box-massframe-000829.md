# Box mass-frame builder falsification — 2026-10-07

`phys_fn_000829` (`MassFrame::nxMassFrameBuildBox`, RVA `0x0001bd00`) computes
the unit-density mass and diagonal inertia from three half-extents. The registered
`NxPhysicsShapeVtableTests` oracle differential drives nine box-extent inputs in
its 201-case mass-frame section. The unmodified candidate reports
`oracle_digest=7c450cef`, `cases=201`, `failures=0`
(`build/shape-vtable-000829-baseline.log`).

A throwaway git-archive build of `d2564d07` changed the xx diagonal sum from
`zz + yy` to `zz - yy` inside this function. The target reported failures for
extent cases `b=0,1,2,3,4,6,7,8`, with `failures=8` and exit 1
(`build/shape-vtable-000829-mutation.log`). The Box shape-vtable, box-sweep,
and box-hull sections remained exact, isolating the failure to the registered
mass-frame comparison. The archive mutation was never applied to the main
checkout.

This closes the row's mutation-sensitivity requirement; it does not close the
rest of Phase 5. The ledger advances to 56 closed and 149 reconstructed rows
awaiting falsification. Public Physics headers are unchanged.
