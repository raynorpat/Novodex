# SDK pointer-binding thunk — `phys_fn_003683`

IDA identifies RVA `0x0008aea0` as a 17-byte thiscall thunk: the key arrives in ECX, the value is its single stack argument, and both are forwarded to `phys_fn_000480`. The candidate now exposes the private MSVC member symbol `?setValue@SdkPointerBindingThunk@@QAE_NPAX@Z` at preferred address `0x10053b90` in `PhysicsInternal.obj`. Existing internal pointer-binding call sites route through this thunk; no public Physics headers changed.

The registered `NxPhysicsSimulationTests` probe resolves the oracle row by RVA and the candidate member by map symbol, then checks insertion, replacement, removal, and cleared lookup against `nxGetSdkPointerBinding`. The restored candidate reports `set=1 update=1 remove=1 cleared=1 mismatches=0`; its full staged differential passes with both processes exiting zero, `stdout_delta=0`, and exact stderr.

A temporary candidate mutation made `setValue` return false without updating the registry. The row probe reported `set=0 update=0 remove=0 cleared=1 mismatches=1`; the differential rejected the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4106`). A clean rebuild from restored source returned the exact control output. The marker is registered for Phases 5, 6, and 7 through the shared gate target table. Full gates pass at 2,849/2,849 (Phase 5), 1,723/1,723 (Phase 6), and 2,982/2,982 (Phase 7).

Raw captured transcripts and their SHA-256 digests are in [`raw/phase7-sdk-pointer-binding-thunk-003683-20261017/`](raw/phase7-sdk-pointer-binding-thunk-003683-20261017/).
