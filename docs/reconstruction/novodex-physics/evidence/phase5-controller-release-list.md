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


## Body-flag read: `phys_fn_000080`

The `NxPhysicsActorBodyFlagTests` differential checks `readBodyFlag` after descriptor initialization, after raising the disable-gravity flag, and after clearing visualization. Inverting the method's masked-bit predicate changed `bodyflag_descriptor`, `bodyflag_raised`, and `bodyflag_cleared`; the mutation was caught with `stdout_delta=6` (both processes exited zero, stderr exact). The restored target matches with `stdout_delta=0`.

`phys_fn_000080` mutation detection: inverted masked-bit predicate caught with `stdout_delta=6`.


## Actor-flag methods: `phys_fn_000074`, `phys_fn_000076`, and `phys_fn_000078`

The registered `NxPhysicsActorMetadataTests` target pins descriptor flags, public reads, and body storage before and after raising/clearing flags. Three isolated mutations were caught: replacing `raiseActorFlag`'s OR with a clear changed the raised line and produced `stdout_delta=4`; replacing `clearActorFlag`'s clear with OR changed the cleared line and produced `stdout_delta=2`; inverting `readActorFlag`'s masked-bit test changed descriptor/default/raised/cleared reads and produced `stdout_delta=8`. Both processes exited zero and stderr was exact for each mutation. The restored clean differential matches the oracle with `stdout_delta=0`.

`phys_fn_000074` mutation detection: raise mutation caught with `stdout_delta=4`.

`phys_fn_000076` mutation detection: clear mutation caught with `stdout_delta=2`.

`phys_fn_000078` mutation detection: read mutation caught with `stdout_delta=8`.
## Actor group accessors: `phys_fn_000112` and `phys_fn_000114`

The registered `NxPhysicsActorMetadataTests` target observes the actor group
through descriptor initialization and after `setGroup`. In separate mutation
builds, `setGroup` was redirected from actor-body offset `+0x1c` to `+0x18`,
which changed `metadata_raised` and was caught with `stdout_delta=2`; `getGroup`
was redirected from `+0x1c` to `+0x18`, changing `metadata_descriptor` and
`metadata_raised`, and was caught with `stdout_delta=4`. Oracle and candidate
processes exited zero and stderr matched in both mutation runs. After restoring
the implementation, the clean differential matches exactly (`stdout_delta=0`,
`stderr_exact=True`). These results close `phys_fn_000112` and
`phys_fn_000114`. No public Physics headers changed.
import json
from pathlib import Path
base=Path('docs/reconstruction/novodex-physics')
p=base/'gates/phase5-closure.json'
d=json.loads(p.read_text(encoding='utf-8'))
d['deferred']=[r for r in d['deferred'] if r['id'] not in {'phys_fn_000112','phys_fn_000114'}]
d['counts']['deferred_reconstructed_not_falsified']=len([r for r in d['deferred'] if r['reason']=='reconstructed_not_falsified'])
p.write_text(json.dumps(d,indent=2,ensure_ascii=False)+'\n',encoding='utf-8',newline='\n')
PY
@'
- `phys_fn_000112`: wrong `setGroup` write offset caught with `stdout_delta=2`.
- `phys_fn_000114`: wrong `getGroup` read offset caught with `stdout_delta=4`.
- `phys_fn_000112`: wrong `setGroup` write offset caught with `stdout_delta=2`.
- `phys_fn_000114`: wrong `getGroup` read offset caught with `stdout_delta=4`.
## Dynamic-state virtual: `phys_fn_000110`

The registered `NxPhysicsActorLifecycleTests` target calls `NxActor::isDynamic`
for a static actor and a dynamic actor. The oracle reports `0` and `1`. An
isolated mutation reversed the body-record predicate; the candidate reported
`1` and `0`, which the paired differential caught with `stdout_delta=4` while
both processes exited zero and stderr remained exact. After restoring the
predicate, the clean differential reports `stdout_delta=0` and exact stderr.
This closes `phys_fn_000110` only for the exercised static/dynamic body-state
query. Public Physics headers are unchanged.

- `phys_fn_000110`: inverted isDynamic predicate caught with `stdout_delta=4`.

## `NpActorVtable::createShape` invalid-descriptor return: `phys_fn_000070`

The registered `NxPhysicsActorDynamicSetterTests` public-API differential calls
`createShape` with an invalid descriptor on static and dynamic actors and pins
the false result, validation diagnostic, and unchanged actor state. For
falsification, the invalid-descriptor branch was changed to return the actor
pointer cast as `NxShape*` after reporting the same diagnostic and releasing
the lock. Both oracle and mutant exited zero, stderr matched exactly, and the
paired transcript differed by four lines (`stdout_delta=4`). The mutation was
restored; the clean staged-pair differential is exact (`stdout_delta=0`,
`stderr_exact=True`). This closes `phys_fn_000070` for the exercised
invalid-descriptor contract. No public Physics headers changed.

- `phys_fn_000070` mutation detection: non-null invalid-descriptor return caught with `stdout_delta=4`.

