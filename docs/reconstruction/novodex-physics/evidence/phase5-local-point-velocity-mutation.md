# Phase 5 local-point velocity mutation evidence

`phys_fn_000148` is `NpActorVtable::getLocalPointVelocityVal` at RVA
`0x00005b40`. The registered `NxPhysicsActorLifecycleTests` differential
compares static, identity, half-turn, and quarter-turn actor paths and a
16-case quaternion/frame/point/velocity grid against the pinned oracle. The
baseline is exact (`stdout_delta=0`, `stderr_exact=True`).

In the candidate's Win32 x87 implementation, changing the final local-Z
accumulation from `fadd dword ptr [ecx+8]` to `fsub dword ptr [ecx+8]` is
caught with `stdout_delta=38`. Both mutated processes exit zero and stderr is
exact, so detection comes from the transcript comparison. The dynamic,
rotated, and quarter-turn local-point outputs change. The source file is
restored byte-for-byte, the DLL and test target are rebuilt, and the clean
differential returns to `stdout_delta=0` with exact stderr.

Build and run logs are in the ignored worktree `build/` directory:
`phase5-local-point-velocity-mutant.log` and
`phase5-local-point-velocity-restored.log`.
