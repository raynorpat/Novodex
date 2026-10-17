# NpFluidEmitter scalar deleting destructor (`phys_fn_003794`)

IDA identifies RVA `0x8c310` as the complete scalar deleting destructor. It resets the primary and `NpFluidEmitterReadLock` vptrs, invokes the read-lock base destructor, restores the primary `NxFluidEmitter` vptr, and conditionally frees the 0x18-byte object. The CMake linker map emits the candidate primary scalar deleting destructor as `??_ENpFluidEmitter@@UAEPAXI@Z` at `0x1003ce40`.

The registered `NxPhysicsFluidEmitterAbiTests` probe reaches it through the secondary `NpFluidEmitterReadLock` vtable for deleting flags 0 and 1. The `phys_fn_003794` marker checks both base-vptr transitions, return identity, x86 stack balance, and allocator free. The restored control reports zero mismatches.

A candidate-only mutation changed the primary-base vptr immediate from `0x1012f304` to `0x1012f390`; the probe rejected it with `candidate_primary=0` and `mismatches=1`. The candidate DLL was restored byte-for-byte. Fresh Phase 5 and Phase 7 gates pass at 2,838/2,838 and 2,971/2,971 assertions; the full reconstruction-tool suite passes 817 tests. Raw evidence is under `evidence/raw/phase7-fluid-emitter-deleting-dtor-003794-20261017/`.