After restoration, the staged `NxPhysicsActorDynamicSetterTests` differential
matches exactly (`stdout_delta=0`, `stderr_exact=True`). At that checkpoint,
the complete Phase 5 gate passed all 15 staged targets, static/oracle proofs,
and 2,251/2,251 coverage assertions (`build/phase5-create-shape-final.log`).

## `NpActorVtable::releaseShape` bounded release: `phys_fn_000072`

The new `NxPhysicsActorReleaseShapeProbeTests` target creates a static actor
with two box shapes, releases one handle, and verifies the surviving handle and
count before normal teardown. Its clean staged-pair differential matches
exactly. Replacing the `nxActorReleaseShape` dispatch with `(void)shape` in a
temporary candidate build leaves the second shape registered and changes two
transcript lines (`stdout_delta=2`); both processes exit zero and stderr stays
exact. The source was restored and rebuilt, and the clean target again matches
exactly. This bounded fixture catches the row without entering the exhaustive
target's release-count loops. Full mutation details are in
`evidence/phase5-release-shape-probe.md`.

- `phys_fn_000072` mutation detection: omitted inner release dispatch caught with `stdout_delta=2`.

After this closure, Phase 5 passes all 16 staged targets, static/oracle proofs,
and 2,252/2,252 coverage assertions.

## `NpActorVtable::updateMassFromShapes`: `phys_fn_000164`

The existing `NxPhysicsActorShapeMutationTests` target exercises mass updates
across primitive families, grouped shapes, planes, triggers, invalid inputs,
and refreshed body records. Removing the row's first `+0x198` record-version
increment changed 80 transcript lines (`stdout_delta=80`); both processes
exited zero and stderr matched exactly. Restoring the increment returned the
target to `stdout_delta=0` with exact stderr. Detailed evidence and log names:
`evidence/phase5-update-mass-mutation.md`.

- `phys_fn_000164` mutation detection: omitted mass-frame version increment caught with `stdout_delta=80`.

The refreshed Phase 5 gate passes all 16 staged targets and 2,252/2,252
coverage assertions after the `updateMassFromShapes` closure
(`build/phase5-update-mass-final.log`). After the following `setDynamic` row
closure, it passes again at the same floor (`build/phase5-set-dynamic-final.log`).

## `NpActorVtable::setDynamic`: `phys_fn_000122`

The existing `NxPhysicsActorShapeMutationTests` target exercises static-to-
dynamic conversion for single and grouped shapes, body creation from density,
replacing an existing dynamic body, and a shape-less body. Suppressing the
static-pruner removal/re-add route's final `nxSceneAddShape` call left the
converted shape out of the dynamic pruner and changed 36 transcript lines
(`stdout_delta=36`); both processes exited zero and stderr matched exactly.
Restoring the call returned the differential to `stdout_delta=0` and exact
stderr. Full mutation details: `evidence/phase5-set-dynamic-mutation.md`.

- `phys_fn_000122` mutation detection: skipped dynamic pruner registration caught with `stdout_delta=36`.

## `ShapeBase::nxBaseSlot7`: `phys_fn_001035`

The three plane final-vtable slot-7 cases in `NxPhysicsShapeVtableTests` compare
the return value and untouched output word against the pinned DLL. Changing
the candidate stub from `false` to `true` produced three failures in the
626-case target (`mismatches=3`; the target prints `failures=3`). Restoring the
stub and rebuilding returned the target to `failures=0`; the box sweep and hull
checks also remained green. Full evidence: `evidence/phase5-shape-base-sweep-mutation.md`.

- `phys_fn_001035` mutation detection: changing the base sweep return is caught with `mismatches=3` (the target prints `failures=3`).

## `NpActorVtable::saveToDesc`: `phys_fn_000120`

The registered `NxPhysicsActorLifecycleTests` differential now assigns distinct
actor groups to the static and dynamic fixtures (3 and 7). The clean target
matches exactly and reports those saved groups along with pose, density, flags,
user data, and preserved descriptor fields. Changing the saved group read from
body+0x1c to body+0x18 is caught with `stdout_delta=8`; changing saved density
from body+0x18 to body+0x14 is caught with `stdout_delta=6`. Both mutated pairs
exit zero with exact stderr. After each mutation, the source bytes are restored,
the DLL/test target rebuilt, and the clean differential returns to
`stdout_delta=0`, `stderr_exact=True`.

- `phys_fn_000120` mutation detection: wrong group source caught with `stdout_delta=8`; wrong density source caught with `stdout_delta=6`.

## `NpActorVtable::getPointVelocityVal`: `phys_fn_000146`

The registered lifecycle differential exercises static, dynamic, rotated, and
quarter-turn actors plus its quaternion/mass-offset/point/velocity grid. The
clean output is exact. Changing the final world-Z velocity accumulation in the
Win32 x87 path from addition to subtraction changes the public point-velocity
transcript (`stdout_delta=38`); both processes exit zero and stderr matches.
Restoring the source byte-for-byte and rebuilding returns the differential to
`stdout_delta=0`, `stderr_exact=True`. Full evidence:
`evidence/phase5-world-point-velocity-mutation.md`.

