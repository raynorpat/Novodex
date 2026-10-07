# Phase 5 controller-list removal falsification

`phys_fn_002318` at `0x0005a170` (106 bytes) records a differential falsification with `stdout_delta=2` for controller linked-list removal implemented by `NxSceneInternal::releaseController` in `Physics/src/Scene.cpp`.

The pinned function removes a middle node by walking from the scene head and replacing the predecessor's `+0x30` link with the released node's next link. It reports an invalid-operation error only when the node is absent. This note records the middle-node and head-removal slice; it does not claim the rest of controller behavior is complete.

The registered `NxPhysicsSimulationTests` probe creates two controllers, then releases the older non-head controller first. It checks that the newer live controller's next link is null, that both generated actors remain registered as observed in the oracle, and that neither release reports an error. It then releases the remaining head. The clean staged-pair differential exits zero for both sides with `stdout_delta=0` and exact stderr.

For falsification, a clean `git archive` copy was configured and built with the updated test and gate registry. The mutation omitted only the predecessor-link assignment in `NxSceneInternal::releaseController`. The oracle printed `stale-next-after-first=0`; the mutant printed `stale-next-after-first=1` and exited 1. The registered differential measured `oracle_exit=0 candidate_exit=1 stdout_delta=2 stderr_exact=True`, so it rejected the mutation. The source mutation was confined to the archive copy and was not retained in this tree.
