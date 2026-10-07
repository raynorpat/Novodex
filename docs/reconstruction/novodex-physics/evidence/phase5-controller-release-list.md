# Phase 5 controller-list removal falsification

`phys_fn_002318` at `0x0005a170` (106 bytes) records a differential falsification with `stdout_delta=2` for controller linked-list removal implemented by `NxSceneInternal::releaseController` in `Physics/src/Scene.cpp`.

The pinned function removes a middle node by walking from the scene head and replacing the predecessor's `+0x30` link with the released node's next link. It reports an invalid-operation error only when the node is absent. This note records the middle-node and head-removal slice; it does not claim the rest of controller behavior is complete.

The registered `NxPhysicsSimulationTests` probe creates two controllers, then releases the older non-head controller first. It checks that the newer live controller's next link is null, that both generated actors remain registered as observed in the oracle, and that neither release reports an error. It then releases the remaining head. The clean staged-pair differential exits zero for both sides with `stdout_delta=0` and exact stderr.

For falsification, a clean `git archive` copy was configured and built with the updated test and gate registry. The mutation omitted only the predecessor-link assignment in `NxSceneInternal::releaseController`. The oracle printed `stale-next-after-first=0`; the mutant printed `stale-next-after-first=1` and exited 1. The registered differential measured `oracle_exit=0 candidate_exit=1 stdout_delta=2 stderr_exact=True`, so it rejected the mutation. The source mutation was confined to the archive copy and was not retained in this tree.

## Actor destruction: `phys_fn_000030`

`phys_fn_000030` at `0x00001c40` (403 bytes) is reconstructed by `nxActorDestroy` in `Physics/src/Scene.cpp`. IDA's listing shows the wrapper/name/root teardown, scene-root removal, dynamic pose copy-back, body-table removal and observer notification, `DynamicBody::destruct`, allocator release, actor-ID recycling, root-keyed pair-record cleanup, and root deletion. The candidate follows that order through the existing Scene and body/shape helpers; root-pair records are cleared in the root's base-shape destructor by `NxSceneRemoveOwnerPairRecords`.

The registered `NxPhysicsActorShapeMutationTests` differential falsifies the row when the root deletion call is omitted from `nxActorDestroy`: the mutant is detected with `stdout_delta=12`, both processes exiting zero and exact stderr (`build/phase5-actor-destroy-root-mutation.log`). Clean actor lifecycle, shape mutation and effector differentials pass in `build/phase5-actor-destroy-green.log`.

`phys_fn_000030` mutation measurement: omitted actor-root deletion, caught with `stdout_delta=12` by `NxPhysicsActorShapeMutationTests`.

## Shape factory mesh arm: `phys_fn_000032`

IDA's switch at `0x10001de0` handles the five public shape types. Case 4 constructs the 0xe8-byte mesh shape, invokes its slot-12 descriptor loader, stores the two NpScene lock links in the public handle, increments Scene+0x10, and calls `phys_fn_000503` to grow the Scene's work buffers to cover the larger of the internal mesh's +8 and +0xc counts. The candidate previously loaded the mesh but omitted the last two side effects.

The `NxPhysicsMeshSimulationTests` probe reports the mesh-shape count and buffer capacity for a small mesh and a 900-vertex/300-triangle mesh. The shipped pair reports `(1, 256)` then `(2, 1024)`; the pre-fix candidate reported `(0, 256)` then `(0, 256)`. The candidate now increments Scene+0x10 and calls `nxSceneUpdateActorCount` with the larger mesh count. The clean staged-pair differential matches (`stdout_delta=0`, exact stderr; `build/phase5-shapefactory-mesh-growth-green.log`).

`phys_fn_000032` mutation measurement: omitted the mesh Scene+0x10 increment and mesh-count-driven buffer reserve; caught by the registered `NxPhysicsMeshSimulationTests` differential with `stdout_delta=4` (`build/phase5-shapefactory-mesh-mutation.log`). The target is registered in Phase 5 as well as the existing mesh/contact gates. The clean Phase 5 run passes all 15 targets (`build/phase5-shapefactory-final.log`).

The Phase 7 `NxPhysicsPairFlagTests` regression isolates the root-ID recycle case: set actor-pair flags, release the actor, create a replacement, and verify the recycled root ID is `1 -> 1` while the old flags are cleared. Oracle and candidate both report `flags=00000000` with `stdout_delta=0` and exact stderr (`build/phase5-pair-cleanup-final-diff.log`). The final Phase 5 gate also passes (`build/phase5-pair-cleanup-final.log`). These checks cover actor destruction and pair-record cleanup; they do not close any other Phase 5 rows.