- `phys_fn_000146` mutation detection: changed the final Z accumulation from addition to subtraction; caught with `stdout_delta=38`.

## `NpActorVtable::getLocalPointVelocityVal`: `phys_fn_000148`

The same lifecycle fixture compares local-point velocity for static, dynamic,
rotated, and quarter-turn actors plus its quaternion/frame/point/velocity grid.
Changing the final local-Z accumulation in the Win32 x87 path from addition to
subtraction changes the transcript (`stdout_delta=38`); both processes exit
zero and stderr matches. Restoring the source byte-for-byte and rebuilding
returns the differential to `stdout_delta=0`, `stderr_exact=True`. Full
evidence: `evidence/phase5-local-point-velocity-mutation.md`.

- `phys_fn_000148` mutation detection: changed the final local-Z accumulation from addition to subtraction; caught with `stdout_delta=38`.

## `NpActorVtable::saveBodyToDesc`: `phys_fn_000046`

The registered lifecycle differential checks the null-record static actor and
dynamic, rotated, and quarter-turn body descriptors. Replacing the gathered
result with `false` leaves the output bytes unchanged but changes the public
success result for the three dynamic actors; the paired transcript catches it
with `stdout_delta=6`. Both processes exit zero with exact stderr. Restoring
the source byte-for-byte, rebuilding, and rerunning returns to
`stdout_delta=0`, `stderr_exact=True`. Full evidence:
`evidence/phase5-save-body-desc-mutation.md`.

- `phys_fn_000046` mutation detection: forced the successful descriptor-gather return to false; caught with `stdout_delta=6`.

## Actor force and torque dispatch rows

The registered `NxPhysicsActorForceTests` target independently falsifies three
public vtable rows. Swapping the force and position arguments in
`addForceAtPos` (`phys_fn_000054`) is caught with `stdout_delta=18`; changing
`addForce`'s force accumulator selector from `false` to `true`
(`phys_fn_000056`) is caught with `stdout_delta=36`; changing `addTorque`'s
selector from `true` to `false` (`phys_fn_000058`) is caught with
`stdout_delta=30`. For all three, oracle and mutant exit zero and stderr is
exact. Each byte-restored rebuild returns to `stdout_delta=0` with exact stderr.
Full evidence: `evidence/phase5-actor-force-dispatch-mutations.md`.

- `phys_fn_000054` mutation detection: swapped force and position arguments; `stdout_delta=18`.
- `phys_fn_000056` mutation detection: routed force into the torque accumulator; `stdout_delta=36`.
- `phys_fn_000058` mutation detection: routed torque into the force accumulator; `stdout_delta=30`.

## `NpActorVtable::getGlobalOrientationQuatVal`: `phys_fn_000094`

The lifecycle differential covers the static matrix-conversion arm and dynamic
identity, rotated, and quarter-turn quaternion reads. Changing the dynamic
record source from record+0x5c to record+0x60 changes the rotated outputs and is
caught with `stdout_delta=4`; both processes exit zero with exact stderr. The
source is restored byte-for-byte, rebuilt, and the clean differential returns
to zero delta. Full evidence:
`evidence/phase5-orientation-quat-mutation.md`.

- `phys_fn_000094` mutation detection: shifted the dynamic quaternion source by four bytes; caught with `stdout_delta=4`.

## Actor center-of-mass, velocity, inertia, and momentum getters

Seven additional rows are independently falsified through registered public
path differentials:

- `phys_fn_000096` (`getCMassLocalPoseVal`): shifted the rotation source from
  record+0xdc to +0xe0; `NxPhysicsActorCMassTests` caught it with
  `stdout_delta=56`.
- `phys_fn_000098` (`getCMassLocalPositionVal`): shifted the position source
  from +0x100 to +0x104; the same target caught it with `stdout_delta=56`.
- `phys_fn_000100` (`getCMassLocalOrientationVal`): shifted the matrix source
  from +0xdc to +0xe0; the same target caught it with `stdout_delta=58`.
- `phys_fn_000102` (`getMassSpaceInertiaTensorVal`): shifted the inertia read
  from +0x18c to +0x190; `NxPhysicsConvexMeshTests` caught it with
  `stdout_delta=2`.
- `phys_fn_000104` (`getLinearVelocityVal`): shifted the velocity read from
  +0x6c to +0x70; `NxPhysicsActorDynamicsTests` caught it with
  `stdout_delta=2`.
- `phys_fn_000106` (`getAngularVelocityVal`): shifted the velocity read from
  +0x78 to +0x7c; the same target caught it with `stdout_delta=2`.
- `phys_fn_000108` (`getLinearMomentumVal`): sourced mass from +0x18c instead
  of +0x188; `NxPhysicsActorMomentumTests` caught it with `stdout_delta=4`.

