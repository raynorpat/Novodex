# Phase 5 Pruner process-pool growth allocation failures — 2026-10-15

Extended `NxPhysicsPrunerPoolAllocationFailureTests` to fail each of the four arrays allocated when a second Scene adds a third process-pool registration: the 16-byte object array and the three 8-byte U16 arrays. The oracle and reconstructed DLL both raise access violation `0xC0000005` in isolated child processes for all four requests; the parent remains green and compares the results exactly.

```text
pruner pool growth array oom cases=4 slots=objects.forward.reverse.generations exit=c0000005.c0000005.c0000005.c0000005 mismatches=0
oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
```

A candidate-only mutation that suppresses the pool growth branch lets all four candidate children survive. The oracle passes while the candidate parent fails with four growth mismatches (`stdout_delta=10`). Rebuilding the restored source returns the staged differential to an exact match. Restored candidate `NxPhysics.dll` SHA-256: `2875ada564304a2cdd220b5ebd1a859ee2a3bb7458b2b0c568ac3042ee700871`.

Fresh Phase 5 and Phase 7 gates pass at 2,746/2,746 and 2,874/2,874 coverage assertions; all 816 reconstruction tooling tests pass. This records the oracle's unchecked allocation-failure behavior and does not claim that pool growth recovers from OOM or close a full-image code row. Evidence logs and hashes are in `raw/phase5-pruner-pool-growth-allocation-failure-20261015/`.
