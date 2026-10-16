# Fluid emitter record scalar deleting destructor (`phys_fn_003603`)

IDA at oracle RVA `0x882b0` shows a scalar deleting destructor for the internal 0x78-byte emitter record. It preserves the internal vtable, calls the wrapper deleting destructor through `record+0x0c`, clears that wrapper slot and the SDK pointer binding, marks owner Scene dirty bit 2, and frees the record through the Foundation allocator only when flag bit 0 is set. The implementation is in `Physics/src/Scene.cpp`; wrapper name removal is handled by `nxNpFluidEmitterRemoveName` in `Physics/src/NpFluidEmitter.cpp`.

`NxPhysicsFluidEmitterAbiTests` compares the oracle RVA with the candidate implementation on non-free and free paths. The checks cover the wrapper link, emitter name and SDK binding removal, Scene dirty state, preserved vtable, return pointer, conditional free behavior, and x86 stack balance. Restored output is:

```text
fluid emitter internal dtor cases=2 wrapper_cleared=1 name_unbound=1 sdk_binding_cleared=1/1 scene_dirty=2 vtable_preserved=1 delete_free=2 stack=1/1 mismatches=0
```

Mutation evidence: omitting `nxSetSdkPointerBinding(internal, 0)` in the candidate produces `sdk_binding_cleared=1/0` and `mismatches=1`; the restored candidate reports `1/1` and zero mismatches. The captured mutant and restored logs are in `evidence/raw/phase7-fluid-emitter-record-destructor-003603-20261016/`.

The full Phase 5 and Phase 7 gates are rerun for this closure. The direct differential is registered in both phases through `NxPhysicsFluidEmitterAbiTests`.
