# Capsule public `is(type)` read lock (phys_fn_001109)

The public `NxShape::is` wrapper at oracle RVA `0x23a90` acquires the read link at `self+0x14`, compares the requested type with the internal shape type at `self+0x18`, releases the link, and returns the public capsule wrapper on a match or null otherwise.

`NxPhysicsActorDynamicSetterTests` calls `is(NX_SHAPE_CAPSULE)` while the read-lock state simulates a foreign owner, then checks the matching wrapper and released lock state. Removing only the read-lock bracket is rejected (`stdout_delta=2`); the restored oracle/candidate differential is exact (`stdout_delta=0`, both exits 0, exact stderr). The candidate linker map resolves `nxCapsuleHandleIs` at RVA `0x35f60`.

No public Physics headers changed.