All mutant processes exited zero with exact stderr. Each source was restored
byte-for-byte, rebuilt, and its clean target returned to `stdout_delta=0` with
exact stderr. Full evidence: `evidence/phase5-actor-accessor-mutations.md`.
- Mutation index: `phys_fn_000096` `stdout_delta=56`; `phys_fn_000098` `stdout_delta=56`; `phys_fn_000100` `stdout_delta=58`; `phys_fn_000102` `stdout_delta=2`; `phys_fn_000104` `stdout_delta=2`; `phys_fn_000106` `stdout_delta=2`; `phys_fn_000108` `stdout_delta=4`.


## Box mass-frame builder — `phys_fn_000829`

The registered `NxPhysicsShapeVtableTests` mass-frame differential covers nine box extent sets. The baseline reports `cases=201 failures=0` (`build/shape-vtable-000829-baseline.log`). A throwaway archive mutation changed the xx diagonal sum from `zz + yy` to `zz - yy`; the target reported eight mismatches, while box-sweep and box-hull sections remained exact (`build/shape-vtable-000829-mutation.log`).

`phys_fn_000829` mutation detection: `mismatches=8`. Full details: `evidence/phase5-box-massframe-000829.md`.


## Box shape mass accumulator — `phys_fn_000849`

The registered `NxPhysicsShapeVtableTests` calls Box shape slot 4 across three dimensions, two local poses, two densities, and the clear/skip flag values. The baseline reports `cases=24 failures=0` (`build/shape-vtable-000849-baseline.log`). An isolated archive mutation omitted `local.nxMassFrameScale(density)` in `BoxShape::nxBoxComputeMassFrame`; all 12 density-two calls failed while the other shape-vtable sections remained exact (`build/shape-vtable-000849-mutation.log`).

`phys_fn_000849` mutation detection: `mismatches=12`. Full details: `evidence/phase5-box-massframe-000849.md`.


## Mass-frame payload fold — `phys_fn_000831`

The registered `NxPhysicsShapeVtableTests` baseline passes all shape-vtable, boxmass, and massframe cases (`build/shape-vtable-000831-baseline.log`). An isolated archive mutation changed the first inertia result in `MassFrame::nxMassFrameFoldPayload`; two capsule and sixteen cached-mesh cases failed, while the Box slot-4, box-sweep, box-hull, and mass-frame-builder sections remained exact (`build/shape-vtable-000831-mutation.log`).

`phys_fn_000831` mutation detection: `mismatches=18`. Full details: `evidence/phase5-massframe-fold-000831.md`.


## Mass-frame recentering — `phys_fn_000841`

The registered `NxPhysicsShapeVtableTests` directly compares the oracle entry and candidate recentering method for four offset cases. Baseline: `shape vtable massframe centre oracle_digest=65953565 cases=4 failures=0` (`build/shape-vtable-000841-baseline.log`). An isolated archive mutation changed the x-offset negation to addition; three cases failed and all other sections stayed exact (`build/shape-vtable-000841-mutation.log`).

`phys_fn_000841` mutation detection: `mismatches=3`. Full details: `evidence/phase5-massframe-centre-000841.md`.


## Conditional mass-frame zeroing — `phys_fn_000847`

The registered `NxPhysicsObjectLayoutTests` compares all thirteen frame words for flag=1 and flag=0. Baseline: `mzero row=phys_fn_000847 zA=00000000 zB=42424242 digest=23206019`; the candidate matches (`build/object-layout-000847-baseline.log`). Changing only the final mass clear to `1.0f` in an isolated archive yields a candidate mismatch and exit 1 (`build/object-layout-000847-mutation.log`).

`phys_fn_000847` mutation detection: `mismatches=1`. Full details: `evidence/phase5-massframe-zero-000847.md`.


## Actor mass from shapes — `phys_fn_000008`

Restored `NxPhysicsActorMassTests` to CMake and the Phase 5 target registry. The oracle-backed target covers primitive, posed and compound shapes, explicit-mass behavior, and trigger-only refusals; baseline is exact (`stdout_delta=0`, `stderr_exact=True`; `build/actor-mass-current-main-differential.log`). A one-component density-inertia mutation in `nxActorComputeMassFromShapes` is caught with `stdout_delta=36`, successful exits on both sides, and exact stderr (`build/actor-mass-000008-mutation.log`).

`phys_fn_000008` mutation detection: `stdout_delta=36`. Full details: `evidence/phase5-actor-mass-000008.md`.


## Kinematic transition — `phys_fn_000785` and `phys_fn_000787`

The registered `NxPhysicsActorBodyFlagTests` baseline matches exactly. Changing the enable arm's first inverse-mass write from `0.0f` to `0.5f` is caught with `stdout_delta=2`; changing the disable arm's inverse-mass reconstruction to `0.5f` is independently caught with `stdout_delta=2`. Both oracle and mutant processes exit zero with exact stderr. The byte-restored control is exact (`build/kinematic-transition-restored-differential.log`; mutation runs: `build/kinematic-enable-mutation-full.log` and `build/kinematic-disable-mutation-full.log`).

