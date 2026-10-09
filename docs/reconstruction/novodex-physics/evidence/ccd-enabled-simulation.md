# Continuous-collision scene differential

Phase 6 now includes `NxPhysicsCcdSimulationTests`, a public-API scene test for
enabled continuous collision detection. It sends a dynamic box through a thin
static box at high speed, then advances one more step to observe the delayed
sweep response. The target is registered in `tools/gate_targets.ps1`, and both
trajectory lines are required gate coverage.

The test compares the pinned oracle (`NxPhysics.dll` SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`) with the
candidate (`NxPhysics.dll` SHA-256
`d0b99bb835e6ee18456a7d420571ce596fe5be272179008fd153043fb9706b0b`). Both
produce:

```text
ccd step=0 x=40e00000 vx=100.000 ready=1 fetched=1
ccd step=1 x=be9eb858 vx=0.400 ready=1 fetched=1
```

The position bits match on both steps, and the response velocity passes the
test's `[0.399, 0.401]` acceptance range. The underlying response velocity
still differs by two ULPs (oracle `0x3ecccccb`, candidate `0x3ecccccd`); the
printed line is rounded to three decimals, so this scene confirms the collision
response and trajectory without claiming bit-exact solver arithmetic.

The full Phase 6 paired differential gate passed on 2026-10-08. All nine
registered targets exited successfully with identical captured stdout and
stderr, including this CCD scene. The candidate implements the recovered CCD
pair path and expands dynamic broadphase bounds for the swept motion. Public
NxPhysics headers were not changed.
