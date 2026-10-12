# Fluid manager dirty-collision refresh: `phys_fn_003630`

The `+0x28` branch of `phys_fn_003630` (`0x89bd0`) destroys the current
FluidModel static-collision object, reacquires that object, calls
`phys_fn_003505` with the scene, step value, and manager cell size at `+0x2c`,
then clears the dirty byte before walking the live-fluid range. The candidate
implements that order in `nxFluidManagerStepImpl` in `Physics/src/Scene.cpp`.

The registered `NxPhysicsFluidEmitterAbiTests` calls the pinned oracle and the
map-resolved candidate with the dirty byte set, an enabled fixture manager,
cell size `0.5`, and an empty scene shape collection. It checks two static
collision lookups, one destroy, one builder initialization, the collision
grid scale and margin, dirty-byte clearing, return value, and x86 stack
balance. The marker is:

```text
fluid manager dirty-refresh get=1 destroy=1 create=1 data=1 flag=1 fluids=0 stack=1/1 mismatches=0
```

A mutation that suppresses the candidate dirty branch is rejected by the same
test (`get=0 destroy=0 create=0 data=0 flag=0`, one mismatch, exit 1). After
restoration, the full Phase 5 gate passes with 2,671 evaluated assertions
against a 2,670 floor. It includes the clean two-fluid manager loop and the
five-family collision-object destructor differentials. The clean differential
and this dirty-refresh differential are separate cases; simultaneous dirty
refresh and live-fluid iteration remains a follow-up.
