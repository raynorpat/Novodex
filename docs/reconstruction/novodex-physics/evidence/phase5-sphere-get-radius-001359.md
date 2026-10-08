# Phase 5: sphere slot-15 radius getter (phys_fn_001359)

The row at RVA `0x00027920` is the sphere internal vtable slot-15 getter. It returns the float at shape offset `+0xe0`. The existing public `NxSphereShape::getRadius()` wrapper reads the public shape state directly, so it did not exercise this internal row.

`NxPhysicsActorDynamicSetterTests` now calls the internal sphere table's slot 15 directly for a sphere whose descriptor radius is `1.0f` and emits `setter sphere_internal_get_radius=3f800000`. The registered Phase 5 oracle transcript pins that result.

A mutation in an isolated worktree changed the slot-15 getter to return `[this+0xe0] + 1.0f`. The staged-pair differential reported `stdout_delta=2`, with both child processes exiting 0 and exact stderr. After restoring the getter, the same target reported `stdout_delta=0`, exact stderr, and equal zero exits.

The earlier public-getter-only mutation probe produced no delta, which confirmed that public API observation alone was not evidence for this internal slot. No public Physics headers or production behavior changed.
