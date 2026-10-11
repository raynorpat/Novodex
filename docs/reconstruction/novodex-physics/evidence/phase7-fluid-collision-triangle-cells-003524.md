# Phase 7 fluid collision triangle-cell helper — `phys_fn_003524`

Reconstructed RVA `0x866d0` as `nxFluidCollisionTriangleCells` in `Physics/src/FluidCollision.cpp`. The helper expands the triangle bounds by the requested margin, floors each bound to signed 16-bit cell coordinates, reserves candidate storage, tests cells in z/y/x order with triangle-box SAT, appends packed XYZ triples, and returns the final Z-layer count. No public Physics header changed.

The direct `NxPhysicsFluidEmitterAbiTests` differential covers three cases: a positive-coordinate triangle (3 cells), a negative-coordinate triangle with margin (10 cells), and a degenerate point triangle with zero initial capacity (1 cell and capacity growth to 2). It compares packed outputs, capacity, return value, canaries, and x86 stack balance. The restored release candidate reports oracle digest `86d21a58`, 14 total oracle cells, and zero mismatches.

The test first failed with the implementation absent. A mutation that suppressed every candidate cell write was then rejected in all three cases (`mismatches=3`); after restoring the implementation, the direct differential passed exactly. Raw logs are under `evidence/raw/phase7-fluid-collision-triangle-cells-003524/`.

The pinned UE3 binary directory does not contain `FluidModel.dll`, which normally installs the indirect `triBoxOverlap` and `Int16Vector3ArrayGrow` callbacks. For this row-level differential, the fixture binds an independent SAT triangle-box reference and array-growth reference into the oracle's extension slots. This verifies the helper's bounds, quantization, iteration, output, growth, and return behavior against the oracle given those callback contracts. Production FluidModel callback integration and Unreal-level static-collision validation remain open, as do caller `phys_fn_003528`, builder `phys_fn_003505`, and manager path `phys_fn_003630`.

Fresh Phase 5 and Phase 7 gates pass at 2,646/2,646 and 1,673/1,673 registered assertions. The full reconstruction tooling suite passes all 810 tests. The unit bundle is regenerated at `units/gap__fluids__Fluid.cpp__to__fluids__FluidManager.cpp.md`.
