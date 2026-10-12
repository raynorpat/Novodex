# Fluid emitter local-pose copy rows 003563, 003565, and 003567

IDA confirms three helper contracts used by the public local aggregate getters:

- `phys_fn_003563` copies 12 dwords from internal emitter `+0x18` to the hidden `NxMat34` result pointer.
- `phys_fn_003565` copies three dwords from internal emitter `+0x3c` to the hidden `NxVec3` result pointer.
- `phys_fn_003567` copies nine dwords from internal emitter `+0x18` to the hidden `NxMat33` result pointer.

`NxPhysicsFluidEmitterAbiTests` drives the immutable oracle's local pose, local position, and local orientation virtuals, then compares the test-side implementation and the loaded candidate DLL. The checks include aggregate return pointer and x86 stack balance. A targeted mutation in `NpFluidEmitter::copyInternal` shifted each one of the three source ranges by one dword in turn. The matching local aggregate comparison failed for each mutation on both candidate paths; the restored control reports six aggregate cases, valid return pointers, balanced stacks, and zero mismatches.

The required Phase 5/7 line is:

```text
fluid emitter local pose-copy rows=3 candidate_cases=3 candidate_retptr=3 candidate_stack=3 dll_cases=3 dll_retptr=3 dll_stack=3 mismatches=0
```

Fresh full Win32 Release gates pass at Phase 5 2,666/2,665 and Phase 7 1,693/1,692 coverage assertions. The required marker is present in both transcripts under `evidence/raw/phase7-fluid-emitter-local-copy-003563-003567/`. Inventory validation reports Phase 7 at 129 closed / 432 deferred, 6,338 functions, 5,138 data objects, and zero unexplained rows.
