# Phase 5 BOX slot-4 wrapper — phys_fn_000947

The registered `NxPhysicsObjectLayoutTests` `boxslot4` fixture drives 64 combinations: four third-pose values, eight low-flag masks, and two densities. It compares the boolean return and all 13 `MassFrame` words against the shipped oracle. The restored implementation passes all 64 cases.

Mutation: changed `BoxShape::nxBoxAccumulateMass` to return `false`. The first case failed immediately with `FAIL boxslot4 pose=0 low=0 density=0 returns=1/0` (exit 1). The original `return true` implementation was restored, rebuilt, and rerun; the differential returned `boxslot4 candidate cases=64 failures=0` and `layout candidate mismatches=0` (exit 0).

The gate now pins the `boxslot4 candidate cases=64 failures=0 provisional=1` transcript line. This closes the wrapper return contract; it does not claim that the shared mass-integration kernels have been fully reconstructed.