`phys_fn_000785` mutation detection: `stdout_delta=2`.
`phys_fn_000787` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-kinematic-transition-000785-000787.md`.

## Base shape slots 4, 5 and 7: `phys_fn_001249`, `phys_fn_004812` and `phys_fn_001035`

The isolated `NxPhysicsShapeVtableTests` directly calls the pinned oracle entries at `NxPhysics.dll+0x24f70`, `+0xb4070` and `+0x22dd0`, then calls the corresponding candidate `ShapeBase` methods. It checks that slot 4 returns false without changing the destination, slot 5 returns null with balanced ESP around both oracle and candidate calls, and slot 7 returns false without changing its output. Slot 5's function type has five stack arguments—ray, max distance, groups, hint flags and hit—matching the oracle's `ret 0x14`. The direct probe reports `shape vtable base_stub slot4 oracle_false=1 candidate_false=1 output_preserved=1`, `shape vtable base_stub slot5 oracle_null=1 candidate_null=1 esp_balanced=1`, and `shape vtable base_stub slot7 oracle_false=1 candidate_false=1 output_preserved=1`; the overall vtable case count is 629.

The slot 4 and slot 5 checks were validated with isolated candidate source mutations. Changing slot 4 to return true caused `base shape slot 4 stub differs`, `mismatches=1`, and exit 1; writing zero through slot 4's destination while returning false was independently caught with `mismatches=1`. Changing slot 5 to return a non-null pointer caused the corresponding slot 5 failure, `mismatches=1`, and exit 1. Restoring each implementation returned the baseline to zero mismatches. The Phase 5 registered coverage floor includes these direct oracle assertions. This closes only the three base return stubs and their tested x86 calling conventions; it does not establish every derived shape caller contract.
phys_fn_001249 slot4 wrong-return mutation: mismatches=1.
phys_fn_004812 slot5 wrong-return mutation: mismatches=1.

## Root-shape cleanup guard — `phys_fn_000006`

An isolated `git archive` mutant omitted the early null-root return in
`nxActorRemoveRootFromScene`. `NxPhysicsActorShapeMutationTests` still reported
the staged pair's loaded DLL paths and hashes before entering test code; the
candidate then faulted in root removal while the oracle exited normally. The
runner compared captured output and exit status and rejected the mutant with
`oracle_exit=0 candidate_exit=-1073741819 stdout_delta=155 stderr_exact=True`.
The clean staged-pair baseline is exact (`stdout_delta=0`, exact stderr). Full
method, hashes and harness details: `evidence/root-shape-cleanup-guard-2026-10-07.md`.
- `phys_fn_000006` closes its null-root guard mutation with `stdout_delta=155`; the mutant candidate exited with an access violation while the pinned oracle exited zero. Full details: `evidence/root-shape-cleanup-guard-2026-10-07.md`.

### Kinematic global-move rows (2026-10-07)

The staged `NxPhysicsActorDynamicsTests` pair baseline and restored candidate are exact. Each rebuilt mutant was detected with both children exiting zero and exact stderr: `phys_fn_000090` `stdout_delta=16`, `phys_fn_000124` `stdout_delta=14`, and `phys_fn_000126` `stdout_delta=16`. Detailed inputs, mutation descriptions, and DLL hashes are in `evidence/phase5-kinematic-move-mutations.md`.

- `phys_fn_000028` vector-growth mutation detection: `mismatches=2`. Full details: `evidence/phase5-vector-pushback-000028.md`.

- `phys_fn_000024` no-root release-report mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-release-shape-000024.md`.

- `phys_fn_000036` create-shape return mutation detection: `stdout_delta=359` (candidate access violation after the fixture observes null instead of the installed shape handle). Full details: `evidence/phase5-create-shape-000036.md`.

- `phys_fn_000026` zero-inertia mass-build mutation detection: `stdout_delta=18`. Full details: `evidence/phase5-body-record-000026.md`.

- `phys_fn_000128` global-pose-reference z-word mutation detection: `stdout_delta=4`. Full details: `evidence/phase5-global-pose-reference-000128.md`.

- `phys_fn_000134` CMass global-pose matrix-offset mutation detection: `stdout_delta=54`. Full details: `evidence/phase5-cmass-global-pose-000134.md`.

- `phys_fn_000136` CMass global-position offset mutation detection: `stdout_delta=54`. Full details: `evidence/phase5-cmass-getters-000136-000138.md`.
- `phys_fn_000138` CMass global-orientation offset mutation detection: `stdout_delta=54`. Full details: `evidence/phase5-cmass-getters-000136-000138.md`.

- `phys_fn_000140` CMass/inertia getter mutation detection: `stdout_delta=20`. Full details: `evidence/phase5-inertia-getters-000140-000142-000144.md`.

- `phys_fn_000142` CMass/inertia getter mutation detection: `stdout_delta=24`. Full details: `evidence/phase5-inertia-getters-000140-000142-000144.md`.

