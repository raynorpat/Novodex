# Phase 7 scene auxiliary constructor — `phys_fn_002415`

The scene constructor allocates a 0xa8-byte auxiliary block and calls `nxSceneAuxConstruct`. A sentinel-backed allocator fills that block with `0xcd`; the oracle and candidate then report one auxiliary allocation, 30 zeroed words, 11 untouched sentinel words, the owner pointer matching the internal scene, and zero mismatches. The candidate implementation is at `nxSceneAuxConstruct` in `Physics/src/Scene.cpp` (map address `0x1006b540`). Candidate RVA `0x0005bc10` is not mapped to this helper, so the test observes the helper through public scene creation.

Replacing the helper's first zero store with `0xcdcdcdcd` leaves ten words incorrect: the candidate reports 20 zero words and exits 1 while the oracle remains unchanged. Restoring the source yields exact differential output (`stdout_delta=0`, exact stderr). The full Phase 7 gate passes at 3,111/3,111 registered coverage assertions. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows; the tooling suite passes 836 tests, and both public-header trees pass 80/80 files.

The raw bundle contains compressed baseline, mutation, restored, full Phase 7, and tooling logs. `manifest.json` records SHA-256 hashes for each compressed and uncompressed log.
