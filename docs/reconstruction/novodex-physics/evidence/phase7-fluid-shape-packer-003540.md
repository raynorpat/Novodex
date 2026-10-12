# Fluid collision shape packer `phys_fn_003540`

`phys_fn_003540` at RVA `0x87580` packs one collision record from a shape. The IDA listing shows a 3x4 pose reordered into the record’s column-major words, a 16-bit geometry index selecting six bounds words, an optional owner-AABB refresh when the embedded prunable update bit is clear, and a final shape handle plus flag bit. The cdecl return is the shape handle. The implementation is `nxFluidCollisionPackShapeData` in `Physics/src/FluidCollision.cpp`.

`NxPhysicsFluidEmitterAbiTests` calls the pinned oracle at `base + 0x87580` and the map-resolved candidate for two distinct geometry records. It compares all 20 output dwords, return values, and x86 stack balance. The existing cases set the prunable update bit, so they exercise the no-refresh path; the callback-refresh branch and invalid-index fault path are documented from the listing but are not directly exercised here.

The mutation changed candidate `output[3]` from `matrix[1]` to `matrix[2]`. After rebuilding `NxPhysics` and the ABI test, both selected-record cases reported `output=0` and the registered summary reported `mismatches=2`. Restoring `matrix[1]` made both cases report `output=1`, `stack=1/1`, and `mismatches=0`; the direct target exited successfully. Raw mutant and restored transcripts are retained in `evidence/raw/phase7-fluid-shape-packer-003540/`.

The direct-oracle marker is now required by Phase 5 and Phase 7, whose coverage floors rise to 2,661 and 1,688. The Phase 7 closure ledger records this row as `oracle_differential_falsified`; the broader dirty static-collision builder and production callback integration remain open.
