# Public structure-return ABI probe — 2026-10-09

`NxPhysicsActorLifecycleTests` now exercises every value-returning `NxActor`
virtual through both the compiler-generated public call and a raw x86 call
using the hidden structure-result pointer. The 25 cases cover all four result
types (`NxVec3`, `NxMat33`, `NxQuat`, and `NxMat34`): 19 `NxActor` getters (17
zero-explicit-argument and two point-velocity getters with an explicit `NxVec3`)
plus all six `NxShape` local/global pose, position, and orientation getters.
Each raw output buffer is byte-compared with the value from the typed call.

The raw adapter places `this` in ECX, pushes the hidden result buffer, then
pushes the visible point pointer when present. The pinned oracle's point-velocity
entries at slots 65 and 66 both end in `ret 8`, confirming that they pop both
stack arguments. The zero-argument methods pop the single hidden result pointer.
Every probe also checks ESP and EBX, ESI, EDI, and EBP preservation.

The slot lists are derived from the immutable `NxActor` and `NxShape`
declarations. In `NxActor`, the apparent `setStatic` virtual is inside a block
comment and is not a vtable entry; the compiler's `getCMassLocalPoseVal`
member thunk confirms slot 29 (byte offset `0x74`). In `NxShape`, the local
value getters are slots 13–15 and the global value getters are slots 22–24.

Both staged runs report:

```text
actor abi_sret actor_cases=19 shape_cases=6 flags=0 mismatches=0 wrong_cleanup_detected=1
```

The negative control omits the required callee stack cleanup and is detected.
Phase 5 registers this output as a required assertion and raises its coverage
floor by one. This closes the `NxActor` aggregate-return ABI surface; aggregate
returns on joint and fluid interfaces remain to be audited.
