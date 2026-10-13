# Phase 7 fluid descriptor-emitter creation (`phys_fn_003649`)

This differential covers creation of one valid emitter from `NxFluidDesc.emitters` in the enabled FluidModel path. It advances but does not close `phys_fn_003649` or the fluid inventory.

## Recovered path

IDA confirms the fluid constructor loops over each 112-byte `NxFluidEmitterDesc` and calls the emitter constructor. That path queries `FluidGetRestParticleDistance`, allocates a 0x78-byte internal emitter, creates the backend emitter, publishes the local pose with `EmitterSetGlobalPose`, refreshes its cached global pose with `EmitterGetGlobalPose`, and appends both the internal object and its public `NpFluidEmitter` wrapper to separate arrays on the internal fluid.

The object probe established the manager layout used by the public API: the primary array stores internal-fluid pointers and the secondary array stores public `NxFluid*` wrappers. The returned wrapper is the 0x18-byte object at internal-fluid `+0x54`; its owner at `+0x14` points back to the internal fluid. `getNbFluids` counts the primary array, while `getFluids` returns the secondary array. The internal emitter arrays begin at fluid `+0x04` and `+0x14`; the latter header must remain clear before its first append.

## Differential and mutation evidence

The registered `NxPhysicsSimulationTests` fixture creates a fluid with one non-default emitter descriptor. It checks that the public list contains the returned wrapper, both manager arrays point to the expected internal and public objects, and the callback summary records one rest-distance query, one emitter create, one set-pose call, and one get-pose call. The test observes no `EmitterDestroy` callback during fluid release, matching the oracle path.

Before the production emitter path was added, the valid-descriptor differential reached both processes successfully but the oracle recorded `emitter=1.0.1.1` while the candidate recorded `emitter=0.0.0.0` (`oracle_exit=0`, `candidate_exit=0`, `stderr_exact=True`). After reconstruction, the staged differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. The matching test line is:

```text
simulation fluid enabled valid=1 invalid=1.1.140 manager-flags=1 listed=1 count=1 arrays-match=1 manager-cleared=1 callbacks=1.1.1.1 refresh=1.1.0.1 distance=1 emitter=1.0.1.1 dirty-cleared=1 cell-size=1 errors=0
```

Build: `cmake --build build/fluid-create-red --target NxPhysics NxPhysicsSimulationTests --config Release`.
Differential transcript: `build/fluid-create-red/fluid-emitter-create-verified.log`.

## Remaining work

This case does not cover multiple emitters, backend allocation or callback failures, actor-relative frames, public `NxFluid` virtual operations, public emitter getters/setters and explicit emitter release, or fluid teardown combinations. Static collision callbacks and the broader manager path also remain incomplete. The inventory row stays open and the full-DLL goal remains active. No public header was changed.
