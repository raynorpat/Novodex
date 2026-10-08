# FluidManager actor-created notification no-op (2026-10-08)

`phys_fn_003637` (`0x00089d50`) is the manager's actor-created notification.
IDA's pinned-binary decompilation shows the disabled-backend warning at
`FluidManager.cpp:263`, followed by a count of the primary fluid pointer array
and one call to `nullsub_1` for each entry. `nullsub_1` is the three-byte
return-only function at RVA `0x000a0f60`; it does not read or modify the actor,
fluid, or manager. The only caller, `Scene::createActor` (`0x00011730`), ignores
the notification result and returns the new actor.

The source helper in `Physics/src/Scene.cpp` therefore reproduces the complete
observable behavior without retaining a timing-only loop: it emits the exact
warning when manager byte `+0x2b` is clear and otherwise has no effect. This
holds for any valid primary-array contents, including nonempty arrays, because
the oracle's per-entry call is a no-op.

The registered `NxPhysicsSimulationTests` fixture creates a public sphere actor
after installing the disabled manager. Its paired differential matches exactly
(`stdout_delta=0`, `stderr_exact=True`;
`build/FluidGate/fluid-actor-notify-green.log`). That fixture exercises the
warning with the pinned manager's empty array; the nonempty-array conclusion
comes from the pinned disassembly and the caller's ignored return value. No
public Physics header changed.
