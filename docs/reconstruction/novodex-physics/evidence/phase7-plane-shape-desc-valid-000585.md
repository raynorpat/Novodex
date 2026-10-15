# Phase 7 row 000585: `NxPlaneShapeDesc::isValid`

The oracle row at RVA `0x00010ab0` is the virtual `NxPlaneShapeDesc::isValid()` implementation. Its finite checks cover the plane normal and distance followed by the base shape descriptor's local pose; it then enforces the shared type, flags, group, and material-index constraints. The four-byte gap before the plane fields is the `next` pointer enabled by `NX_SHAPE_DESC_LIST`. The existing public implementation in `Physics/include/NxPlaneShapeDesc.h` already matches this contract and was left unchanged.

`NxPhysicsPlaneShapeDescValidationTests` calls oracle RVA `0x10ab0` and the candidate DLL method resolved from `NxPhysics.map` on the same ten descriptors: valid defaults, non-finite normal and distance components, non-finite pose, and invalid group, flags, type, and material index. The exact differential reports `cases=10 mismatches=0`. The fixture then patches only the mapped candidate method to return false; the valid descriptor detects the mutation (`caught=1`). The candidate DLL is restored in memory before exit.

The target is registered as a Phase 7 oracle differential with one required coverage marker. Phase 7's floor rises from 2,867 to 2,868. No public Physics header or production source changed.

The complete Phase 7 gate also passed from a clean checkout at commit `d7d0163edb2db624a962d4b6f25e8bddf10a92e8` on 2026-10-14. The clean Release candidate DLL SHA-256 was `d28cd56e5f1340e402718e73a74d1e02bceea279da7cfff666980d638c3d32eb`; the runner evaluated all 2,868 required coverage assertions and ended with `phase_gate=7 status=pass`. The concise command and result receipt is in `evidence/raw/phase7-plane-shape-desc-valid-000585-20261014/phase7-gate-receipt.txt`.
