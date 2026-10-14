# SDK material write-lock wrapper (`phys_fn_000254`, `phys_fn_000256`)

IDA's pinned-oracle decompilation of RVA `0x0000bb90` shows `NpPhysicsSDK::addMaterial` trying the writer lock for each live Scene before dispatching to `PhysicsSDK::addMaterial`. If a later Scene is already locked, it releases acquired Scene locks in reverse order, reports `NXE_INVALID_OPERATION` at `NpPhysicsSDK.cpp:173`, and returns material index zero without changing the material count. MSVC outlines the reverse-unlock continuation at RVA `0x0000bbf0`.

`NpPhysicsSDK::addMaterial` now matches that path. The extended `NxPhysicsSDKTests` fixture creates two live Scenes and holds Scene 2's writer lock on a worker thread. The oracle refuses the operation and a separate probe confirms Scene 1 was unwound; after releasing the held lock, adding the same material succeeds at index 1 and grows the count to 2. The restored candidate matches the oracle exactly.

Both rows have independent mutation evidence. Replacing the forwarded material result with zero is caught on the unlocked-add marker (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=18`). Replacing only the reverse-unlock body with a counter decrement leaks Scene 1's lock and is caught (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=19`, `unwind_first=0` versus oracle `unwind_first=1`). The restored control is exact (`stdout_delta=0`, `stderr_exact=True`).

Fresh gates pass: Phase 2 at 10/10, Phase 3 at 563/563, and Phase 6 at 1,642/1,642 coverage assertions. The 39 gate-target tooling tests pass; inventory reports Phase 2 at 70 closed / 73 deferred, 6,338 function rows, 5,138 data objects, and zero unexplained rows. Public headers are unchanged. Full-DLL reconstruction remains open at 2,786 open code rows.

Raw mutation/control transcripts and the three affected phase-gate transcripts are in `evidence/raw/phase2-sdk-add-material-lock-000254-000256-20261014/`.
