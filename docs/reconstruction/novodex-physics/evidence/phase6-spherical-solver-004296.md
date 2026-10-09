# Spherical solver public pendulum and mutation proof

The registered simulation differential drives a public spherical joint for
eight fixed steps. A dynamic sphere starts at `(13, 4, 0)`, its world anchor is
`(12.75, 4, 0)`, and gravity acts along negative Y. This nonzero lever arm
produces a pendulum response; exact actor position, linear velocity,
orientation, and angular velocity words are required coverage at every step.
The clean oracle/candidate transcript matches exactly.

The row-specific mutation was tested in a detached worktree at `32578585`.
The Y component of the spherical solver bias in
`SphericalJoint::row_slot6` was negated. The registered
`NxPhysicsSimulationTests` staged-pair differential caught the mutation with
both processes exiting 0, `stdout_delta=24`, and exact stderr. The changed
transcript includes the pendulum actor's position and angular response. The
row was restored byte-for-byte, rebuilt, and the same differential passed with
`stdout_delta=0` and exact stderr. Captured transcripts are in
`build/phase6-spherical-004296-mutant.log` and
`build/phase6-spherical-004296-restored.log`.

The corresponding Phase 5/6/7 paired gates include the required per-step
position, velocity, orientation, and angular-velocity words. The full joint
and DLL reconstruction remain open.
