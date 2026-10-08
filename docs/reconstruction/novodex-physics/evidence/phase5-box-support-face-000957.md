# Box hull support-face mutation proof (`phys_fn_000957`)

`phys_fn_000957` is `BoxHullFacade::supportFace`, BOX hull facade slot 9 at RVA `0x00020d40`. The registered `NxPhysicsShapeVtableTests` oracle differential constructs oracle and candidate box hulls, drives support-face and support-feature queries across axis, random-normal, crafted near-tie, pose, and face-count cases, and compares the returned face and edge result.

The clean target reports `box hull oracle_digest=e0477220 cases=324 failures=0` and exits 0. A source mutation that forces `supportFace` to return face 0 was rebuilt into that target; the fixture reported `box hull oracle_digest=e0477220 cases=324 failures=150` and exited 1. Restoring `return bestFace` and rebuilding returned to the clean 324-case result with zero failures. The required Phase 5 coverage line already pins the oracle digest and case count, so no new assertion floor was needed.

This closes mutation sensitivity for the support-face row against the registered box-hull contract. It does not claim every configuration of the hull algorithm is exhausted. Public headers and production behavior are unchanged.

The complete registered Phase 5 gate was rerun after recording this closure: all 18 staged-pair targets, the range static proof, both oracle-differential targets, and 2,311/2,311 coverage assertions passed. Inventory validation and both 80-file header checks passed.
