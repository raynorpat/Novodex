# Phase 5 Pruner process-pool owner allocation failure — 2026-10-15

Extended the public Scene allocator differential to fail the SDK allocator's next 0x1c-byte allocation, which is the `NxPrunerProcessPool` owner. The test creates a Scene, lazily creates a static box actor (which attempts Pruner registration), and verifies actor creation and the Scene remain usable when the singleton allocation fails. Releasing the Scene returns the SDK Scene count to zero. Both the shipped DLL and reconstructed DLL report the same marker and allocator transcript, with no cross-frees.

```text
alloc phase=pruner_pool_owner_oom status=created scenes=1 fail_hits=1 after_release_scenes=0
oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

A candidate-only mutation removed the null-allocation early return in `nxPrunerRegister`. The oracle still passed; the candidate crashed when it continued into placement construction (`candidate_exit=-1073741819`, `stdout_delta=70`). Restoring the guard returned the DLL differential to an exact match. The restored candidate `NxPhysics.dll` SHA-256 is `1d8e92b67680e7fe78b1c0bda7bb927d5d60b2cb75e854116e4888e6a2670aaf`.

Fresh Phase 5 and Phase 7 gates pass at 2,744/2,744 and 2,872/2,872 coverage assertions. All 816 reconstruction tooling tests pass. This is test coverage for the owner-allocation failure path; no production code or public header changed. The four parallel allocations inside Pruner construction and pool growth failure cases remain open, as does the full DLL backlog.

Compressed differential, mutation, restored-control, gate, and tooling transcripts with SHA-256 manifest: `evidence/raw/phase5-pruner-pool-owner-allocation-failure-20261015/`.
