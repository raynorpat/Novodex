# Fluid particle cell-coordinate rounding — `phys_fn_003423`

IDA at oracle RVA `0x00084a80` shows the shared initial-particle routine computing cell coordinates from each copied position and cell size before storing them as signed 16-bit values. The helper used for the conversion floors its input. Existing fixtures used positions exactly on cell boundaries, so they did not distinguish floor from C++ truncation toward zero.

`NxPhysicsFluidCellRoundingTests` now drives positive and negative non-integral coordinates, `2.75` and `-2.75`. The oracle stores `2` and `-3`. Before the production change, the candidate stored `2` and `-2` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). `nxFluidInitializeParticles` now uses `floorf` for each coordinate. The restored oracle/candidate differential passes exactly (`stdout_delta=0`, `stderr_exact=True`). A temporary `truncf` mutation is rejected with the same `2` versus `-2` mismatch; rebuilding the restored source returns the exact differential.

The row's existing fixture family also covers strided position/velocity/lifetime channels, capacity clipping, zero-count allocation, null backend-particle results, allocation failure, normal array creation, append/growth, and failed growth. The null-count early return and zero-cell-size branch now have dedicated, mutation-tested differentials. The row remains open because the full instruction sequence has not been audited.

Candidate implementation is `nxFluidInitializeParticles` in `Physics/src/Scene.cpp`, map address `0x10067cb0`. The public Physics headers are unchanged.

The reconstruction-tool suite passes all 835 tests and 743 subtests. Fresh Release Phase 5, 6, and 7 gates pass at 2,791/2,791, 1,692/1,692, and 2,911/2,911 coverage assertions. Inventory validation reports 6,338 function rows, 5,138 data objects, and zero unexplained records. Raw focused and phase logs are retained in `evidence/raw/phase7-physfn-003423-20261015/` and `evidence/raw/phase7-physfn-003423-zero-cell-20261015/`.
