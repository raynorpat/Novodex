# Joint structure-return ABI probe

The Phase 6 `NxPhysicsJointTests` harness calls `getGlobalAnchorVal` and
`getGlobalAxisVal` through their raw x86 vtable entries for each of the ten
joint families. Each raw `__thiscall` places `this` in ECX and the hidden
`NxVec3` result buffer on the stack, then checks callee stack cleanup and
preservation of EBX, ESI, EDI, and EBP. The raw bytes are compared with the
typed calls, and the Phase 6 gate requires a clean probe for one case from each
family.

The oracle and candidate report two cases, zero ABI flags, and zero byte
mismatches for the representative case in every family. The existing actor
ABI probe retains its deliberately incorrect-cleanup control; the joint probe
uses the same stack and register checks for both return slots.

The joint harness also emits its existing joint-allocation transcript. Any
allocator differences remain visible in the Phase 6 differential rather than
being folded into the actor lifecycle gate.

Public headers remain unchanged.
