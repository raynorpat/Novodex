# Fluid emitter visualization (phys_fn_003607)

`phys_fn_003607` at RVA `0x886b0` reads the live visualization scale and emitter-visualization parameter, refreshes the emitter world pose, fetches the emitter dimensions and fluid rest-particle distance, and emits the emitter basis, rectangular footprint, optional frame basis, and staggered particle-circle grid. The implementation is in `Physics/src/SceneVisualize.cpp`; no public headers changed.

The registered `NxPhysicsFluidEmitterAbiTests` direct-call fixture enables both parameters and supplies a deterministic pose, dimensions, and rest distance. It compares every emitted primitive through an FNV digest and checks the x86 stack for three cases: the unattached emitter, a matrix-backed frame actor, and a quaternion-backed frame actor. All three match exactly; each frame case emits 4 lines, 2 bases, and 24 circles, and the unattached case digest is `c284be3c`. The grid transform follows the oracle's intermediate float rounding and stagger offsets.

For mutation falsification, an immediate return was inserted at the candidate function entry. The unattached case observed 4/0 lines, 2/0 bases, and 24/0 circles; the matrix and quaternion cases each independently reported a mismatch with zero candidate primitives. The fixture exited nonzero with three aggregate mismatches. Removing the mutation restored all three exact comparisons.

Fresh Win32 Release Phase 5 and Phase 7 gates pass at 2,798/2,798 and 2,920/2,920 assertions. The 816-test reconstruction-tool suite passes. Logs are retained in `evidence/raw/phase7-fluid-emitter-visualize-003607-20261016/`.
