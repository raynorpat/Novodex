# `phys_fn_004791` MemoryStream destructor differential

`MemoryStream::~MemoryStream` is the five-byte tail jump at oracle RVA `0x000b3db0` to the destructor body at `phys_fn_004784`. The Phase 4 asset differential now observes the SDK allocator frees around destruction of each stream on both sides. The pinned oracle is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

For each of the 16 penetration-map fixtures, the oracle and candidate each release the stream's data buffer and block record. The registered marker reports `cases=16 oracle_frees=32 candidate_frees=32`; the oracle digest remains `1daa095e` and the full asset target reports zero candidate mismatches.

For the mutation, the `MemoryStream` destructor body in `Physics/src/MemoryStream.cpp` was temporarily disabled and the asset test target rebuilt. All 16 cases reported `oracle_frees=2 candidate_frees=0`, the aggregate row marker reported 32 versus zero, and the candidate differential exited 1 with 16 mismatches. Restoring the destructor and rebuilding returned the exact control result. No public headers changed.

The captured control, mutant, and restored logs are in `evidence/raw/phase4-memory-stream-dtor-004791-20261016/`. The `NxPhysicsAssetTests` oracle-only proof was regenerated from the restored transcript; its existing recorded oracle digest and coverage totals did not change.

Fresh Win32 Release Phase 4 gate passes at 543/543 required coverage assertions, including the row-specific 32/32 free marker on the staged oracle/candidate run. The full gate transcript is archived with the focused controls and mutant.
