# Disabled fluid creation path

This checkpoint reconstructs the observable `NxScene::createFluid` path for
the pinned UE3 SDK build, where the fluid backend is unavailable. It does not
claim a complete fluid implementation.

`tests/PhysicsSimulationTests.cpp` creates a scene, checks empty fluid
enumeration, calls `createFluid` with a default descriptor, checks enumeration
again, and releases the scene. The oracle and candidate both return null, create
the lazy manager at Scene `+0x61c`, keep the fluid list empty, and report exactly
one warning: code 206, line 138,
`\Epic\Novodex\SDKs\Physics\src\fluids\FluidManager.cpp`,
`NxScene::createFluid(): Feature not available!`. No public NxPhysics headers
were changed.

The candidate initializes fields that the oracle's disabled-manager
constructor leaves uninitialized. A cdb first-chance trace showed that
simulating an oracle scene after `createFluid` reaches an indirect call through
a null backend pointer in `phys_fn_003630` (`0x89bd0`). This is undefined state
in the pinned binary, not a stable result to copy. The regression therefore
releases the fluid-created scene without stepping it. Candidate fluid-scene
stepping still needs a deliberate contract after the rest of the disabled
manager path is reconstructed.

Verification recorded on 2026-10-05:

- `build/FluidGate/fluid-simulation-differential.log`: both simulation targets
  exit 0; `stdout_delta=0`, `stderr_exact=True`; the fluid line above matches.
- `build/FluidGate/phase5-fluid.log`: Phase 5 passes with 2,042/2,042
  coverage assertions.
- The isolated Phase 7 gate passes with 1,311/1,311 registered coverage
  assertions, including CMake Release build and the inventory checks.

Remaining fluid work includes `releaseFluid`, actual `NxFluid` and emitter
creation, enabled-backend behavior, manager stepping, implicit meshes, and the
fluid-manager callbacks reached by actor and shape mutation. Those rows remain
discovered; this evidence covers only the unsupported creation case and the
manager's empty-state lifecycle.
