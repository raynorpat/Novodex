# NpFluidEmitter secondary deleting thunk (`phys_fn_003862`)

IDA identifies RVA `0x8d090` as an 8-byte secondary scalar-deleting adjustor: it subtracts 8 from the `NpFluidEmitterReadLock` subobject pointer and jumps to the complete `NpFluidEmitter` scalar deleting destructor at RVA `0x8c310`. The CMake build already emits `??_ENpFluidEmitter@@W7AEPAXI@Z`; no public header changes were needed.

The registered `NxPhysicsFluidEmitterAbiTests` differential invokes the oracle and candidate secondary-vtable slot with deleting flags 0 and 1. It verifies thunk resolution, both primary and secondary vptr transitions, seeded object words, returned root pointer, x86 stack balance, and allocator frees. The restored run reports zero mismatches.

A candidate-only adjustor mutation (`sub ecx,8` to `sub ecx,4`) is rejected with four mismatches: the oracle transitions to both base vptrs while the candidate does neither. The candidate DLL was restored byte-for-byte to its pre-mutation SHA-256. Fresh Phase 5 and Phase 7 gates pass at 2,837/2,837 and 2,970/2,970 assertions. All 817 reconstruction-tool tests pass. Phase 5 and Phase 7 register this differential. Raw logs are retained under `evidence/raw/phase7-fluid-emitter-secondary-thunk-003862-20261017/`.
