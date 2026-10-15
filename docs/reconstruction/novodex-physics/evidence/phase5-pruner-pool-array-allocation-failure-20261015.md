# Phase 5 Pruner process-pool array allocation failures — 2026-10-15

Added an isolated child-process differential for the four array allocations performed by `NxPrunerProcessPool` construction: the 8-byte object array and the three 4-byte index/generation arrays. Each child creates the shipped or reconstructed DLL pair, injects failure at the corresponding allocation ordinal, and attempts to create a static actor that lazily registers a Pruner. The allocator aborts the request and the unchecked constructor path raises access violation `0xC0000005`; the parent requires that exact result for all four cases while keeping the overall test process alive.

```text
pruner pool array oom cases=4 slots=objects.forward.reverse.generations exit=c0000005.c0000005.c0000005.c0000005 mismatches=0
oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

A candidate-only mutation retried each failed array allocation once. All four children then survived, producing `mismatches=4`; the oracle passed and the candidate parent exited 1 (`stdout_delta=2`). Restoring the unchecked path returns the pair to an exact match. Restored candidate `NxPhysics.dll` SHA-256: `cf9036a9730846fa95d6653a0d021f1e924837abda4030ae256ac1f968eed0b7`.

Fresh Phase 5 and Phase 7 gates pass at 2,745/2,745 and 2,873/2,873 coverage assertions. All 816 reconstruction tooling tests pass. The test records oracle behavior; it does not claim the OOM path is recoverable or close any full-image code row. Pruner capacity growth at larger live sets, generation wraparound, and the broader DLL backlog remain open.

Compressed mutation, restored differential, gate, and tooling transcripts with SHA-256 manifest: `evidence/raw/phase5-pruner-pool-array-allocation-failure-20261015/`.
