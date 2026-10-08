# Public convex cooking process variation

The public `createTriangleMesh` route uses the reconstructed hull wrapper and
the pinned qhull implementation. A 400-point cloud with five collinear cluster
centers was not a valid exact-output fixture: across 20 fresh processes the
pinned oracle produced 12 different results and rejected the cook seven times;
the candidate produced seven results. IDA maps the address-dependent facet hash
to `qh_gethash` (`NxPhysics.dll+0x742f0`), matching
`External/qhull/upstream/src/poly.c`. The changed heap layouts make raw facet
order and the reduced hull topology vary between independent processes.

The public test now uses five spatially separated, non-collinear centers. Since
400 points enter the wrapper's 256-point reduction path, the public fixture
checks successful creation, a nonempty 3D result, valid float/index formats,
in-range and referenced indices, nondegenerate triangles, output coordinates
inside the input bounds, and allocator balance. It emits a stable marker rather
than asserting process-dependent facet counts or array order. The Phase 4
`NxPhysicsThirdPartyTests` oracle differential covers separate recorded qhull
input sets and algorithm paths (`hull_compute_qhull` and related cases); the
public cloud is an independent wrapper-integration fixture.

With the revised input, 20 fresh oracle processes and 20 candidate processes
all emitted the same successful topology marker. A subsequent 12-process check
per side also produced identical mesh arrays and exact plane-settle output:
vertex FNV `b358b4da777eaee5`, triangle FNV `de3e8705a22195a4`, and y bits
`0x3f73332a` with zero velocity. The mesh-contact assertion was tightened from
one ULP to exact float bits.

The differential runner previously accepted two matching nonzero child exit
codes. A forced-failure probe returned exit 1 for both sides with zero stdout
delta; the old runner passed it. `run_differential.ps1` now requires both child
exit codes to be zero in addition to matching output and stderr. The same forced
failure now reports `both_exit_zero=False` and fails the gate.

Validation on 2026-10-08:

- Phase 4 passed with 268 recorded coverage assertions against a floor of 266;
  both 80-file public-header manifests, inventory validation, and vendored
  source verification passed.
- The direct `NxPhysicsTriangleMeshApiTests` differential passed with zero
  stdout delta and exact stderr; the focused 12-process repeat passed for both
  DLL pairs.
- Phase 5 passed with 2,563/2,563 coverage assertions after the differential
  runner began requiring successful child exits.

This resolves the deterministic cube mesh-contact mismatch and makes the test
gate reject symmetric process failures. It does not establish byte-identical
reduced hull topology across independent allocator layouts; full mesh and
reducer fidelity remains open.
