# Capsule public material write lock (phys_fn_001111)

The public capsule material setter at oracle RVA `0x23ad0` acquires the write link at `self+0x10`, calls internal slot 13 on the shape at `self+0x18`, and releases the link. A failed try-lock reports `NXE_INVALID_OPERATION` from `NpCapsuleShape.cpp` line 16 and returns without changing the material index.

`NxPhysicsActorDynamicSetterTests` forces a foreign write-lock owner, calls `setMaterial(7)`, and compares the error report, public material, and stored material index. The pinned oracle and restored candidate match exactly (`stdout_delta=0`, both exits 0, exact stderr). Bypassing only the reporting try-lock is rejected (`stdout_delta=629`). The candidate linker map resolves `nxCapsuleHandleSetMaterial` at RVA `0x36010`.

No public Physics headers changed.
