# Two-body spring force differential: `phys_fn_003979`

The public `NxPhysicsSimulationTests` fixture now connects two independent dynamic actors with one `NxSpringAndDamperEffector`. Their centres are two units apart, both anchors have the same off-centre height, and the spring is stretched from relaxed length 1 toward saturation length 4. After two fetched `1/60`-second steps, the pinned oracle and restored candidate report equal-and-opposite velocities:

```text
actor1=408e38e3.00000000.00000000
actor2=c08e38e3.00000000.00000000
```

This reaches the second dynamic-body arm of `SpringAndDamperEffector::apply`, beyond the existing one-body/world-anchor case. A candidate mutation that disables that dispatch leaves actor 2 at zero while the oracle retains `c08e38e3`; the registered staged-pair differential rejects it (`stdout_delta=2`, both exits zero, stderr exact). Restored control is exact (`stdout_delta=0`, both exits zero, stderr exact).

The row-specific marker is registered on the shared Phase 5/6/7 simulation target. Fresh gates pass at 2,808/2,808, 1,699/1,699, and 2,936/2,936 coverage assertions. Raw mutation, restored differential, and full gate logs are retained in `evidence/raw/phase6-spring-force-two-body-003979-20261016/`. The broader instruction-level and exceptional-state audits remain open.
