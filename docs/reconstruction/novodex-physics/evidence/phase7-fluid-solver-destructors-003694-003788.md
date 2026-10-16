# Fluid solver deleting destructors — rows 003694 and 003788

IDA confirms `phys_fn_003694` at RVA `0x8b040` is the primary scalar deleting destructor for the 0x18-byte fluid solver wrapper. It installs the derived primary and secondary vfptrs, invokes the embedded read-lock base destructor at `this+8`, transitions the primary vfptr to its base table, frees the process-heap wrapper only when delete flag bit 0 is set, and returns the primary object. `phys_fn_003788` at RVA `0x8c2a0` is the secondary-vfptr thunk: `sub ecx, 8; jmp 0x8b040`.

The candidate destructor previously only freed the wrapper for the primary route and subtracted eight before freeing for the secondary route; neither route performed the base-vfptr transitions. The candidate now records the read-lock and primary base vfptrs, while the secondary entry adjusts its subobject pointer and shares the primary destructor.

`NxPhysicsFluidEmitterAbiTests` calls both oracle vtable entries and both candidate vtable entries on separate 0x18-byte scratch objects with delete flag zero. It checks both vfptr transitions, preserved non-vptr payload, and the primary return pointer. The registered oracle/candidate run reports `mismatches=0` for both rows and for the complete fluid-emitter ABI target.

Two independent mutations establish sensitivity. Omitting the primary base-vfptr store is caught by the direct primary route (`candidate_transition=0`, aggregate `mismatches=2`). Changing the secondary adjustment from `-8` to `-4` leaves the primary route exact and fails the secondary route (`payload=0`, `return=0`, aggregate `mismatches=1`). The restored candidate passes again.

Evidence is in [`evidence/raw/phase7-fluid-solver-dtor-003694-003788-20261016/`](raw/phase7-fluid-solver-dtor-003694-003788-20261016/). This proof closes only these two deleting-destructor behaviors; it does not claim that Phase 7 or the full DLL is complete.
