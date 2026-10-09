# Prismatic solver public simulation and mutation proof

The registered simulation differential now drives a public prismatic joint for
eight fixed steps. A dynamic sphere starts at `(12, 4, 0)`, the joint axis is
world X, and gravity pulls along constrained Y. The oracle's exact per-step
position and velocity words are required coverage in `gate_targets.ps1`:

```text
step 0: position=41400000.40800000.00000000 velocity=00000000.00000000.00000000
step 1: position=41400000.407fffff.00000000 velocity=00000000.85800000.00000000
step 2-7: position=41400000.407fffff.00000000 velocity=00000000.2bfefae6.00000000
```

This exercises the constrained transverse solver response through the public
scene, joint, and actor APIs. The fixture does not change public Physics
headers.

The row-specific mutation was tested in a detached worktree at
`99eb52a4`. In `PrismaticJoint.cpp`, both linear rows in the prismatic solver
were changed from the recovered `t1` tangent to `t2`. The registered
`NxPhysicsSimulationTests` staged-pair differential caught the mutation with
both processes exiting 0, `stdout_delta=14`, and exact stderr. The changed
transcript is the prismatic actor's constrained-axis drift over steps 1-7.
The row was restored byte-for-byte, rebuilt, and the same differential passed
with `stdout_delta=0` and exact stderr.
Captured transcripts are in `build/phase6-prismatic-004386-mutant.log` and
`build/phase6-prismatic-004386-restored.log`.

The restored fixture also passes the complete registered Phase 5, Phase 6,
and Phase 7 gates: 2,578/2,569, 1,077/1,068, and 1,399/1,390 coverage
assertions respectively. The extra assertions above each floor are the nine
new required prismatic observations. Phase 6 function row `phys_fn_004386` is
now mutation-falsified; the broader Phase 6 and full-DLL reconstruction remain
open. Gate transcripts are `build/phase5-prismatic-baseline.log`,
`build/phase6-prismatic-baseline.log`, and
`build/phase7-prismatic-baseline.log`.
