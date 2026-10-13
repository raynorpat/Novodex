# Phase 7 fluid descriptor-emitter creation (`phys_fn_003649`)

This differential covers valid emitters from `NxFluidDesc.emitters` in the enabled FluidModel path, including a backend factory that returns a null handle. It advances but does not close `phys_fn_003649` or the fluid inventory.

## Recovered path

IDA confirms the fluid constructor loops over each 112-byte `NxFluidEmitterDesc` and calls the emitter constructor. That path queries `FluidGetRestParticleDistance`, allocates a 0x78-byte internal emitter, creates the backend emitter, publishes the local pose with `EmitterSetGlobalPose`, refreshes its cached global pose with `EmitterGetGlobalPose`, and appends both the internal object and its public `NpFluidEmitter` wrapper to separate arrays on the internal fluid.

The object probe established the manager layout used by the public API: the primary array stores internal-fluid pointers and the secondary array stores public `NxFluid*` wrappers. The returned wrapper is the 0x18-byte object at internal-fluid `+0x54`; its owner at `+0x14` points back to the internal fluid. `getNbFluids` counts the primary array, while `getFluids` returns the secondary array. The internal emitter arrays begin at fluid `+0x04` and `+0x14`; the latter header must remain clear before its first append.

## Differential and mutation evidence

The registered `NxPhysicsSimulationTests` fixture creates a fluid with three non-default emitter descriptors. It checks that the public list contains the returned wrapper, both manager arrays point to the expected internal and public objects, and the callback summary records all three emitter callbacks. The second `EmitterCreate` returns null; the oracle still retains its internal emitter and public wrapper, and runs the set/get-pose callbacks with that null handle. The third append crosses the initial capacity of both emitter arrays and verifies the growth path. The test observes no `EmitterDestroy` callback during fluid release, matching the oracle path.

Before the production emitter path was added, the valid-descriptor differential reached both processes successfully but the oracle recorded `emitter=1.0.1.1` while the candidate recorded `emitter=0.0.0.0` (`oracle_exit=0`, `candidate_exit=0`, `stderr_exact=True`). After reconstruction, the staged differential passed exactly. The null-backend case then exposed a second break: the oracle retained the null-backed emitter, while the candidate discarded it (`stdout_delta=52`, candidate exit 1). Removing the candidate-only null-handle failure branch restores the oracle behavior. A third emitter now forces both arrays to grow past two entries. A growth-disabled mutation leaves the oracle at three entries and the candidate at two (`stdout_delta=52`, candidate exit 1). The restored staged differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. The matching test line is:

```text
simulation fluid enabled valid=1 invalid=1.1.140 manager-flags=1 listed=1 count=1 arrays-match=1 manager-cleared=1 callbacks=1.1.1.1 refresh=1.1.0.1 distance=3 emitter=3.0.3.3 emitter-arrays=3.3 growth=1 null-backend-retained=1 dirty-cleared=1 cell-size=1 errors=0
```

Build: `cmake --build build/fluid-create-red --target NxPhysics NxPhysicsSimulationTests --config Release`.
Null-backend red/green transcript: [differential.txt](raw/phase7-fluid-emitter-null-backend-003649/differential.txt). Array-growth mutation and restored control: [differential.txt](raw/phase7-fluid-emitter-array-growth-003649/differential.txt). Earlier single-emitter transcript: `build/fluid-create-red/fluid-emitter-create-verified.log`.

Fresh registered gates after this extension pass: Phase 5 at `2,678/2,678`
assertions and Phase 7 at `1,711/1,711` assertions. The Phase 5 gate requires
the three-entry, growth, and null-handle output line; no public header changed.

## Remaining work

The fixture now covers the first internal-emitter allocation failing before its distance callback, followed by three successful emitter constructions; it also binds the final emitter to a dynamic frame actor with a 180-degree rotation and translation. The test checks the stored actor-body pointer and resulting global pose against the pinned oracle. The marker `frame-actor=1 frame-pose=1` is required in Phase 5 and Phase 7. Larger lists, public `NxFluid` virtual operations, public emitter setters and explicit emitter release, and fluid teardown combinations remain open. Static-collision callbacks and the broader manager path also remain incomplete. The inventory row stays open and the full-DLL goal remains active. No public header was changed.
