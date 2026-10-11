# Phase 7 NxScene::checkResults status-mask branch (phys_fn_000396)

Oracle row phys_fn_000396 is RVA 0x0000d710 (45 bytes). IDA 12.1.2 at VA 0x1000d710 shows checkResults returns true immediately when status bit 0 is clear. When bit 0 is set, the function waits on the scene completion event with timeout zero for nonblocking calls or INFINITE for blocking calls, returning the event result.

tests/PhysicsSimulationTests.cpp asserts through the public NxScene interface that checkResults(static_cast<NxSimulationStatus>(0), false) returns true. The fixture leaves its existing transcript unchanged, preserving the pinned oracle-only lines and Phase 6/7 registered coverage counts.

Mutation: in NpScene::checkResults, change the return for a status with bit 0 clear from true to false. The candidate-only Win32 rebuild was rejected by NxPhysicsSimulationTests: oracle exit 0, candidate exit 1, candidate stderr FAIL checkResults returned pending for an unrequested status, and stdout_delta=3846. The one-line mutation was restored and rebuilt; the complete simulation target then passed with both exits zero, stdout_delta=0, and exact stderr.

Exact captured runner transcripts are retained losslessly as gzip files at raw/phase7-checkresults-status-mask-000396/mutant-staged-pair.log.gz and raw/phase7-checkresults-status-mask-000396/restored-control-staged-pair.log.gz. Inputs were status 0 and block=false. The staged Physics hashes were oracle 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c and restored candidate c62e09b53cf575f6c540f1cb44fcc99f09c401761cd634259b5dfe3f9e84da8e; Foundation hashes were oracle 7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990 and candidate d30e43dff46391931be74243c9d7859b049aa7cdeadc2a3435d17b26b48b5970.

The staged-pair command was powershell.exe -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot <worktree> -BuildRoot <worktree>\build; the candidate was rebuilt with cmake --build build --config Release --target NxPhysics --parallel 8. The test executable was built with cmake --build build --config Release --target NxPhysicsSimulationTests --parallel 8. The source worktree was based on mainline commit d53b8ebdaf49bbc3a97405d73506329015ec7fd5.

The .log.gz files can be decompressed with standard gzip tools. The Phase 7 closure ledger records this row as differentially falsified. Phase 7 advances to 84 closed / 477 remaining function rows; Phase 8 terminal closure remains open. No public headers changed.
