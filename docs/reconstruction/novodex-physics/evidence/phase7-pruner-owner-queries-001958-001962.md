# Scene pruner section-one owner queries (`phys_fn_001958`, `phys_fn_001962`)

IDA confirms both rows read a Prunable owner from the section-one pool. `phys_fn_001958` uses the static pruner at engine slot 0; `phys_fn_001962` reads the engine selector at +0x70 and uses that pruner slot. Both add the section-0 count to the requested section-one index, read the object pointer from `mObjects`, and return `mOwner` at +0x04. The candidate methods in `Physics/src/opcode/IcePruningEngine.cpp` retain the x86 thiscall ABI and are resolved through their map symbols.

The registered `NxPhysicsSceneRaycastTests` fixture seeds distinct section-zero and section-one objects in separate static and selected-dynamic pools. Both indices in each query return the expected owner tokens on the pinned oracle and reconstructed DLL. The restored staged pair is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Omitting the section-zero count in either method is independently rejected (`candidate_exit=1`, `stdout_delta=2814`, one row mismatch); the other method stays exact.

Compressed mutation/control transcripts, mutation instructions, candidate map addresses, and hashes are in [raw/phase7-pruner-owner-queries-001958-001962-20261017](raw/phase7-pruner-owner-queries-001958-001962-20261017/).

Fresh registered Phase 4, 5, and 7 gates pass at 553/553, 2,865/2,865, and 3,004/3,004 coverage assertions, respectively. The Phase 7 SimulationTests child-process checks and both owner-query markers passed in the final rerun.
