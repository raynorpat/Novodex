# Phase 3 row `phys_fn_001853`: convex-mesh contact entry

The five-byte cdecl thunk at RVA `0x00044500` enters `nxContactConvexMeshEntry`. Its registered corpus marker now records 10,000 oracle and candidate calls, 7,964,768 checks per side, and zero mismatches.

A no-op thunk mutation (`xor eax,eax; ret`) was rejected by the direct collision differential with 11,580 mismatches and no access violation. Restoring the thunk returned the differential to zero mismatches. The fresh Phase 3, Phase 4, and Phase 7 gates pass at 987/987, 564/564, and 3,109/3,109 coverage assertions.

All 834 reconstruction-tool tests pass. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Candidate and UE3 public-header trees each match all 80 pinned files. Phase 3 now records 124 closed / 268 deferred functions; full DLL reconstruction remains open.

The baseline, mutation, restored, oracle-proof, and gate transcripts with SHA-256 hashes are recorded in [the raw evidence manifest](raw/phase3-row-001853-convex-mesh-entry-20261019/manifest.json).
