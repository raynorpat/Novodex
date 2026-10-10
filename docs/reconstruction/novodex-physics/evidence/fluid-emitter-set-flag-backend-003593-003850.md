# `NxFluidEmitter::setFlag` backend dispatch (`phys_fn_003593`, `phys_fn_003850`)

The pinned oracle is `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Public Physics headers were not changed.

IDA analysis establishes the wrapper and internal helper contract. `phys_fn_003850` takes the emitter write try-lock through `this+0x0c`, calls `phys_fn_003593` with the internal object at `this+0x14`, then unlocks. If the lock is unavailable, the wrapper reports the invalid-operation diagnostic at source line `0xdd` and returns without changing state. The helper updates the 32-bit flag word at `internal+0x10` before dispatch. Exact masks 4, 8, and 16 call `EmitterSetBodyRepulsionFlag`, `EmitterSetAddBodyVelocityFlag`, and `EmitterSetEnabledFlag`, respectively. Each callback receives `*(*(fluid+0x7c)+0x30)`, `*(fluid+0x80)`, `*(internal+8)`, and the enabled byte, with `fluid=*(internal+4)`. Other masks only change the local flag word.

`NpFluidEmitter::setFlag` now preserves that wrapper flow and delegates the flag transition to a private helper. The helper looks up the three exports from the already-loaded `FluidModel.DLL`, matching the engine's FluidManager load order. The test stages the built candidate DLL as `NxPhysicsCandidate.dll` so Windows can load it beside the pinned oracle, then resolves `NpFluidEmitter::setFlag` from the candidate's own `NxPhysics.map`. It supplies a fixture `FluidModel.DLL` whose exports record callback identity and arguments. The oracle's callback data slots are temporarily redirected to equivalent in-process recorders.

The direct candidate-DLL probe compares all three backend masks for both enabled and disabled transitions, including callback identity, all arguments, and flag-word preservation. These calls exercise the successful write-lock path. It also verifies the visualization bit can be set and cleared and that `NX_FEF_BROKEN_ACTOR_REF` updates local state without making a backend call. The lock-contention diagnostic branch is identified from the oracle disassembly but is not separately triggered by this fixture. The restored run reports:

```text
fluid emitter flags cases=2 mismatches=0
fluid emitter backend flags cases=7 callback_mismatches=0 state_mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

Two row-specific mutations establish detection. First, changing only the mask-4 callback from `EmitterSetBodyRepulsionFlag` to `EmitterSetAddBodyVelocityFlag` is caught with `callback_mismatches=2`; the restored run has zero mismatches. Second, replacing the wrapper's internal-helper call with a no-op while retaining the write-try and unlock is caught with `flags mismatches=1`, `backend callback_mismatches=6`, `state_mismatches=4`, and `aggregate mismatches=11`; the restored run again has zero mismatches. Both mutations rebuilt `NxPhysics.dll` and the harness through CMake Win32 Release and were executed against the pinned oracle.

The test line `fluid emitter backend flags cases=7 callback_mismatches=0 state_mismatches=0` is required by both Phase 5 and Phase 7 gates. The registered coverage floors are 2,617 and 1,445, respectively. This closes only the internal flag helper and public setter wrapper rows; FluidModel creation, other fluid-emitter operations, and the remaining full-DLL reconstruction are still open.
