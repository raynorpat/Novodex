# Phase 5 box slot-8 local-AABB row

`phys_fn_000941` (`BoxShape::nxBoxLocalAABB`) writes the negated box extents to the minimum corner and the positive extents to the maximum corner. The registered `NxPhysicsObjectLayoutTests` box-row fixture constructs a unit box and compares all six output words with the pinned oracle.

For the mutation audit, the first minimum component was changed from negative to positive extent. The candidate reported `boxrow3 candidate ok=0`, digest `653d7035` instead of the oracle digest `8428d8b5`, and `layout candidate mismatches=1` (exit 1). After restoring the source and rebuilding, the candidate reported `boxrow3 candidate ok=1`, digest `8428d8b5`, and zero mismatches (exit 0).

The assertion floor remains 2,308 because this proof uses existing registered observations. No public Physics header or production behavior changed.
