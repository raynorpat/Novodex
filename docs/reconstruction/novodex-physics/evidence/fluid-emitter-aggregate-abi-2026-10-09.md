# NxFluidEmitter aggregate-return ABI findings — 2026-10-09

Oracle image: `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, pinned SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

IDA references from the `NpFluidEmitter.cpp` source string identify the observed `NxFluidEmitter` vtable sequence at RVA `0x1166b8`. After the two destructor entries, its six value-returning getters occupy these rows and return types, matching the immutable public declaration order:

| Getter | Stable row | RVA | Returned object | Oracle ABI |
| --- | --- | ---: | --- | --- |
| `getGlobalPoseVal()` | `phys_fn_003804` | `0x8c590` | `NxMat34` (48 bytes) | hidden result pointer on stack; `ret 4` |
| `getGlobalPositionVal()` | `phys_fn_003806` | `0x8c5e0` | `NxVec3` (12 bytes) | hidden result pointer on stack; `ret 4` |
| `getGlobalOrientationVal()` | `phys_fn_003808` | `0x8c620` | `NxMat33` (36 bytes) | hidden result pointer on stack; `ret 4` |
| `getLocalPoseVal()` | `phys_fn_003816` | `0x8c870` | `NxMat34` (48 bytes) | hidden result pointer on stack; `ret 4` |
| `getLocalPositionVal()` | `phys_fn_003818` | `0x8c8a0` | `NxVec3` (12 bytes) | hidden result pointer on stack; `ret 4` |
| `getLocalOrientationVal()` | `phys_fn_003820` | `0x8c8d0` | `NxMat33` (36 bytes) | hidden result pointer on stack; `ret 4` |

Each method returns the output pointer in EAX. The observed implementation locks the object field at `this+0x10` and accesses the internal emitter field at `this+0x14`. The global pose getter copies 12 dwords from internal `+0x48`; global position copies three dwords from `+0x6c`; global orientation copies nine dwords from `+0x48`. The local pose, position, and orientation getters delegate to rows `phys_fn_003563`, `phys_fn_003565`, and `phys_fn_003567`, respectively, then release the lock.

Existing `lockacc`/`lockedcopy` evidence compares several row bodies using no-op Foundation lock stubs. It does not exercise these methods through the public `NxFluidEmitter` interface or compare the caller's hidden-result-pointer convention against the oracle. The candidate tree has no `Physics/src/fluids/NpFluidEmitter.cpp` implementation, and the public methods are pure virtual; therefore there is no candidate emitter vtable on which to run the end-to-end ABI probe yet. Do not mark M1 complete based on the row-level copy probes. Public headers remain unchanged.