- `phys_fn_000144` CMass/inertia getter mutation detection: `stdout_delta=24`. Full details: `evidence/phase5-inertia-getters-000140-000142-000144.md`.

- `phys_fn_000150` force-helper mutation detection: `stdout_delta=18`. Full details: `evidence/phase5-force-helpers-000150-000152.md`.

- `phys_fn_000152` force-helper mutation detection: `stdout_delta=4`. Full details: `evidence/phase5-force-helpers-000150-000152.md`.

### Local force-at-position wrappers (2026-10-08)

The Force target now includes a rotated-actor, offset-mass-frame `addLocalForceAtPos` case. Clean/restored output is exact; each registered mutation is detected with equal zero exits and exact stderr.

- `phys_fn_000154` mutation detection: `stdout_delta=10`. Full details: `evidence/phase5-force-wrappers-000154-000156-000158.md`.
- `phys_fn_000156` mutation detection: `stdout_delta=6`. Full details: `evidence/phase5-force-wrappers-000154-000156-000158.md`.
- `phys_fn_000158` mutation detection: `stdout_delta=4`. Full details: `evidence/phase5-force-wrappers-000154-000156-000158.md`.

### Local force and torque entry points (2026-10-08)

The registered Force differential catches each entry point’s wrong accumulator selector; clean/restored output remains exact.

- `phys_fn_000160` mutation detection: `stdout_delta=16`. Full details: `evidence/phase5-force-torque-wrappers-000160-000162.md`.
- `phys_fn_000162` mutation detection: `stdout_delta=16`. Full details: `evidence/phase5-force-torque-wrappers-000160-000162.md`.

### Actor mass and damping setters (2026-10-08)

The registered DynamicSetter differential catches each row-specific wrong-store mutation with `stdout_delta=2`; clean and restored output remains exact.

- `phys_fn_000166` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-mass-damping-wrappers-000166-000168-000170-000172.md`.
- `phys_fn_000168` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-mass-damping-wrappers-000166-000168-000170-000172.md`.
- `phys_fn_000170` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-mass-damping-wrappers-000166-000168-000170-000172.md`.
- `phys_fn_000172` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-mass-damping-wrappers-000166-000168-000170-000172.md`.

### Actor velocity and momentum setters (2026-10-08)

Registered DynamicSetter and Momentum differentials reject each row-specific mutation with equal zero exits and exact stderr; restored output matches exactly.

