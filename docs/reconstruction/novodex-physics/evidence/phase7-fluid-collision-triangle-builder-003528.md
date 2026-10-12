# Phase 7 fluid static-collision triangle builder — `phys_fn_003528`

Reconstructed RVA `0x86b40` as `nxFluidCollisionBuildTriangleCells` in `Physics/src/FluidCollision.cpp`. The helper walks the mesh index triples, transforms each vertex by the mesh 3x4 affine transform, clears the output cell count, calls `phys_fn_003524` to enumerate overlapped signed-16-bit grid cells, then calls `phys_fn_003526` to add their bucket counts. The private mesh descriptor is 0x40 bytes, with triangle count at +0, indices at +4, vertices at +0x0c, and the twelve transform floats at +0x10. No public Physics header changed.

The direct `NxPhysicsFluidEmitterAbiTests` differential covers an identity-transformed two-triangle mesh, an asymmetrically rotated and translated two-triangle mesh, and an empty mesh. Final triangle cells (3 and 4), all seven bucket records, untouched output tails, and x86 stack balance match the oracle; the empty case preserves state. The restored control reports digest `b2825642`, seven total final cells, and zero mismatches.

A deliberate no-output mutation returns before the triangle loop. Both nonempty cases are rejected (`mismatches=2`); the empty case remains equivalent. After restoring the source and forcing a rebuild, all three controls pass exactly. Raw logs are under `evidence/raw/phase7-fluid-collision-triangle-builder-003528/`.

The test calls oracle RVA `0x86b40`, whose internal triangle-cell helper calls indirect FluidModel callbacks. The supplied UE3 binary directory has no `FluidModel.dll`, so the harness binds independent SAT and array-growth references into the oracle callback slots. The test therefore validates indexed traversal, affine transformation, per-triangle accumulation, cell outputs, hash updates, and empty handling, while production callback integration and Unreal static-collision validation remain open. Dirty builder `phys_fn_003505` and full-DLL closure also remain open.


Fresh mainline verification: Phase 5 passes at 2,647/2,647 registered assertions; Phase 7 passes at 1,674/1,674. All 810 reconstruction-tool tests pass, and inventory validation reports zero unexplained rows. The phase-gate runners also verified the immutable 80-header manifest. Concise outputs: `evidence/raw/phase7-fluid-collision-triangle-builder-003528/phase-gates.txt`.
