# `NxFluidEmitter::getFlag` (`phys_fn_003852`)

The pinned oracle is `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The candidate reads the mask at `mInternal + 0x10` while holding the wrapper's read link at `+0x10`, then returns `flags & flag`.

`NxPhysicsFluidEmitterAbiTests` compares the candidate and oracle across the visualization flag's set and clear transitions while preserving unrelated sentinel bits. The restored control reports `fluid emitter flags cases=2 mismatches=0` and exits zero.

For a row-specific falsification of `phys_fn_003852`, a detached throwaway checkout at commit `c326054a` changed only the getter's mask source from `mInternal + 0x10` to `mInternal + 0x14`. A fresh CMake Win32 Release build of `NxPhysicsFluidEmitterAbiTests`, followed by execution against the pinned oracle, reported `fluid emitter flags cases=2 mismatches=1` and exited nonzero. The restored candidate again reports zero mismatches.

This closes the tested getter row `phys_fn_003852`. Setter row `phys_fn_003850` remains open: its local bitfield transition is implemented and covered, while the backend callbacks for flag masks `4`, `8`, and `16` still need reconstruction.

After recording the closure, the full Phase 7 gate passed with 1,444 coverage assertions against a floor of 1,444. Its oracle-side `NxPhysicsFluidEmitterAbiTests` run required the exact zero-mismatch flag line; inventory validation reports 39 closed and 522 deferred Phase 7 rows.