- `phys_fn_000174` mutation detection: `stdout_delta=6`. Full details: `evidence/phase5-velocity-momentum-wrappers-000174-000176-000178-000180-000182.md`.
- `phys_fn_000176` mutation detection: `stdout_delta=8`. Full details: `evidence/phase5-velocity-momentum-wrappers-000174-000176-000178-000180-000182.md`.
- `phys_fn_000178` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-velocity-momentum-wrappers-000174-000176-000178-000180-000182.md`.
- `phys_fn_000180` mutation detection: `stdout_delta=40`. Full details: `evidence/phase5-velocity-momentum-wrappers-000174-000176-000178-000180-000182.md`.
- `phys_fn_000182` mutation detection: `stdout_delta=18`. Full details: `evidence/phase5-velocity-momentum-wrappers-000174-000176-000178-000180-000182.md`.

### Actor global pose setters (2026-10-08)

The CMass differential catches each pose setter mutation, and clean/restored output is exact.

- `phys_fn_000196` mutation detection: `stdout_delta=112`. Full details: `evidence/phase5-global-pose-setters-000196-000198-000200-000202-000204.md`.
- `phys_fn_000198` mutation detection: `stdout_delta=196`. Full details: `evidence/phase5-global-pose-setters-000196-000198-000200-000202-000204.md`.
- `phys_fn_000200` mutation detection: `stdout_delta=56`. Full details: `evidence/phase5-global-pose-setters-000196-000198-000200-000202-000204.md`.
- `phys_fn_000202` mutation detection: `stdout_delta=392`. Full details: `evidence/phase5-global-pose-setters-000196-000198-000200-000202-000204.md`.
- `phys_fn_000204` mutation detection: `stdout_delta=218`. Full details: `evidence/phase5-global-pose-setters-000196-000198-000200-000202-000204.md`.

### Center-of-mass setter mutations (2026-10-08)

The registered `NxPhysicsActorCMassTests` differential independently caught
each setter's wrong-store mutation with both processes exiting zero and exact
stderr; restoring the source returned to an exact transcript:

- `phys_fn_000206` mutation detection: `stdout_delta=148`.
- `phys_fn_000208` mutation detection: `stdout_delta=150`.
- `phys_fn_000210` mutation detection: `stdout_delta=28`.
- `phys_fn_000212` mutation detection: `stdout_delta=52`.
- `phys_fn_000214` mutation detection: `stdout_delta=56`.
- `phys_fn_000218` mutation detection: `stdout_delta=120`.
- `phys_fn_000220` mutation detection: `stdout_delta=58`.
- `phys_fn_000222` mutation detection: `stdout_delta=60`.

Full inputs, mutation descriptions, and restored logs: `evidence/phase5-cmass-setter-mutations-000206-000222.md`.

### Center-of-mass orientation error tail (2026-10-08)

The registered DynamicSetter differential catches a source-line mutation in the static/kinematic error report contained in the tail row `phys_fn_000216`; both processes exit zero and stderr is exact.

- `phys_fn_000216` mutation detection: `stdout_delta=4`. Full details: `evidence/phase5-cmass-orientation-tail-000216.md`.


### Sphere slot-15 radius getter (2026-10-08)

- `phys_fn_001359` mutation detection: `stdout_delta=2`. Full details: `evidence/phase5-sphere-get-radius-001359.md`.


### Capsule slot-14 set-radius (2026-10-08)

- `phys_fn_000995` mutation detection: `stdout_delta=4`. Full details: `evidence/phase5-capsule-set-radius-000995.md`.


### Sphere and capsule geometry slots (2026-10-08)

- `phys_fn_001357` mutation detection: `mismatches=1`.
- `phys_fn_001363` mutation detection: `stdout_delta=2`.
- `phys_fn_001001` mutation detection: `stdout_delta=2`.
- `phys_fn_001004` mutation detection: `stdout_delta=2`.
- Full details and restored controls: `evidence/phase5-shape-accessors-001001-001004-001357-001363.md`.


### Box slot-13 save-state row (2026-10-08)

- phys_fn_000927 mutation detection: oxrow2 candidate ok=0 d13=ea972950, mismatches=1; restored run reports ok=1, mismatches=0.
- Full details: vidence/phase5-box-save-state-000927.md.


### Box world-AABB row (2026-10-08)

- phys_fn_000935 mutation detection: oxrow3 candidate ok=0, mismatches=1; restored run reports ok=1, mismatches=0.
- Full details: vidence/phase5-box-world-aabb-000935.md.


### Box rows 11 and 8 (2026-10-08)

- `phys_fn_000939` mutation detection: `boxrow2 candidate ok=0`, `mismatches=1`; restored output matches exactly.
- `phys_fn_000941` mutation detection: `boxrow3 candidate ok=0`, `mismatches=1`; restored output matches exactly.
- Full details: `evidence/phase5-box-zero-center-000939.md` and `evidence/phase5-box-local-aabb-000941.md`.


### Box slot-10 general-dimension row (2026-10-08)

- `phys_fn_000937` matches the oracle with non-unit dimensions and a translated pose; omitting `dy*dy` is caught with `mismatches=1`, while the restored differential is exact.
- IDA disassembly confirms `(dx*dx + dy*dy) + dz*dz` before `fsqrt`. Full details: `evidence/phase5-box-center-diagonal-000937.md`.


### Continuation — BOX slot-4 return closure (2026-10-08)

- `phys_fn_000947` is caught by a return-false mutation on the first of 64 cases (`returns=1/0`, with `mismatches=1`); the restored implementation passes all 64.
- Phase 5 advances to 122 closed / 83 deferred functions; the coverage floor rises to 2,309. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-box-slot4-000947.md`.

### Continuation — box-hull support bounds closure (2026-10-08)

- `phys_fn_000975` is caught by reversing the minimum comparison (`hull support candidate ok=0`; `mismatches=1`). Restoring the comparison yields exact oracle words and zero mismatches.
- Phase 5 advances to 123 closed / 82 deferred functions. The 2,309 assertion floor is unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-hull-support-000975.md`.

### Continuation — BOX slot-3 visualization dispatcher closure (2026-10-08)

- `phys_fn_000945` is caught by reversing the live visualization guard (`slot3 mismatches=1`); restoring the guard passes the 64-case candidate contract and ordering check.
- Phase 5 advances to 124 closed / 81 deferred functions. The 2,309 assertion floor is unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-box-debug-render-000945.md`.


### DynamicBody island dirty bit (`phys_fn_000748`)

- `phys_fn_000748` mutation detection: `stdout_delta=172`; oracle reports `bodycreate mark_island_dirty=3.5`, candidate mutant reports `1.5` and exits 1. The restored candidate differential is exact.
- Full details: `evidence/phase5-mark-island-dirty-000748.md`.

### Body-record mass-energy helper (`phys_fn_000742`)

- `phys_fn_000742` mutation detection: `actorsm2 candidate ok=0`, energy `422b41c8` versus oracle `4240985d`, `mismatches=1`, target exit 1. The restored target reports `ok=1`, identical energy words, and zero layout mismatches.
- Full details: `evidence/phase5-energy-x87-000742.md`.


### Group-sleep chain helper (`phys_fn_000744`)

- `phys_fn_000744` changing the group-sleep comparison from `> 0.0f` to `>= 0.0f` is caught by `NxPhysicsObjectLayoutTests`: `actorsm5 candidate ok=0`, `mismatches=1`. The restored candidate returns digest `75b57124` with zero mismatches.
- Full details: `evidence/phase5-chain-settled-000744.md`.


