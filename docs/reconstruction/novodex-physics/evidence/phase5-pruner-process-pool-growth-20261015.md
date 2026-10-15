# Phase 5 Pruner process-pool growth — 2026-10-15

Extended `NxPhysicsSceneLifecycleTests` to create static and dynamic anchor actors in three simultaneously live Scenes. This instantiates six Pruner registrations and crosses the process pool's initial capacity of two. The oracle and candidate both report the expected active handles `0` through `5`; after releasing the middle Scene, the appended Scene reuses the released slots at generation 1, with static handle `0x00010003` and dynamic handle `0x00010002`.

The restored staged pair is exact:

```text
scene registration growth active=6 handles=00000000.00000001.00000002.00000003.00000004.00000005 stable=1
scene registration growth reused static=00010003 dynamic=00010002 stable=1
oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

A candidate-only mutation disabled the `NxPrunerProcessPool::add` capacity-growth branch. The oracle passed, while the candidate crashed during the third registration (`candidate_exit=-1073741819`, `stdout_delta=5`, exact stderr). Rebuilding the restored source returned the pair to an exact match. Restored candidate `NxPhysics.dll` SHA-256: `044ca51c11fcb6d5c08f9fdeb2dcbc19934ac9a6996fcad4660c733482b398b8`.

The Phase 5 gate passed at 2,743/2,743 coverage assertions, and all 816 reconstruction tooling tests passed. No production code or public header changed; this packet closes the direct test gap for growth and reuse, while allocation-failure behavior and the full DLL backlog remain open.

Compressed gate, mutation, restored-differential, and tooling transcripts with SHA-256 manifest are in `evidence/raw/phase5-pruner-process-pool-growth-20261015/`.
