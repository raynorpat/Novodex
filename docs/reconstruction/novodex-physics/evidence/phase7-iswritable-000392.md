# Phase 7 NpScene::isWritable row (phys_fn_000392)

Oracle row phys_fn_000392 is RVA 0x0000d660 (31 bytes). IDA 12.1.2 at VA 0x1000d660 shows the method calls the scene write-lock try operation, returns false when acquisition fails, releases the lock after successful acquisition, and then returns true.

The registered public NxPhysicsSimulationTests staged-pair fixture records simulation writable=initial 1. No new output or target was added.

The candidate-only mutation changed the successful return in NpScene::isWritable from true to false after the write lock had been released, then rebuilt NxPhysics.dll. The oracle and candidate both exited zero with exact stderr; the registered staged-pair comparison rejected the changed public result with stdout_delta=2. Restoring the source and rebuilding returned the entire simulation target to both exits zero, stdout_delta=0, and exact stderr.

Exact captured runner transcripts are retained losslessly as gzip files at raw/phase7-iswritable-000392/mutant-staged-pair.log.gz and raw/phase7-iswritable-000392/restored-control-staged-pair.log.gz. The mutation Physics hash was 6bcb202dfbe2379fdfc6c33a5e96ffd5c7f6d92845c450e4eb06c4b7c526e966; restored candidate Physics hash was fad1967615a034064c68e7410840d7128dce139db5a6b144637125f68080e6d6. Oracle Physics hash is pinned to 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c. Oracle Foundation hash was 7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990; candidate Foundation hash was d30e43dff46391931be74243c9d7859b049aa7cdeadc2a3435d17b26b48b5970.

The candidate was rebuilt with cmake --build build --config Release --target NxPhysics --parallel 8. The public simulation test was rebuilt with cmake --build build --config Release --target NxPhysicsSimulationTests --parallel 8; the differential was run with powershell.exe -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot <worktree> -BuildRoot <worktree>\build. The source worktree was based on mainline commit 345bef0de593e8648ae368d8ba247f3eebe05cdf.

This proof covers the successful unlocked path observed by the fixture. The false-on-contention branch remains supported by the pinned IDA contract and is not claimed as dynamically exercised. Phase 7 advances to 85 closed / 476 remaining function rows; Phase 8 remains open. No public headers changed.