### Sphere and capsule mass-frame builders (`phys_fn_000851`, `phys_fn_000853`)

- `phys_fn_000851` sphere mass mutation detection: `stdout_delta=16`; both mutant processes exit 0 with exact stderr, and the restored candidate is exact.
- `phys_fn_000853` capsule mass mutation detection: `stdout_delta=18`; both mutant processes exit 0 with exact stderr, and the restored candidate is exact.
- Full details and logs: `evidence/phase5-shape-mass-000851-000853.md`.


### Capsule and sphere descriptor loads (`phys_fn_000989`, `phys_fn_001353`)

- `phys_fn_000989` radius-store mutation is caught by `NxPhysicsObjectLayoutTests`: `capload candidate ok=0`, `layout candidate mismatches=2`; restored run has zero mismatches.
- `phys_fn_001353` radius-store mutation is caught by `NxPhysicsObjectLayoutTests`: `sphload candidate ok=0`, `layout candidate mismatches=2`; restored run has zero mismatches.
- Full details and logs: `evidence/phase5-shape-load-000989-001353.md`.


### Kinematic target writer (`phys_fn_000784`)

`NxPhysicsActorDynamicsTests` compares the pinned oracle and restored candidate exactly (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000784-restored.log`). Temporarily omitting the position-target flag OR in `nxNpActorSetKinematicTarget` is caught by the same registered differential with `stdout_delta=42`; both mutant processes exit 0 and stderr is exact (`build/phase5-000784-mutant.log`). The row is now dynamically gated.
`phys_fn_000784` mutation detection: `stdout_delta=42`; both mutant processes exited 0 with exact stderr. The restored candidate differential is exact (`stdout_delta=0`, `stderr_exact=True`).


### World-mass-pose actor transform (`phys_fn_000789`)

The baseline and restored `NxPhysicsActorCMassTests` staged-pair runs are exact (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000789-baseline.log`, `build/phase5-000789-restored.log`). The temporary `actorPosition[0] + 1.0f` mutation in `nxNpActorApplyWorldMassPose` is caught with `stdout_delta=168`; both mutant processes exit 0 and stderr is exact (`build/phase5-000789-mutant.log`).
`phys_fn_000789` mutation detection: `stdout_delta=168`; restored candidate output is exact (`stdout_delta=0`, `stderr_exact=True`).


### Body-descriptor loader (`phys_fn_000793`, `phys_fn_000795`)

Baseline and restored `NxPhysicsBodyCreationTests` staged-pair runs are exact (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000793-000795-baseline.log`, `build/phase5-000793-000795-restored.log`). Both mutations run against the same shared loader implementation but alter distinct oracle-owned ranges.
- `phys_fn_000793`: adding 1.0f to the stored mass at +0x188 is caught with `stdout_delta=26`; both mutant processes exit 0 and stderr is exact (`build/phase5-000793-mutant.log`).
- `phys_fn_000795`: flipping bit 0 of the first copied linear-velocity word at +0x6c is caught with `stdout_delta=14`; both mutant processes exit 0 and stderr is exact (`build/phase5-000795-mutant.log`).
`phys_fn_000793` mutation detection: `stdout_delta=26`; `phys_fn_000795` mutation detection: `stdout_delta=14`; restored output is exact.


### Dynamic mass-frame refresh (`phys_fn_000768`)

The baseline and restored `NxPhysicsActorCMassTests` staged-pair runs are exact (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000768-baseline.log`, `build/phase5-000768-restored.log`). Adding 1.0f to the refreshed world-center X in `nxNpActorUpdateMassFrame` is caught with `stdout_delta=184`; both mutant processes exit 0 and stderr is exact (`build/phase5-000768-mutant.log`).
`phys_fn_000768` mutation detection: `stdout_delta=184`; restored output is exact (`stdout_delta=0`, `stderr_exact=True`).


### Dynamic-body record constructor (`phys_fn_000797`)

Baseline and restored `NxPhysicsBodyCreationTests` staged-pair runs are exact (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000797-baseline.log`, `build/phase5-000797-restored.log`). Adding one word to the saved world-center copy at +0x230 in `DynamicBody::construct` is caught with `stdout_delta=12`; both mutant processes exit 0 and stderr is exact (`build/phase5-000797-mutant.log`).
`phys_fn_000797` mutation detection: `stdout_delta=12`; restored output is exact (`stdout_delta=0`, `stderr_exact=True`).


### Dynamic-body base pose constructor (`phys_fn_000801`)

Baseline and restored `NxPhysicsBodyCreationTests` staged-pair runs are exact (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-000801-baseline.log`, `build/phase5-000801-restored.log`). Adding one word to the copied pose X at +0x38 in `DynamicBodyBase::construct` is caught with `stdout_delta=12`; both mutant processes exit 0 and stderr is exact (`build/phase5-000801-mutant.log`).
`phys_fn_000801` mutation detection: `stdout_delta=12`; restored output is exact (`stdout_delta=0`, `stderr_exact=True`).
