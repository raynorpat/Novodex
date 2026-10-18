# Phase 7 element-count helper: `phys_fn_003477`

`phys_fn_003477` at RVA `0x00085580` is a 10-byte member helper that returns the signed element count `([this+8] - [this+4]) >> 2`. The candidate implementation is `nxLockedElementCount` in `Physics/src/ObjectModel.cpp`; callers pass the containing object's field offset, which is zero in the direct helper fixture.

The `NxPhysicsObjectLayoutTests` oracle/source differential now calls the oracle row and candidate helper for positive, empty, and negative spans. The restored run reports `elemcount row=phys_fn_003477 cases=3 mismatches=0` and passes with exit 0.

For mutation testing, the candidate arithmetic shift was changed from 2 to 3. The direct row fixture reported `mismatches=2`; the registered oracle/source gate also rejected the mutant through the dependent count-wrapper check. The restored implementation was rebuilt and the differential returned to zero mismatches.

Fresh Phase 5, 6, and 7 gates pass with 2,927/2,924, 1,778/1,777, and 3,063/3,060 coverage assertions. The Phase 5 oracle proof also validates the pinned harness/fixture-source bundle and retains oracle digest `3acf0dc7`. Raw logs and the executable/oracle hashes are recorded in `evidence/raw/phase7-element-count-003477-20261018/`.
