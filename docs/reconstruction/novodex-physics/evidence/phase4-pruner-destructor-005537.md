# Phase 4 Pruner destructor: `phys_fn_005537`

IDA decompilation of the shipped `0x100f15a0` base destructor writes the base vtable, calls the registry removal helper with `this+0x34`, then frees the embedded pruning-pool boxes and objects. The reconstructed `Pruner::~Pruner` performs the same unregister-before-pool-release sequence.

The registered `NxPhysicsSceneRaycastTests` path releases a Scene containing static and dynamic pruners, then creates a new Scene with one static and one dynamic shape. The new instances must reuse the freed slot indices with incremented generation tags and timestamp 1: static `00010002.00000001`, dynamic `00010003.00000001`. The oracle and restored candidate match exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

For row-specific falsification, the candidate destructor was mutated to omit only `nxPrunerUnregister(mRegistration)`, then the DLL and harness were rebuilt. The oracle reused indices 2 and 3 with generation 1; the candidate retained the old registrations and used indices 4 and 5 instead. The test failed only on the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). After restoring and forcing a source rebuild, the focused differential returned to exact. Full dependent gates pass: Phase 4 533/533, Phase 5 2740/2740, and Phase 7 2871/2871. Compressed raw logs and their hashes are in [the evidence directory](raw/phase4-pruner-destructor-005537-20261015/).

This closes successful destructor unregistration and generation-tagged reuse. Process-pool allocation failure and capacity-exhaustion behavior remain open.
