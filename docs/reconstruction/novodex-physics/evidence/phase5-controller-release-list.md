# Phase 5 controller-list removal falsification

## Box identity vtable row: `phys_fn_001391`

The Box vtable shares the three-byte self-return method at RVA `0x00027f00` across slots 14–16. `NxPhysicsShapeVtableTests` calls slot 14 on the pinned oracle and candidate and checks that each returns its own object address. The clean target reports `shape vtable oracle_digest=ed1294b6 cases=626 failures=0` (`build/shape-self-001391-green.log`).

A throwaway `git archive` copy changed the private `BoxShape::nxBoxSelf` implementation to return null. Its oracle/candidate check reported `shape vtable oracle_digest=ed1294b6 cases=626 failures=1` and exited 1 (`build/shape-self-001391-mutation.log`). The single failed identity comparison for `phys_fn_001391` is recorded as `mismatches=1`. The generated project was restored and the target was clean-built with the original private header; the baseline again reported zero failures. This closes only the shared identity method, not the remaining Box vtable slots. Details: `evidence/shape-self-001391.md`.

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

## Actor descriptor userData: `phys_fn_000034`

IDA identifies `phys_fn_000034` at `0x10002010` as `Actor::loadFromDescInternal`; the call at `0x10011dac` comes from `Scene::createActor` (`0x10011730`). The descriptor contract in `NxActorDescBase` says `userData` is copied to `NxActor::userData`. The candidate previously left the wrapper's `+4` field holding its temporary Scene link, even though the body had already copied that Scene link to `body+4`.

`NxPhysicsActorMetadataTests` now creates an actor with sentinel descriptor userData and reports whether the returned actor preserves it. Before the fix, the shipped DLL reported `metadata_user_data=1` and the candidate `0` (`stdout_delta=2`, exact stderr). `nxActorLoadFromDescInternal` now copies the descriptor value into the wrapper after scene registration; both sides report `1` and the complete differential matches (`stdout_delta=0`, exact stderr; `build/phase5-next/actor-metadata-userdata-green.log`). The omitted-copy state is the row's mutation and is rejected by the registered Phase 5 test. Public Physics headers remain unchanged.

`phys_fn_000034` mutation detection: `stdout_delta=2`.

## Dynamic body auxiliary registration: `phys_fn_002421`

IDA decompilation identifies `phys_fn_002421` at `0x1005c160` as the auxiliary-manager record insertion called by `DynamicBodyBase::construct` at `0x1001ba79`. It reads the body ID from record `+0x104`, ensures the manager's sparse record table at `aux+0x80` covers that ID, stores the record at exactly that slot, and calls `phys_fn_002417` to add the ID to the manager's indexed and active arrays. `nxSceneAuxRegisterRecord` now grows these indexed arrays in 256-slot chunks and grows the active list when it fills.

The registered `NxPhysicsBodyCreationTests` target releases IDs 0 and 5 out of order, then checks the LIFO-reused ID 5 and ID 0 map to their own record slots. It also registers 257 concurrent bodies to cross the first chunk boundary and checks ID `0x100`. Oracle and candidate report the same manager state (`stdout_delta=0`, exact stderr; `build/phase5-next/body-creation-aux-registration-green.log`). A mutation that clears the indexed record-pointer write is rejected with `stdout_delta=6` (`build/phase5-next/aux-registration-mutation.txt`). The test exits after identity reporting for the high-water stress case because the shipped DLL's separate high-ID release path remains under study.

`phys_fn_002421` mutation detection: `stdout_delta=6`.

The Phase 7 `NxPhysicsPairFlagTests` regression isolates the root-ID recycle case: set actor-pair flags, release the actor, create a replacement, and verify the recycled root ID is `1 -> 1` while the old flags are cleared. Oracle and candidate both report `flags=00000000` with `stdout_delta=0` and exact stderr (`build/phase5-pair-cleanup-final-diff.log`). The final Phase 5 gate also passes (`build/phase5-pair-cleanup-final.log`). These checks cover actor destruction and pair-record cleanup; they do not close any other Phase 5 rows.
