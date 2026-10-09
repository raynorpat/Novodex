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

The candidate now has a private `NpFluidEmitter` wrapper with the measured 0x18-byte layout: `NxFluidEmitter` is the primary base, the 0x0c-byte read-lock base begins at +0x08, the lock link is at +0x10, and the internal emitter pointer is at +0x14. The six getters are concrete on this class; all unrelated emitter operations remain pure virtual. `tests/PhysicsFluidEmitterAbiTests.cpp` calls the pinned oracle through raw x86 virtual dispatch and normal public-interface calls, then compares those results with the locally linked candidate wrapper. The probe checks the hidden output pointer in EAX plus stack balance for all six aggregate returns. Its passing coverage is `fluid emitter ctor size=24 secondary_vptr_nonnull=1 internal=1 mismatches=0`, `fluid emitter raw_abi cases=6 retptr=6 stack_balanced=6 mismatches=0`, and `fluid emitter aggregate cases=6 mismatches=0`. Seven temporary mutations were each caught: the constructor's cleared secondary-base word was detected with mismatches=1, and each getter's source-offset mutation produced raw ABI mismatches=1 plus aggregate mismatches=2. Restored runs are exact. The target is registered as a Phase 5 and Phase 7 oracle differential with required coverage lines; both phase gates pass. This closes the six public aggregate-return methods and wrapper construction evidence; it does not reconstruct the rest of the fluid emitter API or backend creation path. Public headers remain unchanged.
