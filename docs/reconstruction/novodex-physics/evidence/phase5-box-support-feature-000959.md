# Box hull support-feature mutation proof (`phys_fn_000959`)

`phys_fn_000959` is `BoxHullFacade::supportFeature`, BOX hull facade slot 10 at RVA `0x00020f90`. The registered `NxPhysicsShapeVtableTests` oracle differential calls slot 10 with face-winning and edge-winning directions, optional poses and output pointers across crafted near-ties, random normals, and face-count overrides.

The clean target reports `box hull oracle_digest=e0477220 cases=324 failures=0` and exits 0. A source mutation that sets `edgeWon=1` in the no-edge arm was rebuilt into that target; the fixture reported 145 mismatches and exited 1. Restoring the `edgeWon=0` assignment and rebuilding returned to the clean 324-case result with zero failures. The existing Phase 5 coverage line pins the clean digest and case count, so no assertion-floor change was required.

This closes mutation sensitivity for the measured support-feature contract. It does not claim every numeric or degenerate hull configuration is exhausted. Public headers and production behavior are unchanged.
