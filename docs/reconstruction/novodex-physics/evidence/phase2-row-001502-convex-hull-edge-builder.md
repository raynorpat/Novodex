# Phase 2 row `phys_fn_001502`: convex-hull edge-builder entry

The 298-byte entry at RVA `0x0002cb50` sums polygon face counts, allocates four temporary arrays, and continues into the edge builder. The registered `NxPhysicsThirdPartyTests` marker proves this row executes on ordinary, direct-without-polygon, and rebuild paths: 91 calls total (66 main calls, 3 direct calls without polygons, and 25 rebuild calls).

The restored differential passes with exact convex-hull output. Inverting the first allocator-success branch at RVA `0x0002cbb9` changes the candidate hull word count from 32,993 to 18,611; the oracle differential reports three candidate mismatches and exits 1 without a crash. The source was restored, the target rebuilt, and the restored differential passes again.

The fresh Phase 4 gate passes at 564/564 assertions. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. All 833 reconstruction-tool tests pass, and candidate and UE3 public-header trees each match all 80 pinned files. Public headers are unchanged. Phase 2 now records 104 closed / 39 deferred functions. The full-census Phase 8 audit remains open with 8,413 findings.

The compressed baseline, mutation, restored, and Phase 4 transcripts with SHA-256 values are recorded in [the raw evidence manifest](raw/phase2-row-001502-convex-hull-edge-builder-20261019/manifest.json).
