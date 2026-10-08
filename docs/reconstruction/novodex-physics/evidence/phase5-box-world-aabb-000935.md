# Phase 5 box slot-9 world-AABB row

`phys_fn_000935` (`BoxShape::nxBoxWorldAABB`) computes the six bounds from the box dimensions, pose-one rotation, and translation. The registered `NxPhysicsObjectLayoutTests` box-row fixture constructs a unit box, invokes slot 9, and compares the candidate outputs with the pinned oracle.

For the mutation audit, `1.0f` was added to `out[0]` after computing all six bounds. The oracle digest was `8428d8b5`; the mutant reported `boxrow3 candidate ok=0`, produced digest `c573f6a8`, and failed with `layout candidate mismatches=1` (exit 1). After restoring the source and rebuilding, the candidate reported `boxrow3 candidate ok=1`, digest `8428d8b5`, and `layout candidate mismatches=0` (exit 0).

The pinned oracle SHA-256 was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The assertion floor remains 2,308 because this proof uses existing registered observations. No public Physics header or production behavior changed.
