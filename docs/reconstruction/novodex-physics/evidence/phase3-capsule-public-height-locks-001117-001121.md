# Capsule public height lock wrappers (phys_fn_001117 and phys_fn_001121)

The public capsule height wrappers at oracle RVAs `0x23bf0` and `0x23c80` use separate scene write and read locks. The write wrapper acquires the link at public shape offset `+0x10`, delegates to internal row `phys_fn_000997` through internal shape `+0x18`, then releases the lock. If another thread owns the write lock, it reports `NXE_INVALID_OPERATION` from `NpCapsuleShape.cpp` line 52 and leaves the height unchanged. The getter brackets internal row `phys_fn_000999` on `+0x18` with the read link at public shape offset `+0x14`.

`NxPhysicsActorDynamicSetterTests` now exercises both public paths. Its locked-setter case compares the error code, file, line, message, public height, and stored half-height. Its getter case marks the read-lock owner as another thread, calls `getHeight`, and checks that the value is returned and the lock state is released without comparing the nondeterministic thread ID.

On 2026-10-15 the pinned oracle and candidate produced exact output (`stdout_delta=0`, both exits 0, exact stderr). Suppressing only the setter error report removed the expected `NpCapsuleShape.cpp` report and was rejected with `stdout_delta=623`. Removing only the getter's read-lock bracket left the simulated lock state held and was rejected with `stdout_delta=2`; restoring the bracket returned to exact output. The candidate linker map resolves the public callbacks to `nxCapsuleHandleSetHeight` at RVA `0x35f00` and `nxCapsuleHandleGetHeight` at RVA `0x35e60`.

No public Physics headers changed.
