# Capsule public dimensions write lock (phys_fn_001113)

The public `NxCapsuleShape::setDimensions` wrapper at oracle RVA `0x23b30` acquires the write link at `self+0x10`. On failure it reports `NXE_INVALID_OPERATION` from `NpCapsuleShape.cpp` line 34 and leaves the capsule unchanged. On success it calls internal row `phys_fn_000993` on the shape at `self+0x18` and releases the lock.

`NxPhysicsActorDynamicSetterTests` now calls the public method with a simulated foreign lock owner and checks the line-34 report plus the unchanged radius, height, and internal fields. Suppressing only the wrapper's error-reporting try-lock is rejected by the staged DLL differential (`stdout_delta=617`). The restored candidate matches the oracle exactly (`stdout_delta=0`, both exits 0, exact stderr). The candidate linker map resolves `nxCapsuleHandleSetDimensions` at RVA `0x35f40`.

No public Physics headers changed.
