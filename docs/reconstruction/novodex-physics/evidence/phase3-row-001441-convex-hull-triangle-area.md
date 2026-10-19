# Phase 3 row `phys_fn_001441`: convex-hull triangle area

The `hull_leaf` family calls the reconstructed `nxHullTriangleArea` row at RVA `0x0002a620` directly for 900 inputs on each DLL side. A new registered marker records those per-side invocation counts; the oracle and candidate outputs match exactly in the restored differential.

Changing the squared-length `faddp` at RVA `0x0002a6b8` to `fsubp` is rejected: the `hull_leaf` family reports 607 mismatches and the full target 1,473, with no access violation. Restoring the instruction and rebuilding returns the target to zero candidate mismatches.

Fresh Phase 3, Phase 4, and Phase 7 gates pass at 987/987, 565/565, and 3,110/3,110 coverage assertions. All 835 reconstruction-tool tests pass. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Candidate and UE3 public-header trees each match all 80 pinned files. Phase 3 now records 125 closed / 267 deferred functions; the full-census Phase 8 audit remains open with 8,413 findings. Public headers are unchanged.

The baseline, mutation, restored, oracle-proof, and gate transcripts with SHA-256 hashes are recorded in [the raw evidence manifest](raw/phase3-row-001441-convex-hull-triangle-area-20261019/manifest.json).
