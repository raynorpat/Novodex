# NpScene joint and effector lifecycle wrapper falsification

Rows `phys_fn_000299`, `phys_fn_000301`, and `phys_fn_000303` are the public
`NpScene` forwards for joint release, spring-and-damper effector creation, and
effector release. Each uses the existing Phase 7 staged-pair fixture that
exercises the operation and observes lifecycle state.

The clean joint matrix matches the pinned oracle exactly. A temporary
`releaseJoint` mutation that omits the `Scene::releaseJoint` call is rejected by
`NxPhysicsJointStagedPairTests` (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=494`, exact stderr). The clean effector fixture also matches
exactly. Omitting `Scene::createSpringAndDamperEffector` is rejected by
`NxPhysicsEffectorTests` (`stdout_delta=85`); omitting `Scene::releaseEffector`
is rejected by the same target (`stdout_delta=48`). All mutant processes exit
zero with exact stderr. After restoring the source and rebuilding, both
registered controls return to `stdout_delta=0` with exact stderr.

The source was restored byte-for-byte before final verification. The fresh
full Phase 7 gate passes at `1,704/1,704` required assertions. The focused
mutation, restored-control, and full-gate transcripts are archived under
[`evidence/raw/phase7-npscene-lifecycle-wrappers-000299-000301-000303-20261012/`](raw/phase7-npscene-lifecycle-wrappers-000299-000301-000303-20261012/).
