# Phase 7 row 000585: `NxPlaneShapeDesc::isValid`

The oracle row at RVA `0x00010ab0` is the virtual `NxPlaneShapeDesc::isValid()` implementation. Its finite checks cover the plane normal and distance followed by the base shape descriptor's local pose; it then enforces the shared type, flags, group, and material-index constraints. The four-byte gap before the plane fields is the `next` pointer enabled by `NX_SHAPE_DESC_LIST`. The existing public implementation in `Physics/include/NxPlaneShapeDesc.h` already matches this contract and was left unchanged.

`NxPhysicsPlaneShapeDescValidationTests` calls oracle RVA `0x10ab0` and the candidate DLL method resolved from `NxPhysics.map` on the same ten descriptors: valid defaults, non-finite normal and distance components, non-finite pose, and invalid group, flags, type, and material index. The exact differential reports `cases=10 mismatches=0`. The fixture then patches only the mapped candidate method to return false; the valid descriptor detects the mutation (`caught=1`). The candidate DLL is restored in memory before exit.

The target is registered as a Phase 7 oracle differential with one required coverage marker. Phase 7's floor rises from 2,867 to 2,868. No public Physics header or production source changed.
