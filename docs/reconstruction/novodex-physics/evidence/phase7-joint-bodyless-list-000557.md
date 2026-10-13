# Phase 7 joint bodyless-list insertion (`phys_fn_000557`)

`NxSceneInternal::pushJointWithoutBodies` inserts a joint whose two actors have
been detached onto the Scene's bodyless-joint list at `Scene + 0x5a0`. The
registered `NxPhysicsJointWithoutBodiesTests` fixture creates two dynamic
actors and a fixed joint, calls `NxActor::setDynamic` to exercise the detach
path, then follows the private Scene list and verifies that `NxJoint::appData`
identifies its only node and that the node's next pointer is null. This keeps
the public Physics headers unchanged.

The staged oracle and candidate both report:

```text
joint bodyless list member=1 nodes=1 next_null=1
```

The registered focused differential exits zero on both sides, with
`stdout_delta=0` and exact stderr. A deliberate no-op mutation inserted an
immediate return in `NxSceneInternal::pushJointWithoutBodies`; the oracle still
reports the node, while the mutant reports `member=0 nodes=0 next_null=1` and
exits 1 (`stdout_delta=2`). Restoring the implementation returns to an exact
differential.

The fresh full Phase 7 gate passes at 1,710/1,710 evaluated coverage
assertions. The gate log records the fixture marker for both pairs and the
target differential as `oracle_exit=0 candidate_exit=0`,
`stdout_delta=0`, `stderr_exact=True`.

The gate candidate DLL SHA-256 is
`ba3bd319f8cc388009d5bcc238518e5166a111d03ad4f922d281cc7bb781633e`; the
pinned oracle DLL SHA-256 is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
The focused restored differential candidate SHA-256 is
`a3e16f9ce1a5daa8f6b7a843de037f28b60813578a1f41c7a8f9224d822b93f0`.

Raw focused baseline, mutant, and restored logs and the Phase 7 summary are in
[`evidence/raw/phase7-joint-bodyless-list-000557/`](raw/phase7-joint-bodyless-list-000557/).
