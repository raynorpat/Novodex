# Fluid manager dirty-collision refresh: `phys_fn_003630`

The `+0x28` branch of `phys_fn_003630` (`0x89bd0`) destroys the current
FluidModel static-collision object, reacquires that object, calls
`phys_fn_003505` with the scene, step value, and manager cell size at `+0x2c`,
then clears the dirty byte before walking the live-fluid range. The candidate
implements that order in `nxFluidManagerStepImpl` in `Physics/src/Scene.cpp`.

The registered `NxPhysicsFluidEmitterAbiTests` calls the pinned oracle and the
map-resolved candidate with the dirty byte set, an enabled fixture manager,
cell size `0.5`, an empty scene shape collection, and two distinct live fluids.
It checks two static collision lookups, one destroy, one builder
initialization, the collision grid scale and margin, dirty-byte clearing,
both ordered `FluidRun` payloads and state clears, return value, and x86 stack
balance. The fixture returns distinct old and replacement buffers. The test
checks the rebuilt scale and margin in the replacement and byte-compares the
old buffer against its original poison. The new coverage marker is:

```text
fluid manager dirty-refresh get=1 destroy=1 create=1 data=1 flag=1 fluids=2/2 callbacks=1 timestep=1 state=1 stack=1/1 mismatches=0
```

A wrong-target mutation that rebuilds into the old buffer is rejected by the
same test (`built=0 old-preserved=0`, `data=0`, one mismatch, exit 1). The
restored candidate reports `built=1 old-preserved=1` and zero mismatches. The
existing dirty-branch suppression mutation remains covered by the prior proof.
Fresh Release gates pass: Phase 5 evaluates 2,672/2,672 coverage assertions
and Phase 7 evaluates 1,702/1,702. The oracle-only transcript digest is
unchanged; only the pinned `PhysicsFluidEmitterAbiTests.cpp` fixture-source
hash was updated. The clean two-fluid differential remains registered as an
independent branch control. Raw Phase 5/7 logs and wrong-target mutation /
restored-control logs are retained under
`evidence/raw/phase7-fluid-manager-dirty-refresh-003630/`.
