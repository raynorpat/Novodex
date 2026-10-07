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


## Box hull facade vertex pointer: `phys_fn_000955`

IDA identifies RVA `0x00020d30` as `lea eax,[ecx+0x10]`: the `BoxHullFacade::vertices()` slot returns the facade's vertex array at `+0x10`. `NxPhysicsShapeVtableTests` reaches this slot through the box hull rebuild and load paths, then compares all 117 serialized hull words against the pinned oracle.

The unmodified candidate matches the pinned DLL: `box hull oracle_digest=e0477220 cases=314 failures=0`. A temporary mutation changed `BoxHullFacade::vertices()` to return `mVertices + 1`; the same test rejected it with `box hull oracle_digest=e0477220 cases=314 failures=30` (all 30 mismatches were rebuild/load cases). The mutation was reverted, and the restored target returned to 314/314 with zero failures. No public Physics header changed.

`phys_fn_000955` mutation detection: `mismatches=30`.

## Box hull facade face count: `phys_fn_000961`

IDA identifies RVA `0x000213c0` as `mov eax,6; ret`: facade slot 3 reports the six faces used by box hull rebuild and support queries. The clean `NxPhysicsShapeVtableTests` run matches the pinned oracle at `box hull oracle_digest=e0477220 cases=314 failures=0` (`build/phase5-facecount-clean.log`). A temporary mutation changed the return value from six to five; the same target exited 1 with `box hull oracle_digest=e0477220 cases=314 failures=44` (`build/phase5-facecount-mutation.log`). After restoring the six-face return, the target again passed with zero failures. No public Physics header changed.

`phys_fn_000961` mutation detection: `mismatches=44`.

## Box hull facade face record: `phys_fn_000963`

The face-record getter at RVA `0x000213d0` returns `&mFaces[index]` (`this + 0x70 + index * 36`). The original hull differential did not call facade slot 4 directly, so the support and rebuild tests could not falsify a broken getter. `NxPhysicsShapeVtableTests` now asks both vtables for each of the six records and checks the returned pointer against the expected in-object record address. The clean run passes all six new assertions and reports `box hull oracle_digest=e0477220 cases=320 failures=0` (`build/phase5-facegetter-clean.log`). A temporary cyclic-index mutation fails all six direct assertions and exits 1 (`build/phase5-facegetter-mutation.log`). No public Physics header changed.

`phys_fn_000963` mutation detection: `mismatches=6`.

## Remaining Box hull facade accessors

`NxPhysicsShapeVtableTests` now invokes facade slot 1 directly and checks its
vertex count is eight. It also calls slots 6, 7, and 8 and compares all 24
words of each edge, face-corner, and adjacency table between the pinned oracle
and candidate. The clean differential reports `box hull oracle_digest=e0477220
cases=324 failures=0` (`build/shape-vtable-accessor-clean-build.log`).

Each row was falsified independently: `vertexCount()` returning one fewer than
`kVertexCount` failed the direct count assertion; `edgeTable()` returning the
face-corner table failed the direct word comparison; `faceCornerTable()`
returning the edge table failed its direct comparison; and `adjacencyTable()`
returning the edge table failed its direct comparison. Every mutated target
exited 1. The mutations were restored before the clean run. Public headers are
unchanged.

- `phys_fn_000953`: count mutation caught, `mismatches=1`.
- `phys_fn_000967`: edge-table mutation caught, `mismatches=1`.
- `phys_fn_000969`: face-corner table mutation caught, `mismatches=83`.
- `phys_fn_000971`: adjacency-table mutation caught, `mismatches=83`.

## Reverse strided range helper: `phys_fn_000002`

`NxPhysicsRangeIterationTests` already covers descending visitation, negative
stride, the final callback result, and zero-count behavior. Mutating the loop
update from `address -= stride` to `address += stride` made the target fail
three named checks; the first reported `check_failed three values are visited
in descending address order`. The restored implementation passes the focused
Release test. The row is classified `statically_reviewed`, since this target
compares against the recovered disassembly contract rather than a pinned-oracle
transcript. No public header changed.

## Object-layout helper rows: `phys_fn_000004` and `phys_fn_000012`

`NxPhysicsObjectLayoutTests` compares the `miscsm` object-layout fixture
against the pinned oracle. For `phys_fn_000004` at RVA `0x00001070`, changing
the subobject forwarder to pass null instead of the caller's argument produced
oracle digest `9460eb64` and candidate digest `bb22aa65`; the target exited 1
with one candidate mismatch. The restored target reports `miscsm candidate
ok=1 digest=9460eb64` and zero candidate mismatches.

For `phys_fn_000012` at RVA `0x00001430`, changing the freelist pop cursor
update from `cursor -= 4` to `cursor += 4` produced candidate digest
`85d7355f` against oracle digest `9460eb64`; the target exited 1 with one
candidate mismatch. The restored counter and freelist cases match the oracle.
Both mutations were reverted before the clean target run. Public headers are
unchanged.

- `phys_fn_000004`: forwarded-argument mutation caught, `mismatches=1`.
- `phys_fn_000012`: freelist cursor mutation caught, `mismatches=1`.

## Linear damping getter: `phys_fn_000050`

The registered `NxPhysicsActorDynamicSetterTests` target pins `setter damping=3ecccccd.3f19999a.3ecccccd.3f19999a`, covering the dynamic body-record `+0xb8` load through the public getter. The same target checks the static actor's zero return and invalid-operation report. An isolated mutation changed the load to `+0xb4`; the candidate getter became zero and the pair differential caught it with `stdout_delta=2` (both processes exited zero, stderr exact). The clean staged-pair differential has `stdout_delta=0`.

