# Capsule public radius lock wrappers (phys_fn_001115 and phys_fn_001119)

The public capsule `setRadius` wrapper at oracle RVA `0x23b90` uses the write link at `self+0x10`. On success it dispatches internal slot 14 on the shape at `self+0x18` and releases the link. A failed try-lock reports `NXE_INVALID_OPERATION` from `NpCapsuleShape.cpp` line 43 and returns without changing the radius. The public `getRadius` wrapper at `0x23c50` acquires the read link at `self+0x14`, calls internal slot 15 on `self+0x18`, releases the link, and returns the scalar.

`NxPhysicsActorDynamicSetterTests` checks the held-write-lock report, unchanged public and internal radius, and read-lock state after `getRadius`. The pinned oracle and candidate match exactly (`stdout_delta=0`, both exits 0, exact stderr). Bypassing only the setter's reporting try-lock was rejected with `stdout_delta=629`; removing only the getter lock bracket was rejected with `stdout_delta=2`. Restoring both paths returned to exact output. The candidate linker map resolves `nxCapsuleHandleSetRadius` at RVA `0x35fc0` and `nxCapsuleHandleGetRadius` at RVA `0x35ed0`.

No public Physics headers changed.
