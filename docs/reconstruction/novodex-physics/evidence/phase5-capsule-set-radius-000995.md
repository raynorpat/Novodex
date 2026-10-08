# Phase 5: capsule slot-14 set-radius (phys_fn_000995)

The row at RVA `0x00021be0` stores the requested radius at capsule shape offset `+0xe0`, then calls the capsule's slot-6 owner-update method with flag `1`. `NxPhysicsActorDynamicSetterTests` creates a capsule, calls the public `setRadius(0.75f)` route, and observes the resulting radius, dimensions, and AABB.

The pinned oracle and restored candidate produce the exact same transcript (`stdout_delta=0`, exact stderr, both exits 0). In an isolated worktree, a mutation that changed the stored value to `radius + 1.0f` was caught by that registered staged-pair test with `stdout_delta=4`, exact stderr, and both child processes exiting 0. Restoring the store returned the differential to exact.

No public Physics headers or production behavior changed.