`phys_fn_000050` mutation detection: wrong getter offset `+0xb4` caught with `stdout_delta=2`.


## Angular damping getter: `phys_fn_000052`

`NxPhysicsActorDynamicSetterTests` already pins both damping getters in `setter damping=3ecccccd.3f19999a.3ecccccd.3f19999a`; the angular getter must read body record `+0xbc`. A separate mutation redirected it to `+0xb8`, returning the linear value `0.4` instead of angular `0.6`; the differential caught it with `stdout_delta=2` (both processes exited zero, stderr exact). The restored clean target passes with `stdout_delta=0`.

`phys_fn_000052` mutation detection: wrong getter offset `+0xb8` caught with `stdout_delta=2`.


## Sleep-velocity getters: `phys_fn_000066` and `phys_fn_000068`

The required `NxPhysicsActorDynamicSetterTests` line `setter sleep_thresholds=3d800000.3e800000.3e800000.3f000000` pins both raw squared thresholds and public getter results. The linear getter reads body record `+0xd0`; the angular getter reads `+0xd4`. An isolated mutation redirected the linear getter to `+0xd4`, changing the candidate line to `3d800000.3e800000.3f000000.3f000000`; the registered differential caught it with `stdout_delta=4` (both processes exited zero, stderr exact). A separate mutation redirected the angular getter to `+0xd0`, changing the candidate line to `3d800000.3e800000.3e800000.3e800000`; the differential caught it with `stdout_delta=2` (both processes exited zero, stderr exact). The restored clean build matches the oracle with `stdout_delta=0`.

`phys_fn_000066` mutation detection: wrong getter offset `+0xd4` caught with `stdout_delta=4`.

`phys_fn_000068` mutation detection: wrong getter offset `+0xd0` caught with `stdout_delta=2`.


## Sleep-threshold setters: `phys_fn_000184` and `phys_fn_000186`

The required `NxPhysicsActorDynamicSetterTests` output pins the raw squared thresholds, both public getters, and the dirty-state bits. The linear setter writes `threshold * threshold` to body record `+0xd0`; the angular setter writes to `+0xd4`. An isolated linear-setter mutation redirected `+0xd0` to `+0xd4`, changing the candidate `setter sleep_thresholds` line to `3cb851ec.3e800000.3e19999a.3f000000`; the registered differential caught it with `stdout_delta=4` (both processes exited zero, stderr exact). A separate angular-setter mutation redirected `+0xd4` to `+0xd0`, changing the same line to `3e800000.3ca0902e.3f000000.3e0f5c29`; the differential caught it with `stdout_delta=2` (both processes exited zero, stderr exact). The restored clean build matches the oracle with `stdout_delta=0`.

`phys_fn_000184` mutation detection: wrong setter offset `+0xd4` caught with `stdout_delta=4`.

`phys_fn_000186` mutation detection: wrong setter offset `+0xd0` caught with `stdout_delta=2`.


## Sleep and wake transitions: `phys_fn_000062`, `phys_fn_000064`, `phys_fn_000192`, and `phys_fn_000194`

The registered `NxPhysicsActorDynamicSetterTests` output pins group sleeping, individual sleeping, wake counters, put-to-sleep flags, and the wake-state transitions. Four independent source mutations were each caught: changing the group scan from `> 0.0f` to `>= 0.0f` made a fully asleep group report awake (`stdout_delta=4`); reversing the individual asleep comparison inverted its awake/asleep/rewake results (`stdout_delta=8`); redirecting `wakeUp` counter storage from record `+0x84` to `+0x80` changed the wake and negative-wake states (`stdout_delta=10`); and redirecting `putToSleep` from `+0x84` to `+0x80` left the actor and group awake (`stdout_delta=6`). Both processes exited zero and stderr was exact for each mutation. The restored clean differential matches the oracle with `stdout_delta=0`.

`phys_fn_000062` mutation detection: registered sleep/wake observation caught the mutation with `stdout_delta=4`.

`phys_fn_000064` mutation detection: registered sleep/wake observation caught the mutation with `stdout_delta=8`.

`phys_fn_000192` mutation detection: registered sleep/wake observation caught the mutation with `stdout_delta=10`.

`phys_fn_000194` mutation detection: registered sleep/wake observation caught the mutation with `stdout_delta=6`.


## Body-flag mutations: `phys_fn_000188` and `phys_fn_000190`

The registered `NxPhysicsActorBodyFlagTests` differential pins the actor body-flag word, manager copy, dirty-queue state, allocation, and kinematic transition. Replacing `raiseBodyFlag`'s `record + 0x10c |= flag` with a clear operation changed `bodyflag_raised` from `1.1.101` to `0.0.0` and altered transition state; the differential caught it with `stdout_delta=16`. Replacing `clearBodyFlag`'s `record + 0x10c &= ~flag` with a set operation changed `bodyflag_cleared` from `1.0.1` to `1.1.101`; it was caught with `stdout_delta=12`. Both processes exited zero and stderr was exact. The restored clean target matches the oracle with `stdout_delta=0`.

`phys_fn_000188` mutation detection: body-flag raise mutation caught with `stdout_delta=16`.

`phys_fn_000190` mutation detection: body-flag clear mutation caught with `stdout_delta=12`.
