# Phase 5 box slot-11 zero-center row

`phys_fn_000939` (`BoxShape::nxBoxZeroCenterAndDiagonal`) zeroes the output center and writes the diagonal length. The registered `NxPhysicsObjectLayoutTests` box-row fixture constructs a unit box, invokes slot 11, and compares its output with the pinned oracle.

For the mutation audit, the first zero output was changed to `1.0f`. The candidate then reported `boxrow2 candidate ok=0`, with digest `598082de` instead of the oracle digest `2f2dc4eb`, and the differential ended with `layout candidate mismatches=1` (exit 1). After restoring the source and rebuilding, the candidate reported `boxrow2 candidate ok=1`, digest `2f2dc4eb`, and zero mismatches (exit 0).

The assertion floor remains 2,308 because this proof uses existing registered observations. No public Physics header or production behavior changed.
