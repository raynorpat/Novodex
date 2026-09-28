# NpActor.cpp: contract

The public actor API unit (`NpActor.cpp` in `work_units.json`, evidenced span 0x2610-0xb100,
87 code rows, 53 of them `discovered` (30,673 B) and 34 `reconstructed`). Written by Task 1
of the NpActor.cpp completion plan (`evidence/npactor-completion.md`): an audit of every row
against its Capstone listing (authoritative; `oracle/capstone/manifest.json`), with the
manifest decompiles (unit bundle, `tools/unit_bundle.py`, not committed) as hints only, and a
cdb trace of the candidate over the twelve Phase 5 actor staged-pair targets
(`evidence/npactor-trace.txt`). Nothing was fixed in this task; the rows' inventory state is
unchanged.

Candidate files at commit 63fd8cd: `Physics/src/NpActor.cpp` (class `NpActorVtable`, the
concrete class whose vtable every actor wrapper carries), `Physics/src/include/NpActor.h`,
`Physics/src/include/NpActorDynamicMath.h`, `Physics/src/include/NpSceneGuard.h`, the shape
helpers `nxActorAppendShape`/`nxActorRemoveShape` and `NxSceneInternal::releaseActor` in
`Physics/src/Scene.cpp`, and the listing models ("OM") of already-reconstructed rows in
`Physics/src/ObjectModel.cpp`. Line numbers below are of that commit.

## Dispatch tables

The actor public table is `phys_data_000679` at .rdata 0x10104530 (the inventory's
`dispatch_table` object lists only its first 8 targets). Its words were read from the image
through the PE section table and each names a function entry; slot i is mapped to the i-th
virtual of `Physics/include/NxActor.h` in declaration order, as `units/revolute-contract.md`
mapped the NxJoint slots. Two points make the mapping line up exactly:

- `setStatic()` is inside a comment block in `NxActor.h` (`AM: This doesn't work yet`), so it
  has no slot: `isDynamic` is slot 19 and `setCMassOffsetLocalPose` slot 20.
- `getPointVelocity`/`getLocalPointVelocity` are inline virtuals with bodies in the header;
  their out-of-line copies are 000038 (0x2400) and 000040 (0x2430), slots 63 and 64, in the gap
  before this unit.

Slots 36 (`getMass`, 000048 at 0x25c0) and 81 (`saveBodyToDesc`, 000046 at 0x24c0) are rows of
the gap unit `Actor.cpp..NpActor.cpp`, not of this unit. Word 87 of the table (0x1010468c) is
000116, the first word of the next table: the member (actor+8) sub-object table whose only
entry is the this-adjusting thunk to slot 0 (`sub ecx,8; jmp 000118`). The 88 words therefore
are 87 NxActor slots plus that thunk. The candidate's own `??_7NpActorVtable@@6B@` follows the
same declaration order (checked in the built DLL: slot 19 isDynamic, 63/64 the inline
`NxActor::getPointVelocity`/`getLocalPointVelocity`, 86 getGroup) and has one extra word 87,
`setGlobalPose(const NxVec3&, const NxMat33&)` (NpActor.cpp:2610, an empty body that is not an
oracle row and that no NxActor slot reaches; it is the "setGlobalPose" the plan lists as
unimplemented), where the oracle has the member table's thunk.

Non-vtable rows: 000150 (0x5e70, called by 000156/000158/000160/000162) rotates a local vector
by the body orientation; 000152 (0x5fa0, called by 000154/000158) maps a local point to world;
000216 (0xa5d0) is the tail of 000214 (no prologue, 000214's registers and frame, reached by
`jmp 0xa5d0` at 0xa5cb and eight conditional jumps; Capstone seeded the loop-top jump target as
an entry). 000044 (0x2480, gap unit), the actor constructor, is the "caller" the bundle lists
for every slot: those are table-install edges, not calls.

## Conventions and cross-cutting tags

"NA" is the `NpActorVtable` method the public slot dispatches to (what the staged-pair tests
run); "OM" is the ObjectModel.cpp listing model of an already-reconstructed row (driven by the
NxPhysicsObjectLayoutTests oracle differential, not by the public path). The verdict in the
table is for the NA form; the OM verdict is in the summary.

Status: `implemented` (a body exists for everything the row does, possibly with defects),
`partial` (a named part of the row's behaviour is absent), `missing` (empty body or no
candidate function). Verdict: `faithful` (every block checked matches) or `defect`, with the
class in parentheses: E = the only defects are E1 (and H1); H = H1 only; S = S1 (and H1);
X = substantive arithmetic/field/branch differences; M = missing.

- **G1** (every write-guarded row): on a failed write-lock try (002364 at 0x5b730 returns 0)
  the oracle reports `FoundationSDK::error(2, NpActor.cpp, <row line>, 0, "PhysicsSDK:
  WriteLock is still aquired. Procedure call skipped to avoid a deadlock!")` and returns
  without unlocking; `nxNpSceneGuardWriteTry` (NpSceneGuard.h) returns silently. The rows'
  G1 lines are in the detail below. G1 alone does not stop a row from being `faithful`.
- **E1**: a precondition error the oracle reports (kind 1, file 0x10104690
  `\Epic\Novodex\SDKs\Physics\src\NpActor.cpp`, a row-specific line and message; usually
  "Actor must be dynamic!", "Actor must be (non-kinematic) dynamic!", "Actor must be
  kinematic!" or "Cannot be called on a static actor!") where the candidate skips silently.
  A per-row defect; the line is given with each row. Kinematic = `test byte [rec+0x10c]; js`,
  the same bit as the candidate's `& 0x80`.
- **H1** (every row that marks the record dirty; D1's review calls it S2):
  `nxNpActorMarkRecordDirty` (NpActor.cpp:91) returns without marking when `[rec+0x120]`,
  the flags array, or any of the dirty-list begin/end/capacity pointers is null, and when the
  body id `[rec+0x11c]` is >= 256. The oracle's inline sequence has no bound and no null
  test: with an unallocated list (begin == 0) it grows to 2 entries through the allocator,
  pushes the id and ORs the mask. The growth size (2n+2), copy/free order and index-table
  write otherwise match. Whether the null-list case is reachable from the public API was not
  established (all twelve differentials pass with stdout_delta=0).
  H1 also covers the allocator: the helper grows and frees the list through
  `nxGetSdkAllocator()` (NpActor.cpp:107-111), where every oracle inline growth goes through
  the import [0x101041bc] `nxFoundationSDKAllocator` (e.g. 0x100082f1 malloc, 0x10008329
  free), and it adds an `if(!grown) return` the oracle lacks. `nxNpActorTransitionKinematic`
  (NpActor.cpp:130) has the same class of defect: 000785 allocates the 0x20-byte kinematic
  state at 0x1001995d and frees it at 0x10019cda through [0x101041bc] with no null check,
  and in the oracle the dirty marks 0x10000/0x20000/0x80000 (and any list growth they cause)
  precede the malloc, while the candidate allocates first and marks once at the end.
- **NG** (group R): the NA method takes no scene lock at all where the oracle brackets the
  read with 002362/002366 on [actor+0x10] (or the write-try on [actor+0xc]).
- **SSE**: `NpActor.cpp` is not on the `/arch:IA32` list in `CMakeLists.txt` (only Geometry,
  MassProperties, the narrow phase, ShapeRaycast and the joint units are), so plain float
  expressions in it compile to SSE scalar code and round every product and sum to float,
  and `double`-typed chains are SSE2 double. The `__asm` blocks are x87 and unaffected. Any
  row whose arithmetic is written in plain C++ is x87-faithful only where its roundings
  happen to coincide. For finite values a `double`-typed SSE2 chain equals the oracle's x87
  at its 53-bit precision control, so the fix for rows like 000182 and 000789 is the
  listing's source order with `double` register lifetimes and `float` spills; `float`-typed
  expressions need `double` lifetimes or `__asm`. `/arch:IA32` is still needed wherever NaN
  payload/propagation matters (the reason the joint units are on the list).
- G1, E1, H1 and SSE as described above are the state at commit 63fd8cd; Task 2 fixed all
  four (see `## Task 2: the cross-cutting pass`).
- **S1** (000196-000202; 000204-000208 lack the call entirely): the oracle ends with
  `000004(body, 1)` (`nxForwardSubobjectCall`, ObjectModel.cpp:3650), a virtual call of slot 6
  on [body+0x10]. The candidate's `nxNpActorNotifyOwnedShapes` (NpActor.cpp:847) calls
  `ShapeBase::nxApplyOwnerUpdate(1)` directly and, for a kind-5 group, loops over the
  children. For the four single-shape families slot 6 is that method. For a group it is
  not: the group vtable 0x10106c2c slot 6 is 001018 (0x227d0, 62 B, `discovered`), which calls
  each child's slot 6 and then 001315(flags) on the group itself. `nxNpActorNotifyOwnedShapes`
  omits that group-level 001315 call, so for grouped actors S1 is a real behavioural defect.
  A faithful 000004 dispatch fixes it provided the candidate group's slot 6 behaves like
  001018 (001315 is itself `discovered`/partial). Closed by Task 4: the group has its own
  table with slot 6 = 001018, and `nxNpActorNotifyOwnedShapes` is the plain 000004 call.
- **ROT**: the body rotation from the quaternion at [rec+0x5c] is built with one x87 sequence
  (five float spills: 2yy, 2xz, 2yw, 1-2xx, 2yz) in every row that needs it.
  `nxNpActorRotationFromQuaternionGetter` (asm, NpActor.cpp:970) and `nxNpActorComposeRotation`
  (NpActorDynamicMath.h:242) reproduce it; `nxNpActorRotationFromQuaternion[At]`
  (NpActorDynamicMath.h:18-38, double, no spills) and `NxMat33(NxQuat)` (SSE float) do not.
- **RF**, **T746**, **FAP**, **A782**: the rotation-times-mass-frame operand orders per row,
  the 000746 world-tensor helper (`nxNpActorWorldTensorRDRt` models it; `nxNpActorWorldTensor`
  does not), 000791 add-force-at-position (lever.x kept on the x87 stack, one 000782 call), and
  000782 (the body's force/torque accumulator) -- defined in the group B and A details.
- Withdrawn in review: a claimed wake-comparison defect (`<=` vs `<`, "W1" in the group A
  notes). `fcomp [0x101053d4]; fnstsw; test ah,5; jp skip` skips when C0 = C2 = 0 (greater or
  equal: equal sets only C3) and when unordered, so the oracle wakes only on an ordered strict
  `[rec+0x84] < 0.39999998f`, which is what the candidate tests. Every wake block in the unit
  (000174-000182, 000204-000222, 000782, 000784) has this form.

Constants read from the image: [0x101053d4] = 0x3ecccccc (0.39999998f), [0x101041ec] = 1.0f,
[0x101041f0] = 0.0f, [0x101043cc] = 0.5f, 0x10122078 = the identity 3x3, 0x10123c1c = (0,0,0).
The allocator import [0x101041bc] is `nxFoundationSDKAllocator` (vtable +8 malloc(size, 0),
+0x14 free).

## Rows

Traced: hits of the candidate function in `evidence/npactor-trace.txt` (candidate sha256
fb64a931...), per target with `NxPhysics`/`Tests` dropped. A hit shows the candidate
function was entered, not that every block ran; a hit on 000132/000092 can come from
000130's candidate, which calls both, and 000092 also from 000126's. The staged-pair cases
driving each row are the targets listed. NxPhysicsEmptySceneTests reaches no row;
NxPhysicsDynamicFirstTests reaches only the 000118 counterpart.

| Row | RVA | B | State | Slot / role | Candidate | Status | Verdict | Traced (hits: targets) | Summary |
|---|---|---:|---|---|---|---|---|---|---|
| 000050 | 0x00002610 | 107 | reconstructed | slot 42 getLinearDamping | NpActor.cpp:1774 `getLinearDamping`; OM ObjectModel.cpp:1161 | implemented | faithful | 2: ActorDynamicSetter 1, ActorDynamics 1 | NA: E1 0xd9 (the setLinearDamping literal) reproduced by Task 2. OM faithful |
| 000052 | 0x00002680 | 107 | reconstructed | slot 44 getAngularDamping | NpActor.cpp:1799 `getAngularDamping`; OM ObjectModel.cpp:1181 | implemented | faithful | 2: ActorDynamicSetter 1, ActorDynamics 1 | NA: E1 0xe8 reproduced by Task 2. OM faithful |
| 000054 | 0x000026f0 | 167 | reconstructed (Task 7) | slot 54 addForceAtPos | NpActor.cpp:1993 `addForceAtPos` | implemented | faithful | 1: ActorForce 1 | Task 3: 000791 (r.x kept in the register, r.y/r.z spilled, torque components rounded once, one 000782 call) and 000782 from the listing (x unrounded in modes 0/3, angular rows (I2z + I1y) + I0x with the listing spills, one wake after both arms, mode > 4 still wakes); G1 0x12a, E1 0x12b and H1 from Task 2 |
| 000056 | 0x000027a0 | 165 | reconstructed (Task 7) | slot 58 addForce | NpActor.cpp:2094 `addForce` | implemented | faithful | 12: ActorForce 12 | Task 3: 000782 from the listing (see 000054); G1 0x14d, E1 0x14e and H1 from Task 2 |
| 000058 | 0x00002850 | 165 | reconstructed (Task 7) | slot 60 addTorque | NpActor.cpp:2135 `addTorque` | implemented | faithful | 14: ActorForce 14 | Task 3: 000782 angular arm from the listing (see 000054); G1 0x160, E1 0x161 and H1 from Task 2 |
| 000060 | 0x00002900 | 72 | reconstructed | slot 62 computeKineticEnergy | NpActor.cpp:2156 `computeKineticEnergy`; OM ObjectModel.cpp:1119 | implemented | faithful | 5: ActorMomentum 5 | Task 3: the 000742 order: I_k w_k in registers, (vz vz + vy vy) + vx vx times m, ((m v.v + I2 w2 w2) + I1 w1 w1) + I0 w0 w0 halved, rounded to float. OM faithful |
| 000062 | 0x00002950 | 60 | reconstructed | slot 67 isGroupSleeping | NpActor.cpp:2336 `isGroupSleeping`; OM ObjectModel.cpp:3429 | implemented | faithful | 14: ActorDynamicSetter 14 | NA and OM faithful |
| 000064 | 0x00002990 | 73 | reconstructed | slot 68 isSleeping | NpActor.cpp:2360 `isSleeping`; OM ObjectModel.cpp:1136 | implemented | faithful | 6: ActorDynamicSetter 6 | NA and OM faithful |
| 000066 | 0x000029e0 | 75 | reconstructed | slot 69 getSleepLinearVelocity | NpActor.cpp:2372 `getSleepLinearVelocity`; OM ObjectModel.cpp:839 | implemented | faithful | 4: ActorDynamicSetter 4 | NA and OM faithful (sqrtss == fsqrt+fstp) |
| 000068 | 0x00002a30 | 75 | reconstructed | slot 71 getSleepAngularVelocity | NpActor.cpp:2397 `getSleepAngularVelocity`; OM ObjectModel.cpp:863 | implemented | faithful | 3: ActorDynamicSetter 3 | NA and OM faithful |
| 000070 | 0x00002a80 | 185 | reconstructed (Task 7) | slot 13 createShape | NpActor.cpp:1172 `createShape` | implemented | faithful | 1: ActorShapeMutation 1 | Task 4: 000036(body, desc) (Scene.cpp `nxActorCreateShape`: reentry 0x150, empty-root install, group append, promotion with scene remove/add, group+8, 001041 arrays) and the handle [shape+0x9c] read before the unlock; the callees and their residuals are in `## Task 4`. G1 0x1ab and E1 0x1ac from Task 2 |
| 000072 | 0x00002b40 | 90 | reconstructed (Task 7) | slot 14 releaseShape | NpActor.cpp:1181 `releaseShape` | implemented | faithful | 1: ActorShapeMutation 1 | Task 4: 000024(body, [NxShape+8]) (Scene.cpp `nxActorReleaseShape`): reentry 0x186, E1 0x18f/0x19c/0x19d/0x1a4, 001028 swap-remove with the +0xdc/+0x10c writes, the emptied group and the single root released through 000006 and their deleting destructors. G1 0x1b3 from Task 2 |
| 000074 | 0x00002ba0 | 85 | reconstructed | slot 75 raiseActorFlag | NpActor.cpp:2453 `raiseActorFlag`; OM ObjectModel.cpp:1107 | implemented | faithful | 1: ActorMetadata 1 | NA faithful (G1 0x1bb reproduced by Task 2); OM faithful |
| 000076 | 0x00002c00 | 87 | reconstructed | slot 76 clearActorFlag | NpActor.cpp:2463 `clearActorFlag`; OM ObjectModel.cpp:1113 | implemented | faithful | 1: ActorMetadata 1 | NA faithful (G1 0x1c1 reproduced by Task 2); OM faithful |
| 000078 | 0x00002c60 | 39 | reconstructed | slot 77 readActorFlag | NpActor.cpp:2473 `readActorFlag`; OM ObjectModel.cpp:826 | implemented | faithful | 6: ActorMetadata 6 | NA and OM faithful |
| 000080 | 0x00002c90 | 103 | reconstructed | slot 80 readBodyFlag | NpActor.cpp:2517 `readBodyFlag`; OM ObjectModel.cpp:3773 | implemented | faithful | 6: ActorBodyFlag 6 | NA: E1 0x1e3 reproduced by Task 2. OM faithful |
| 000082 | 0x00002d00 | 36 | reconstructed | slot 15 getNbShapes | NpActor.cpp:1192 `getNbShapes`; OM ObjectModel.cpp:912 | implemented | faithful | 6: ActorLifecycle 3, ActorShapeMutation 3 | NA: Task 3 added the read lock (NG). OM faithful |
| 000084 | 0x00002d30 | 36 | reconstructed | slot 16 getShapes | NpActor.cpp:1207 `getShapes`; OM ObjectModel.cpp:926 | implemented | faithful | 14: ActorDynamicSetter 5, ActorLifecycle 5, ActorName 1, ActorShapeMutation 3 | NA: Task 3 added the read lock (NG). OM faithful |
| 000086 | 0x00002d60 | 40 | reconstructed | slot 84 getName | NpActor.cpp:2580 `getName`; OM ObjectModel.cpp:939 | implemented | defect (X) | 5: ActorName 5 | NA: Task 3 added the read lock (NG). Open: reads gNxShapeNames, not 000454 binding table (candidate gPointerBindings). OM faithful |
| 000088 | 0x00002d90 | 91 | reconstructed | slot 83 setName | NpActor.cpp:2573 `setName`; OM ObjectModel.cpp:1221 | implemented | defect (X) | 3: ActorName 3 | NA: Task 2: write-try, G1 0x1ff and unlock added. Open: writes gNxShapeNames not 000480's table. OM faithful |
| 000090 | 0x00002df0 | 211 | reconstructed (Task 7) | slot 11 moveGlobalPosition | NpActor.cpp:1147 `moveGlobalPosition` | implemented | faithful | 3: ActorDynamics 3 | Task 3: the target is the input plus the unrotated local mass position +0x100 (each sum rounded), written by 000784 (ORs 1, no null test) with its wake; G1 0x29e and E1 0x2a1 from Task 2 |
| 000092 | 0x00002ed0 | 90 | reconstructed | slot 6 getGlobalPositionVal | NpActor.cpp:955 `getGlobalPositionVal`; OM ObjectModel.cpp:1240 | implemented | faithful | 22: ActorCMass 6, ActorDynamicSetter 8, ActorDynamics 3, ActorLifecycle 5 | NA: Task 3 added the read lock (NG) and dropped the body null test the oracle lacks. OM faithful |
| 000094 | 0x00002f30 | 517 | reconstructed | slot 8 getGlobalOrientationQuatVal | NpActor.cpp:1082 `getGlobalOrientationQuatVal`; OM ObjectModel.cpp:3203 | implemented | faithful | 3: ActorLifecycle 3 | NA: Task 3 added the read lock (NG); the static arm uses the setter conversion (0x2f77-0x3109, (m11 + m22) spilled, z arm over float(s), x/y arms with the reciprocal spilled). OM: no guard, trace/case association, spills and NaN arm differ (the OM form, not the NA path) |
| 000096 | 0x00003140 | 179 | reconstructed | slot 29 getCMassLocalPoseVal | NpActor.cpp:1613 `getCMassLocalPoseVal`; OM ObjectModel.cpp:3455 | implemented | faithful | 21: ActorCMass 21 | NA: E1 0x2f2 reproduced by Task 2. OM faithful |
| 000098 | 0x00003200 | 151 | reconstructed | slot 30 getCMassLocalPositionVal | NpActor.cpp:1624 `getCMassLocalPositionVal`; OM ObjectModel.cpp:3559 | implemented | faithful | 21: ActorCMass 21 | NA: E1 0x2fa reproduced by Task 2. OM faithful |
| 000100 | 0x000032a0 | 108 | reconstructed | slot 31 getCMassLocalOrientationVal | NpActor.cpp:1634 `getCMassLocalOrientationVal`; OM ObjectModel.cpp:3483 | implemented | faithful | 21: ActorCMass 21 | NA: E1 0x301 reproduced by Task 2. OM faithful |
| 000102 | 0x00003310 | 151 | reconstructed | slot 38 getMassSpaceInertiaTensorVal | NpActor.cpp:1729 `getMassSpaceInertiaTensorVal`; OM ObjectModel.cpp:3566 | implemented | faithful | 0 (breakpointed, not hit) | NA: E1 0x328 reproduced by Task 2. OM faithful (its comment says slot 44; the slot is 38) |
| 000104 | 0x000033b0 | 137 | reconstructed | slot 47 getLinearVelocityVal | NpActor.cpp:1839 `getLinearVelocityVal`; OM ObjectModel.cpp:3574 | implemented | faithful | 3: ActorDynamicSetter 1, ActorDynamics 1, ActorMomentum 1 | NA: E1 0x343 reproduced by Task 2. OM faithful |
| 000106 | 0x00003440 | 140 | reconstructed | slot 48 getAngularVelocityVal | NpActor.cpp:1851 `getAngularVelocityVal`; OM ObjectModel.cpp:3582 | implemented | faithful | 6: ActorDynamicSetter 1, ActorDynamics 1, ActorMomentum 4 | NA: E1 0x34a reproduced by Task 2. OM faithful |
| 000108 | 0x000034d0 | 173 | reconstructed | slot 52 getLinearMomentumVal | NpActor.cpp:1918 `getLinearMomentumVal`; OM ObjectModel.cpp:3590 | implemented | faithful | 2: ActorMomentum 2 | NA: E1 0x353 reproduced by Task 2. OM faithful |
| 000110 | 0x00003580 | 35 | reconstructed | slot 19 isDynamic | NpActor.cpp:685 `isDynamic`; OM ObjectModel.cpp:802 | implemented | faithful | 2: ActorLifecycle 2 | NA: Task 3 added the read lock (NG); no body null test, as the oracle. OM faithful |
| 000112 | 0x000035b0 | 82 | reconstructed | slot 85 setGroup | NpActor.cpp:2588 `setGroup`; OM ObjectModel.cpp:3628 | implemented | faithful | 1: ActorMetadata 1 | NA faithful (G1 0x3cd reproduced by Task 2); OM faithful |
| 000114 | 0x00003610 | 34 | reconstructed | slot 86 getGroup | NpActor.cpp:2597 `getGroup`; OM ObjectModel.cpp:814 | implemented | faithful | 2: ActorMetadata 2 | NA and OM faithful |
| 000116 | 0x00003640 | 8 | reconstructed | table word 87 (0x1010468c): the member (actor+8) table's this-adjust thunk to slot 0 | none (candidate actor has no +8 member table) | OM only | defect (M) | not breakpointed (no candidate function) | NA: no member table or thunk. OM (ObjectModel) faithful, inherits 000118's allocator defect |
| 000118 | 0x00003650 | 55 | reconstructed | slot 0 ~NxActor (scalar deleting dtor) | none; the wrapper is freed in `NxSceneInternal::releaseActor` Scene.cpp:1332 (free at :1369); the compiler-generated `??_GNpActorVtable` (slot 0 of the candidate table) is the NA counterpart, which no path calls | OM only | defect (M) | 47: ActorBodyFlag 1, ActorCMass 11, ActorDynamicSetter 8, ActorDynamics 2, ActorForce 11, ActorLifecycle 6, ActorMetadata 1, ActorMomentum 4, ActorName 1, ActorShapeMutation 1, DynamicFirst 1 | NA: no destructor; Scene.cpp:1369 frees the wrapper without the table stores/002406. OM: frees through nxGetSdkAllocator, not the imported nxFoundationSDKAllocator ([0x101041bc]); returns void |
| 000120 | 0x00003690 | 429 | reconstructed | slot 82 saveToDesc | NpActor.cpp:2543 `saveToDesc` | implemented | faithful | 3: ActorLifecycle 3 | NA faithful (G1 0x22 reproduced by Task 2) |
| 000122 | 0x00003840 | 761 | reconstructed (Task 7) | slot 18 setDynamic | NpActor.cpp:1579 `setDynamic` | implemented | faithful | Task 6 trace: 14: ActorShapeMutation 14 | Task 5: G1 0x5b; E1 0x63 (mass < 0, then the twelve massLocalPose words through _fpclass & 0x207), 0x66 (no shape and a zero-bit tensor); 000533 on a static body's shape; 000026 (Scene.cpp `nxActorBuildRecord`) with E1 0x7c/0x7d leaving the shape out; an old record: 000632, notifyObservers(0x100), 000776, free; 000531 with true. See `## Task 5` |
| 000124 | 0x00003b40 | 1075 | reconstructed (Task 7) | slot 10 moveGlobalPose | NpActor.cpp:1128 `moveGlobalPose` | implemented | faithful | 5: ActorDynamics 5 | Task 3: the CMass-frame composition from the listing (row 0 of M p kept in the register, rows 1-2 spilled; G = M F in the listing orders), the 000801 conversion of G, 000784 (ORs 1/2 into +0xc, no null test) and its wake |
| 000126 | 0x00003f80 | 1192 | reconstructed (Task 7) | slot 12 moveGlobalOrientation | NpActor.cpp:1164 `moveGlobalOrientation` | implemented | faithful | 3: ActorDynamics 3 | Task 3: composes inline under its own lock with its own orders (translation +0x50; M p row 0 in the register, rows 1-2 spilled; G = M F row by row), the 000801 conversion, 000784 and its wake; no longer delegates |
| 000128 | 0x00004430 | 333 | reconstructed (Task 7) | slot 9 getGlobalPoseReference | NpActor.cpp:1100 `getGlobalPoseReference` | implemented | faithful | 2: ActorCMass 2 | all blocks incl. the one-shot 0xd0 warning (line 0x2c0) and the x87 quat-to-rows sequence; Task 2 made the report getInstance().error (the inline `cmp [instance],0; int3`); the Task 3 review moved position x/y through fld/fstp (SNaN quieted) and z as a dword, as 0x10004553-0x10004568 |
| 000130 | 0x00004580 | 318 | reconstructed | slot 5 getGlobalPoseVal | NpActor.cpp:944 `getGlobalPoseVal`; OM ObjectModel.cpp:1359 | implemented | faithful | 16: ActorCMass 6, ActorDynamicSetter 6, ActorLifecycle 4 | NA: Task 3: one read lock around both inline sub-reads, as 0x4580-0x46bb. OM: no guard, double quat-to-matrix without the spills (the OM form) |
| 000132 | 0x000046c0 | 259 | reconstructed | slot 7 getGlobalOrientationVal | NpActor.cpp:1058 `getGlobalOrientationVal`; OM ObjectModel.cpp:3276 | implemented | faithful | 20: ActorCMass 6, ActorDynamicSetter 7, ActorLifecycle 7 | NA: Task 3 added the read lock (NG); x87 helper exact. OM: no guard, no spills (the OM form) |
| 000134 | 0x000047d0 | 907 | reconstructed (Task 7) | slot 32 getCMassGlobalPoseVal | NpActor.cpp:1644 `getCMassGlobalPoseVal` | implemented | faithful | 21: ActorCMass 21 | Task 3: W = R F in the 134 order ((a1f4 + a2f7) + a0f1, (a0f2 + a1f5) + a2f8 in columns 1 and 2; nxNpActorWorldMassRotation NX_RF_134); ROT and the position were already faithful; E1 0x30a from Task 2 |
| 000136 | 0x00004b60 | 498 | reconstructed (Task 7) | slot 33 getCMassGlobalPositionVal | NpActor.cpp:1657 `getCMassGlobalPositionVal` | implemented | faithful | 21: ActorCMass 21 | E1 0x314 reproduced by Task 2; ROT and position arithmetic faithful |
| 000138 | 0x00004d60 | 658 | reconstructed (Task 7) | slot 34 getCMassGlobalOrientationVal | NpActor.cpp:1669 `getCMassGlobalOrientationVal` | implemented | faithful | 21: ActorCMass 21 | E1 0x31d reproduced by Task 2; ROT and RF faithful (nxNpActorDerivedMassFrame) |
| 000140 | 0x00005000 | 706 | reconstructed (Task 7) | slot 39 getGlobalInertiaTensorVal | NpActor.cpp:1741 `getGlobalInertiaTensorVal` | implemented | faithful | 5: ActorMomentum 5 | Task 3: ROT, W = R F in the 140 order, then the 000746 model (nxNpActorWorldTensorRDRt) of diag(+0x18c); E1 0x32f from Task 2 |
| 000142 | 0x000052d0 | 691 | reconstructed (Task 7) | slot 40 getGlobalInertiaTensorInverseVal | NpActor.cpp:1751 `getGlobalInertiaTensorInverseVal` | implemented | faithful | 5: ActorMomentum 5 | Task 3: ROT, W in the 134 order, the 000746 model of diag(+0xc4); no lock (Task 2), E1 0x338 |
| 000144 | 0x00005590 | 849 | reconstructed (Task 7) | slot 53 getAngularMomentumVal | NpActor.cpp:1935 `getAngularMomentumVal` | implemented | faithful | 6: ActorMomentum 6 | Task 3: ROT, W in the 144 order (row 0 column 2 as in 138), T = 000746 of diag(+0x18c), L rows (t1 wy + t2 wz) + t0 wx in x87, rounded once; E1 0x35a from Task 2 |
| 000146 | 0x000058f0 | 579 | reconstructed | slot 65 getPointVelocityVal | NpActor.cpp:728 `getPointVelocityVal` | implemented | faithful | 20: ActorLifecycle 20 | NA faithful |
| 000148 | 0x00005b40 | 811 | reconstructed | slot 66 getLocalPointVelocityVal | NpActor.cpp:2186 `getLocalPointVelocityVal` | implemented | faithful | 20: ActorLifecycle 20 | NA faithful |
| 000150 | 0x00005e70 | 298 | reconstructed (Task 7) | helper: rotate a local vector by the body orientation (thiscall on the record; callers 000156/000158/000160/000162) | NpActor.cpp:1959 `nxNpActorRotateLocalForce` | implemented | faithful | 7: ActorForce 7 | Task 2: rewritten from the listing: ROT via nxNpActorRotationFromQuaternionGetter, rows (m1y+m2z)+m0x in x87 registers (double lifetimes, NpActor.cpp now /arch:IA32), one rounding at the store |
| 000152 | 0x00005fa0 | 346 | reconstructed (Task 7) | helper: local point to world (callers 000154/000158) | NpActor.cpp:1975 `nxNpActorLocalPosition` | implemented | faithful | 4: ActorForce 4 | Task 2: rewritten from the listing: ROT, x = (R01y+R02z)+R00x kept in the register and added to t.x; y = (R11y+R12z)+R10x and z = (R22z+R20x)+R21y spilled to float before t.y/t.z are added |
| 000154 | 0x00006100 | 201 | reconstructed (Task 7) | slot 55 addForceAtLocalPos | NpActor.cpp:2003 `addForceAtLocalPos` | implemented | faithful | 2: ActorForce 2 | Task 3: 000791/000782 from the listing; G1 0x131, E1 0x132 and the 000152 helper from Task 2 |
| 000156 | 0x000061d0 | 201 | reconstructed (Task 7) | slot 56 addLocalForceAtPos | NpActor.cpp:2014 `addLocalForceAtPos` | implemented | faithful | 1: ActorForce 1 | Task 3: 000791/000782 from the listing; G1 0x13b, E1 0x13c and the 000150 helper from Task 2 |
| 000158 | 0x000062a0 | 214 | reconstructed (Task 7) | slot 57 addLocalForceAtLocalPos | NpActor.cpp:2025 `addLocalForceAtLocalPos` | implemented | faithful | 2: ActorForce 2 | Task 3: 000791/000782 from the listing; G1 0x144, E1 0x145 and both helpers from Task 2 |
| 000160 | 0x00006380 | 198 | reconstructed (Task 7) | slot 59 addLocalForce | NpActor.cpp:2124 `addLocalForce` | implemented | faithful | 2: ActorForce 2 | Task 3: 000782 from the listing; G1 0x156, E1 0x157 and the 000150 helper from Task 2 |
| 000162 | 0x00006450 | 198 | reconstructed (Task 7) | slot 61 addLocalTorque | NpActor.cpp:2145 `addLocalTorque` | implemented | faithful | 2: ActorForce 2 | Task 3: 000782 from the listing; G1 0x169, E1 0x16a and the 000150 helper from Task 2 |
| 000164 | 0x00006520 | 1846 | reconstructed (Task 7) | slot 17 updateMassFromShapes | NpActor.cpp:1468 `updateMassFromShapes` | implemented | faithful | Task 6 trace: 48: ActorShapeMutation 48 | Task 5: G1 0x98; E1 0x9a (ordered below zero or unordered), 0x9d, 0x9e, 0x9f, 0xa0, 0xa8/0xa9 from 000008 (Scene.cpp `nxActorComputeMassFromShapes`); +0x188, +0xc0 (1/m, no test), mark 0x10000; diagonal and _fpclass-zeroed inverses, mark 0x20000; +0x100, mark 0x200, ++0x198; +0xdc, mark 0x400, ++0x198; 000768. See `## Task 5` |
| 000166 | 0x00006c60 | 479 | reconstructed (Task 7) | slot 35 setMass | NpActor.cpp:1681 `setMass` | implemented | faithful | 2: ActorDynamicSetter 2 | E1 0xba and 0xbb ("Body::setMass: mass is %f, should be positive!", the mass passed as a double) reproduced by Task 2; record effects faithful |
| 000168 | 0x00006e40 | 577 | reconstructed (Task 7) | slot 37 setMassSpaceInertiaTensor | NpActor.cpp:1707 `setMassSpaceInertiaTensor` | implemented | faithful | 3: ActorDynamicSetter 2, ActorMomentum 1 | Task 3: the three float inverses 1/m are classified by the CRT _fpclass the oracle calls (005666); any NaN or infinity (0x207) zeroes all three, so negatives keep their inverse and zero/denormal inertias zero them; G1 0xc5, E1 0xc6 and H1 from Task 2 |
| 000170 | 0x00007090 | 431 | reconstructed (Task 7) | slot 41 setLinearDamping | NpActor.cpp:1761 `setLinearDamping` | implemented | faithful | 2: ActorDynamicSetter 2 | E1 0xd1 (value, checked before dynamic) and 0xd2 reproduced by Task 2; record effects faithful |
| 000172 | 0x00007240 | 431 | reconstructed (Task 7) | slot 43 setAngularDamping | NpActor.cpp:1786 `setAngularDamping` | implemented | faithful | 2: ActorDynamicSetter 2 | E1 0xe0 and 0xe1 reproduced by Task 2; record effects faithful |
| 000174 | 0x000073f0 | 770 | reconstructed (Task 7) | slot 45 setLinearVelocity | NpActor.cpp:1811 `setLinearVelocity` | implemented | faithful | 3: ActorDynamicSetter 3 | Task 3: the wake after the stores and the 4 mark: the input (y y + z z) + x x in the register against +0xd0 (below or unordered skips), then +0x114 & 0x100, the ordered < 0.39999998f and the 0x10 mark; G1 0xf3, E1 0xf4 and H1 from Task 2 |
| 000176 | 0x00007700 | 786 | reconstructed (Task 7) | slot 46 setAngularVelocity | NpActor.cpp:1825 `setAngularVelocity` | implemented | faithful | 3: ActorDynamicSetter 3 | Task 3: the wake as 000174 against +0xd4; G1 0xfc, E1 0xfd and H1 from Task 2 |
| 000178 | 0x00007a20 | 385 | reconstructed (Task 7) | slot 49 setMaxAngularVelocity | NpActor.cpp:1863 `setMaxAngularVelocity` | implemented | faithful | 2: ActorMomentum 2 | E1 0x109 and H1 fixed by Task 2 |
| 000180 | 0x00007bb0 | 782 | reconstructed (Task 7) | slot 50 setLinearMomentum | NpActor.cpp:1876 `setLinearMomentum` | implemented | faithful | 3: ActorMomentum 3 | Task 3: the wake over the stored velocity (x x + y y) + z z against +0xd0; G1 0x113, E1 0x114 and H1 from Task 2 |
| 000182 | 0x00007ec0 | 867 | reconstructed (Task 7) | slot 51 setAngularMomentum | NpActor.cpp:1893 `setAngularMomentum` | implemented | faithful | 6: ActorMomentum 6 | Task 3: rows (I1 y + I2 z) + I0 x, (I4 y + I3 x) + I5 z, (I7 y + I6 x) + I8 z rounded once each, then the wake over the stored angular velocity (x x + y y) + z z against +0xd4; G1 0x11c, E1 0x11d and H1 from Task 2 |
| 000184 | 0x00008230 | 333 | reconstructed (Task 7) | slot 70 setSleepLinearVelocity | NpActor.cpp:2384 `setSleepLinearVelocity` | implemented | faithful | 3: ActorDynamicSetter 3 | H1 fixed by Task 2; body faithful (G1 0x195 reproduced) |
| 000186 | 0x00008380 | 333 | reconstructed (Task 7) | slot 72 setSleepAngularVelocity | NpActor.cpp:2409 `setSleepAngularVelocity` | implemented | faithful | 2: ActorDynamicSetter 2 | H1 fixed by Task 2; body faithful (G1 0x1a2 reproduced) |
| 000188 | 0x000084d0 | 407 | reconstructed (Task 7) | slot 78 raiseBodyFlag | NpActor.cpp:2485 `raiseBodyFlag` | implemented | faithful | 9: ActorBodyFlag 2, ActorDynamicSetter 1, ActorDynamics 4, ActorForce 1, ActorMomentum 1 | Task 3: 000785 enable arm now begins with the island-root refresh (000712 on +0x1bc with path compression; [root+0x1e4] |= 2 when [root+0x1e0]); G1 0x1cf, E1 0x1d0, H1 and the marks/allocator from Task 2 |
| 000190 | 0x00008670 | 415 | reconstructed (Task 7) | slot 79 clearBodyFlag | NpActor.cpp:2501 `clearBodyFlag` | implemented | faithful | 8: ActorBodyFlag 1, ActorDynamicSetter 1, ActorDynamics 4, ActorForce 1, ActorMomentum 1 | Task 3: 000785 disable arm now begins with the same island-root refresh; G1 0x1d9, E1 0x1da, H1, the marks/allocator and the unconditional 1/m from Task 2 |
| 000192 | 0x00008810 | 371 | reconstructed (Task 7) | slot 73 wakeUp | NpActor.cpp:2422 `wakeUp` | implemented | faithful | 7: ActorCMass 1, ActorDynamicSetter 5, ActorForce 1 | H1 fixed by Task 2; body faithful (G1 0x207 reproduced) |
| 000194 | 0x00008990 | 354 | reconstructed (Task 7) | slot 74 putToSleep | NpActor.cpp:2437 `putToSleep` | implemented | faithful | 5: ActorDynamicSetter 4, ActorForce 1 | H1 fixed by Task 2; body faithful (G1 0x211 reproduced) |
| 000196 | 0x00008b00 | 1114 | reconstructed (Task 7) | slot 1 setGlobalPose | NpActor.cpp:695 `setGlobalPose` | implemented | faithful | 10: ActorCMass 7, ActorDynamicSetter 3 | Task 2: G1 0x21d and H1 fixed; conversion, stores and 000768 faithful. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000198 | 0x00008f60 | 418 | reconstructed (Task 7) | slot 2 setGlobalPosition | NpActor.cpp:862 `setGlobalPosition` | implemented | faithful | 12: ActorCMass 7, ActorDynamicSetter 5 | Task 2: G1 0x232 and H1 fixed; both arms and the refresh faithful. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000200 | 0x00009110 | 821 | reconstructed (Task 7) | slot 3 setGlobalOrientation | NpActor.cpp:887 `setGlobalOrientation` | implemented | faithful | 12: ActorCMass 7, ActorDynamicSetter 5 | Task 2: G1 0x242 and H1 fixed; setter conversion faithful. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000202 | 0x00009450 | 611 | reconstructed (Task 7) | slot 4 setGlobalOrientationQuat | NpActor.cpp:915 `setGlobalOrientationQuat` | implemented | faithful | 9: ActorCMass 7, ActorDynamicSetter 2 | Task 2: G1 0x254 and H1 fixed; static quat-to-matrix x87 sequence faithful. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000204 | 0x000096c0 | 520 | reconstructed (Task 7) | slot 26 setCMassGlobalPose | NpActor.cpp:1419 `setCMassGlobalPose` | implemented | faithful | 1: ActorCMass 1 | Task 3: 000756 = nxNpActorBodyQuaternionFromMatrix (x87 roots), 000789 rewritten from the listing (000746 tensor first, displacement row 0 unrounded and rows 1-2 spilled, setter conversion), 000004 shape update added (virtual slot 6; the group arm models 001018). Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000206 | 0x000098d0 | 504 | reconstructed (Task 7) | slot 27 setCMassGlobalPosition | NpActor.cpp:1574 `setCMassGlobalPosition` | implemented | faithful | 1: ActorCMass 1 | Task 3: 000789 rewritten from the listing and the 000004 shape update added. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000208 | 0x00009ad0 | 492 | reconstructed (Task 7) | slot 28 setCMassGlobalOrientation | NpActor.cpp:1586 `setCMassGlobalOrientation` | implemented | faithful | 1: ActorCMass 1 | Task 3: 000756, 000789 and the 000004 shape update fixed as in 000204. Task 4: S1 closed (the group has its table; 000004 reaches 001018 and the group-level 001315) |
| 000210 | 0x00009cc0 | 998 | reconstructed (Task 7) | slot 20 setCMassOffsetLocalPose | NpActor.cpp:1254 `setCMassOffsetLocalPose` | implemented | faithful | 8: ActorCMass 8 | E1 0x388, G1 0x387 and H1 fixed by Task 2; store/dirty/++0x198/000768/wake order faithful |
| 000212 | 0x0000a0b0 | 739 | reconstructed (Task 7) | slot 21 setCMassOffsetLocalPosition | NpActor.cpp:1273 `setCMassOffsetLocalPosition` | implemented | faithful | 9: ActorCMass 9 | E1 0x394, G1 0x393 and H1 fixed by Task 2 |
| 000214 | 0x0000a3a0 | 557 | reconstructed (Task 7) | slot 22 setCMassOffsetLocalOrientation | NpActor.cpp:1289 `setCMassOffsetLocalOrientation` | implemented | faithful | 8: ActorCMass 8 | E1 0x39f (the report sits in 000216's range), G1 0x39e and H1 fixed by Task 2 |
| 000216 | 0x0000a5d0 | 163 | reconstructed (Task 7) | tail of 000214 (wake dirty-list growth loop, epilogue, error tail; entered by jumps from 000214) | inside NpActor.cpp:1289 `setCMassOffsetLocalOrientation` | implemented | faithful | not breakpointed (no candidate function); Task 6: 000214's 13 hits run its epilogue | not a function: 000214's tail, covered by setCMassOffsetLocalOrientation including E1 0x39f and H1 (Task 2) |
| 000218 | 0x0000a680 | 1614 | reconstructed (Task 7) | slot 23 setCMassOffsetGlobalPose | NpActor.cpp:1374 `setCMassOffsetGlobalPose` | implemented | faithful | 8: ActorCMass 8 | E1 0x3ac, G1 0x3ab and H1 fixed by Task 2; rotation, 3 position and 9 orientation sums faithful |
| 000220 | 0x0000acd0 | 1059 | reconstructed (Task 7) | slot 24 setCMassOffsetGlobalPosition | NpActor.cpp:1389 `setCMassOffsetGlobalPosition` | implemented | faithful | 8: ActorCMass 8 | E1 0x3b8, G1 0x3b7 and H1 fixed by Task 2; rotation and 3 sums faithful |
| 000222 | 0x0000b100 | 1187 | reconstructed (Task 7) | slot 25 setCMassOffsetGlobalOrientation | NpActor.cpp:1403 `setCMassOffsetGlobalOrientation` | implemented | faithful | 8: ActorCMass 8 | E1 0x3c1, G1 0x3c0 and H1 fixed by Task 2; rotation and 9 sums faithful |

## Counts

These are the Task 1 counts (before Task 2); the counts after Task 2 are at the end of
`## Task 2: the cross-cutting pass`, and after Task 3 in `## Task 3: row-level defects`.

The 53 `discovered` rows (30,673 B):

- Status: 37 implemented, 14 partial, 2 missing (000122 setDynamic, 000164
  updateMassFromShapes).
- Verdict: 1 faithful (000128 getGlobalPoseReference), 52 defect. Of the 52: 4 have only H1
  (000184 000186 000192 000194); 13 have only E1 (+H1) (000136 000138 000166 000170 000172
  000178 000210 000212 000214 000216 000218 000220 000222); 4 have S1 (+H1) and otherwise
  faithful arithmetic (000196 000198 000200 000202); 29 have substantive differences; 2 are
  missing.
- Traced: 50 hit by at least one Phase 5 target; 3 not: 000122 and 000164 (breakpointed on
  their empty candidate bodies and not hit: no case calls setDynamic or updateMassFromShapes)
  and 000216 (no candidate function of its own; 000214's candidate was hit 8 times).

The inventory's "partial" notes on 000196-000202 ("pending lock-error, scene and final
shape-family closure") reduce to G1, S1 and H1: the listings contain no scene or broadphase
call besides 000004, and the conversions, stores and the single 000768 refresh are faithful.
They are counted as implemented.

The 34 `reconstructed` rows: the NA forms are 32 implemented, 2 without a public-path form
(000116, 000118: the candidate never deletes an actor through its vtable); 12 faithful
(000062-000068, 000074-000078, 000112, 000114, 000120, 000146, 000148), 22 defect. Untraced:
000102 (getMassSpaceInertiaTensorVal: breakpointed, no case calls it) and 000116 (not
breakpointed).

## Task 2: the cross-cutting pass

Task 2 fixed G1, E1, H1 and the SSE build across the unit; the Rows table above carries the
new verdicts and says in each summary what Task 2 fixed and what is still open.

- **G1**: every write-guarded row goes through `nxNpActorWriteTry(ctx, line)` (NpActor.cpp),
  which reports kind 2, the row's line and the 0x10104760 message and returns without
  unlocking: 46 rows, lines 0x22-0x3cd, each read from its listing. 000088 setName now takes
  the write lock at all (G1 0x1ff). 000122/000164 (empty bodies) are left to their own tasks.
- **E1**: 51 reports (`nxNpActorReport(line, message)`, kind 1; 000166's 0xbb is the one
  formatted report, the mass passed as a double) in the order each listing has them: the
  readers report inside their read lock before the default result and the unlock; the
  writers report and then unlock; 000170/000172 test the value before the actor; 000204-000208
  test the actor before the write lock and never lock on that arm; 000142 takes no lock at all
  (the candidate's read guard was removed). 000070 asks `desc.isValid()` (0x1ac); 000126 now
  takes its own lock (0x2ac) and checks kinematic (0x2ae) before it delegates.
- **int3**: every report is `FoundationSDK::getInstance().error(...)`, the joint units' form;
  the variadic error stays the Foundation import and `getInstance()` inlines, so the candidate
  DLL has the oracle's `mov eax,[__imp_instance]; cmp [eax],0; jne; int3` before each
  `call [__imp_error]` (checked by disassembling the built nxNpActorWriteTry). 000128 was
  switched to the same form.
- Every message and the file string were checked byte for byte against the PE (all 51 found
  NUL-terminated in the image; Task 3 recounted them, 51 not 52).
- **H1**: `nxNpActorMarkRecordDirty` is the listing's inline NxArray pushBack (000184
  0x10008285-0x1000836c; 000170, 000785 and the other rows differ only in register allocation
  and store order, so one helper covers all of them): no null or id tests, the index written
  before the growth, growth when `capacity <= end` to 2n+2 unless the current capacity (0 for a
  null list) covers it, `nxFoundationSDKAllocator` malloc(size, 0)/free, the old block freed
  only when non-null, no `if(!grown)`. `nxNpActorTransitionKinematic` (000785/000787) marks
  0x10000, 0x20000 and 0x80000 in the listing's order before the 0x20-byte block is
  allocated (enable) or freed (disable) through `nxFoundationSDKAllocator` with no null check,
  and the disable arm divides unconditionally (1.0/m). The 000712 root refresh is still open.
- **SSE**: `Physics/src/NpActor.cpp` is on the NxPhysics `/arch:IA32` list (the second list in
  `CMakeLists.txt`; the first is the Foundation's own, for Utilities.cpp, and source-file
  properties are per directory, so the Internal and Collision test targets that also
  compile NpActor.cpp get it too). The cross-cutting helpers 000150 and 000152 were rewritten
  from their listings; the row-specific orders (000134, 000140-000144, 000182, 000060/000742,
  000204-000208, 000746, 000782/000791) are Task 3's.
- Tests: NxPhysicsActorDynamicSetterTests passes a Foundation error stream that stays silent
  until its new cases, then prints every report (231 new oracle-side lines: static,
  kinematic and invalid-argument E1s, and G1 on every guarded row with the write lock held by
  owner 0); NxPhysicsActorBodyFlagTests grows a null dirty list through 2, 6 and 14 entries
  and checks 000785's allocation order (12 lines); NxPhysicsActorForceTests drives 000150 with
  irregular rotations (8 lines). All 251 match the oracle; against the Task 1 candidate the
  setter target differs in 379 lines, the body-flag target crashes on the null list, and
  x87_rotate_5 differs in the last bit.

Counts after Task 2. The 53 `discovered` rows: 20 faithful (000128, 000136, 000138, 000150,
000152, 000166, 000170, 000172, 000178, 000184, 000186, 000192, 000194, 000210-000222), 4 S1
(000196-000202), 27 substantive, 2 missing. The 34 `reconstructed` rows: 22 faithful (the 12
above plus 000050, 000052, 000080, 000096-000108), 12 defect (000060, 000082-000088, 000092,
000094, 000110, 000116, 000118, 000130, 000132). 000102's E1 arm is now driven by the setter
target.

## Task 3: row-level defects

Task 3 fixed the row-level defects left after Task 2, in six commits (d6a84ba, cf5b3c0, 123b88c,
ebfd183, 725cfd2, 7666f0c). The Rows table above carries the new verdicts. Every change was
checked against the Capstone listing (a symbolic x87 walk of each arithmetic block), driven by
new staged-pair cases, and falsified against the previous commit's candidate.

- **CMass-global setters (000204-000208).**
  - 000756 (0x17420) is the 000801 conversion, so `nxNpActorUpdateCMassQuaternion` now calls
    `nxNpActorBodyQuaternionFromMatrix`.
  - 000789 (0x19d00) is rewritten from the listing: the 000746 tensor comes first, then the nine
    x87 dot products. The displacement keeps row 0, (A2 p2 + A1 p1) + A0 p0, in the register and
    spills rows 1 and 2. The actor rotation is converted with the setter sequence.
  - The rows end with the 000004 shape update: a virtual slot-6 dispatch on [body+0x10]. The
    group arm models 001018: each child's slot 6, over (end - begin) / 4 entries.
  - The group-level 001315 call is still open, because the candidate group is a 0x110-byte stub
    with no table and no ShapeBase layout.
  - The square roots of both quaternion conversions are now X87Sqrt.h forms, with the listing's
    operand order.
- **Kinematic moves (000090, 000124, 000126).**
  - 000784 (0x194b0) writes the target block with no null test, ORs 1/2 into +0xc, and runs the
    wake.
  - 000124 composes pose * {+0xdc, +0x100}: row 0 of M p stays in the register and rows 1-2 are
    spilled; G = M F uses the listing's orders; G is converted with the 000801 sequence.
  - 000126 does the same inline under its own lock, with its own operand orders and the current
    position +0x50.
  - 000090 adds the unrotated +0x100.
- **Wake blocks (000174, 000176, 000180, 000182).**
  - The squared speed is compared against the sleep threshold (+0xd0 or +0xd4): the input's
    (y y + z z) + x x for the velocity setters, the stored velocity's (x x + y y) + z z for the
    momentum setters.
  - A speed that is below the threshold, or unordered, skips the wake. Otherwise the usual
    +0x114 & 0x100 test, the < 0.39999998f test and the 0x10 mark follow.
  - 000182's rows are (I1 y + I2 z) + I0 x, (I4 y + I3 x) + I5 z and (I7 y + I6 x) + I8 z.
- **000168.** The three float inverses are classified by the CRT `_fpclass` that the oracle calls
  (005666). A NaN or infinity (0x207) in any of them zeroes all three.
- **Summation orders.**
  - 000060/000742: ((m v.v + I2 w2 w2) + I1 w1 w1) + I0 w0 w0, with the linear sum
    (vz vz + vy vy) + vx vx.
  - `nxNpActorWorldMassRotation` holds the four W = R F operand orders:
    - 134, used by 000134 and 000142;
    - 138;
    - 140;
    - 144, which is 140 with row 0's third column from 138.
  - 000140, 000142 and 000144 now use ROT and the 000746 model; 000144's T w rows are x87 dot
    products.
- **The rest.**
  - 000785/000787 begin both arms with the 000712 island-root refresh.
  - NG: 000082, 000084, 000086, 000092, 000094, 000110, 000130 and 000132 take the read lock on
    [actor+0x10]. 000130 does both sub-reads under one lock.
  - 000094's static arm uses the setter conversion. The body null tests the oracle lacks were
    dropped.
  - 000782 (0x18730) is rewritten from the listing:
    - the modes 0/3 linear x product and angular rows 0-1 stay in registers, and so does mode 1's
      angular row 0;
    - the rows are (I2 z + I1 y) + I0 x;
    - there is one wake after both arms, and a mode above 4 still wakes.
  - 000791 keeps r.x in the register and makes one 000782 call.
  - setName lost its body null test.
  - Stable-ID lines were added for 000074, 000076, 000112, 000184, 000186, 000192 and 000194.
  - The E1 count is 51, not 52.
- **Helper rows outside this unit** are marked with `// phys_fn_` lines and the owning unit
  (gap:SceneRaycast.cpp..CapsuleShape.cpp): 000746, 000756, 000782, 000784, 000789 and 000791.
  001018's body is modelled inside `nxNpActorNotifyOwnedShapes` (gap:CapsuleShape.cpp..NpBoxShape.cpp).
  None of them changes state.
- **Tests.** 446 oracle lines, each registered verbatim; floor 5 went from 1122 to 1568. Every
  new case was run against the previous commit's candidate, and each group's cases fail there:
  - CMass: 286 lines, 58 of which fail on the Task 2 candidate;
  - Dynamics: 21 lines (18);
  - Momentum: 93 lines (41 across the two groups), where the near-cancelling momenta, the energy tie and the two RF frames
    were constructed so that the order decides the last bit;
  - DynamicSetter: 23 (19);
  - BodyFlag: 5 (4);
  - Force: 18 (8).
- **Task 3 review** (commits bd53f6e and the follow-up):
  - 001315 (Shape.cpp; `ShapeBase::nxApplyOwnerUpdate`, ObjectModel.cpp) composes the shape's
    world pose from the listing: the owner rotation from record +0x24 by the five-spill
    sequence, each world element in the listing's permuted operand order, the translation's
    row 0 in the register and rows 1-2 spilled before T. The pruner/list arms (0x26a36-0x26ab8)
    are described there; the append to the +0xa0 object's array is not reproduced.
  - The CMass cases print the shape poses as exact words; the 48 rounded registrations were
    replaced by the 48 exact oracle lines, and all of them match.
  - 000128 now moves the position's x and y through fld/fstp (an SNaN comes out quiet), as
    0x10004553-0x10004568 do; a CMass case covers it.
  - A Force case on an unrotated body shows 000782's unrounded mode 0 linear x, mode 0
    angular rows 0-1 and mode 1 angular row 0; all four words differ on the candidate before
    the 000782 rewrite.
  - Floor 5 is 1571 (the three new lines).

Counts after Task 3. Of the 53 `discovered` rows:
- 42 are faithful.
- 7 are S1-residual: 000196-000208, where only the group-level 001315 call is missing.
- 2 are X, the shape add/remove rows 000070 and 000072 (Task 4).
- 2 are M, 000122 and 000164 (Task 5).

Of the 34 `reconstructed` rows:
- 30 are faithful (NA form).
- 2 are X: 000086 and 000088, whose name table is not 000454/000480's.
- 2 are M: 000116 and 000118, with no member table or destructor.

The OM forms of 000094, 000130 and 000132 (ObjectModel.cpp) keep their own defects.

## Task 4: shape add/remove

Task 4 (commits 370aca7 and 2b4bcb1) rebuilt createShape (000070) and releaseShape (000072)
on the Actor.cpp rows they call, and the chain under those, from the Capstone listings. Line
numbers below are of 370aca7.

- **000070** (NpActor.cpp:1387): after Task 2's lock (G1 0x1ab) and `desc.isValid()` (E1
  0x1ac), 000036 on the body [actor+0x14]; a built shape's handle [shape+0x9c] is read before
  the unlock and returned, else 0 (0x2b06-0x2b36).
- **000072** (NpActor.cpp:1406): after the lock (G1 0x1b3), 000024 on the body with the
  internal shape [NxShape+8], then the unlock (0x2b7a-0x2b90).
- **000036** (Scene.cpp:3088 `nxActorCreateShape`): the reentry flag (.data 0x10123c10;
  set: code 2, Actor.cpp line 0x150, the message at 0x10122050), then the factory, then by
  the root: none -> the shape is the root and 000531 adds it; a group -> 001041, the shape's
  slot 6 with 1, 001941, 000503(001957 + 001960), 003628 with a fluid manager; a single ->
  000535/000533 on the old root, a 0x110-byte group through [0x101041bc] constructed by 001033
  with an id from 000012 (taken after the allocation), the root, +8 = [scene+0x540] - 1, 001041
  old then new, 000531 on the group. The new shape (not the group) is returned.
- **000024** (Scene.cpp:3157 `nxActorReleaseShape`): reentry (code 2, 0x186); no root -> E1
  0x1a4; a group holding one child on a static body -> E1 0x18f (before any search); else
  001028, and a group it empties leaves the Scene (000006), is deleted (slot 0 with 1) and the
  root cleared; a single root on a static body -> E1 0x19c; a root that is not the shape ->
  E1 0x19d; the root itself -> 000006, delete, root cleared. All E1s are code 1 with
  `\Epic\Novodex\SDKs\Physics\src\Actor.cpp`.

The callees, each claimed with a `// phys_fn_` line in Scene.cpp, which is the candidate's
Actor.cpp equivalent (it already holds 000034 and 000013) and its runtime shape model:

| Row | RVA | B | Owning unit | Candidate | Notes |
|---|---|---:|---|---|---|
| 000032 | 0x1de0 | 539 | Actor.cpp | `nxActorShapeFactory` | Id first (000012 inlined), the families' sizes (plane 0x10c, sphere 0xe4, box 0x228, capsule 0xec) through [0x101041bc], construct, slot-12 load, the handle's +0x10/+0x14 NpScene lock links, +8 = [scene+0x540] - 1; no shape -> the id back through 000028. Type 4 (triangle mesh, 0xe8, 001379, the Scene+0x10 count and its own 000503) has no runtime family and takes the no-shape arm |
| 000024 | 0x1860 | 328 | Actor.cpp | `nxActorReleaseShape` | above |
| 000036 | 0x2250 | 420 | Actor.cpp | `nxActorCreateShape` | above |
| 000006 | 0x1080 | 30 | gap:<start>..Actor.cpp | `nxActorRemoveRootFromScene` | 000535 with a record, else 000533 |
| 000012 | 0x1430 | 32 | gap:<start>..Actor.cpp | ObjectModel.cpp `nxIdAllocNext` (already reconstructed) | called on Scene+0x6e4 |
| 001033 | 0x22d60 | 99 | gap:CapsuleShape.cpp..NpBoxShape.cpp | `nxShapeGroupConstructAt` | 001273, the group table, arrays emptied, sentinel 5, +0xd8 = 0xffff, +0x10c = -1.0f; also used by the creation path's group (`nxShapeGroupConstruct`) |
| 001041 | 0x22e80 | 442 | gap:CapsuleShape.cpp..NpBoxShape.cpp | `nxShapeGroupAddChild` | both pushes grow a full array to 2n + 2 through [0x101041bc]; child +0xdc bit 0; +0x10c = -1.0f; 001325(0x100) |
| 001028 | 0x22b50 | 152 | gap:CapsuleShape.cpp..NpBoxShape.cpp | `nxShapeGroupRemoveChild` | swap-with-last in both arrays, child +0xdc bit 0 cleared, +0x10c = -1.0f; nothing freed or unregistered |
| 001018 | 0x227d0 | 62 | gap:CapsuleShape.cpp..NpBoxShape.cpp | `nxShapeGroupOwnerUpdate` (group slot 6) | each child's slot 6, then 001315 on the group |
| 001032, 001037, 001039 | 0x22d00, 0x22de0, 0x22e50 | 96, 110, 34 | gap:CapsuleShape.cpp..NpBoxShape.cpp | `nxShapeGroupDeleteChildren`, `nxShapeGroupDeletingDtor` (group slot 0) | children deleted, handle then child array freed, 001323, group freed |
| 001273 | 0x25530 | 424 | gap:NpTriangleMeshShape.cpp..Shape.cpp | `nxRuntimeShapeBaseInit` | the listing's stores on a runtime shape (ShapeBase::ShapeBase is the listing model) |
| 001279 | 0x25760 | 53 | gap:NpTriangleMeshShape.cpp..Shape.cpp | `nxShapeLeavePruning` | 001955, 001945, +0xa0 cleared |
| 001323 | 0x26bd0 | 182 | Shape.cpp | `nxRuntimeShapeBaseDestroy` | name, +0x70c, aux (002413), pairs (002344), id (000028), pruning |
| 000503 | 0x100a0 | 232 | gap:PhysicsSDK.cpp..Scene.cpp | `nxSceneUpdateActorCount` (Scene.cpp:2084) | rewritten: (n + 0x100) & ~0xff, free/alloc order, only +8 zeroed, 004847 on +0x50/+0x500/+0x510 |
| 000531 | 0x10600 | 97 | Scene.cpp | `nxSceneAddShape` | slot 6 with 1, 001943, 000503(001960 + 001957), 003628 |
| 000533, 000535 | 0x10670, 0x106a0 | 40, 40 | Scene.cpp | `nxSceneRemoveStaticShape`, `nxSceneRemoveDynamicShape` | 001279, 003628 with false/true |
| 001941 | 0x4ba80 | 59 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | `nxPruningAddShape` | type (+0x70 or 0), kind 0, insert |
| 001943 | 0x4bac0 | 270 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | `nxPruningAddBody` | root +0xa0, kinds 2 (group) / 1 / 0 (children), the +0x78 list push (SdkContainer, 004840) |
| 001945 | 0x4bbd0 | 74 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | `nxPruningRemoveBody` | root and current children erased |
| 001955 | 0x4bde0 | 153 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | `nxPruningRemoveRootPairs` | +0x78 swap-remove; the pair-record loop (+0x44/+0x48) is empty in the candidate |
| 001957, 001960 | 0x4be80, 0x4bec0 | 16, 20 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | `nxPruningCountFirst`, `nxPruningCountIndexed` | exact (already reconstructed rows) |
| 003628 | 0x89bb0 | 22 | gap:fluids\Fluid.cpp..fluids\FluidManager.cpp | `nxFluidManagerShapeChanged` | exact (already reconstructed row) |

The group is now its own object with a table (slot 0 = 001039, slot 6 = 001018; the other
slots of 0x10106c2c are not reached and stay null), so 000004 in the pose and CMass-global
setters is a plain `nxForwardSubobjectCall` and reaches the group-level 001315: **S1 is
closed** for 000196-000208. The runtime shapes' deleting destructors (the families' slot 0,
e.g. 001375) are `nxRuntimeShapeDelete`: the collision object freed, 001323, the shape freed.

**Pruner model.** The OPCODE pruners (004852, 004857, 004859 and their classes) belong to the
opcode units and stay a model (Scene.cpp:987 onward): per-prunable insert and erase with the
measured allocations and growth points (4 entries, then doubling when an insertion would exceed
the capacity; the fifth prunable of a pruner grows it, as the oracle does for a static and a
dynamic group), the type (+0xce) and kind (+0xcf) bytes of 004888/004890, and +8 = the number of
nonzero-kind prunables (the per-actor roots), which is what 001957/001960 read (the oracle keeps
it at one per actor through promotion and appends). Entry order and handle assignment are the
model's. The creation path's registration (`nxSceneStaticPrunerRegister`,
`nxSceneBroadphaseRegister`) now sets the root's kind and +0xa0 and counts the root, and grows
the static pruner; actor release erases the root and the current children by search.

**Not reproduced** (recorded):
- the reentry flags themselves cannot be set from the harness (no callback runs), so 0x150 and
  0x186 are static-only;
- 000032's mesh arm (no runtime triangle-mesh family);
- 001943's release of the collection's cached +0x2c object and 001955's pair-record loop (the
  candidate keeps neither), 004861's per-pruner slot-4 call in 000503, 000517's pass over the
  Scene's +0x3c/+0x40 pairs in 001323, the prunable's vptr, its 005297 member and its
  destructor 004892, the collision object's hook teardown 002406;
- 001315's +0xa0 list append and pruner slot-3 call on the add paths: every root now carries
  the pruning collection at +0xa0, but the arm needs +0xdc bit 2 clear (+0xdc starts at 6 and
  every nonzero call sets it; nothing in the candidate clears it), and 000531/000036 run slot 6
  before the prunable is inserted, so neither add path reaches either arm;
- the creation path (000034's model) still registers only a static group's root, and does not
  run 000531's slot 6 on a desc-built group. (Task 5: a static group root now runs its slot 6
  with 1 and a desc-built group is 000034's arm, 001033 and 001041 pushes; a dynamic root keeps
  the model's registration, the factory wrapper's 001315 with 1 and then its flag-0 refresh
  (`nxShapeFactoryRefreshPose`), which the DynamicSetter posed_mirror line needs.)
- 000028 (`nxU32VectorPushBack`, ObjectModel.cpp) still allocates through
  `nxGetSdkAllocator()`, where the listing uses [0x101041bc].

Also changed: a dynamic actor built without shapes registers its record in Scene+0x56c (the
oracle's push and 000503 precede the actor array's growth), and the creation path's factory
wrapper runs 001315 with 1 (the shape takes the Scene stamp at +8, as 000531's slot 6 does).

**Tests.** `PhysicsActorShapeMutationTests` gains a Task 4 block after its existing flow (the
nine registered lines are unchanged): createShape of plane, sphere, box and capsule on a static
and a dynamic actor (promotion; appends growing the group arrays 2 -> 6 -> 14; the pruner growth
at the fifth prunable), releaseShape at the middle, first and last positions and with a foreign
handle, down to E1 0x18f (static) and the group teardown (dynamic), E1 0x1a4 on an empty actor,
installs into it, E1 0x19c and 0x19d, the single-root release, the group's own poses before and
after setGlobalPose (S1), the roots' prunable bytes, and a fresh Scene whose first shape comes
from createShape on a shapeless dynamic actor. Each case prints the allocation transcript, the
shape count and handle order, the group's arrays, +8, +0x10c, +0xdc, id and +0xa0, each child's
type, +0xdc, +8, id and +0xa0, and Scene +4, the pruner counts and +0x540. 136 oracle lines are
registered verbatim; floor 5 = 1707. 125 of them are absent from the Task 3 candidate's
transcript (it returns 0 for every append and crashes at the S1 print).

Counts after Task 4. Of the 53 `discovered` rows:
- 51 are faithful (000070, 000072 and the seven S1 rows joined).
- 2 are M, 000122 and 000164 (Task 5).

The 34 `reconstructed` rows are unchanged: 30 faithful, 2 X (000086, 000088), 2 M (000116,
000118).

## Task 5: mass from shapes and setDynamic

Task 5 (commits 65e3add and 5df7040) wrote updateMassFromShapes (000164) and setDynamic
(000122) from their listings and the chains under them. Line numbers are of 5df7040.

- **000164** (NpActor.cpp:1468): the lock (G1 0x98); E1 0x9a when the density or the total
  mass is below zero or unordered (`fcomp; test ah,1`, density first); 0x9d without a record;
  0x9e without a root (body +0x10); 0x9f both zero and 0xa0 both nonzero (`fucompp; test
  ah,0x44`: -0.0 is zero); 000008 on the body with the density, the total mass's own argument
  slot, an identity pose and an unset diagonal (0x6678-0x66f4); 1 is E1 0xa8, any other nonzero
  E1 0xa9; then the writes and marks in the order of the Rows table, and 000768. No wake, no
  kinematic test.
- **000122** (NpActor.cpp:1579): the lock (G1 0x5b); E1 0x63 (`test ah,5; jnp`: a NaN mass
  passes; the twelve massLocalPose words classified in order); E1 0x66; 000533 on a static
  body's shape (bl = 1); 000026; 1 is E1 0x7c and any other nonzero E1 0x7d, both leaving the
  removed shape out of the Scene (t5_trigger_dynamic_root and t5_plane_dynamic_root show
  +0xa0 = 0 and the pruner count down by one); with an old record: 000632, the record's
  Observable notifyObservers(0x100) (the Foundation import, on an empty observer list), 000776,
  the record freed through [0x101041bc]; 000531(shape, true); unlock.

The chain rows, each claimed with a `// phys_fn_` line:

| Row | RVA | B | Owning unit | Candidate | Notes |
|---|---|---:|---|---|---|
| 000008 | 0x10a0 | 751 | gap:<start>..Actor.cpp | Scene.cpp:3308 `nxActorComputeMassFromShapes` | 000847 zero, the root's slot 4 at unit density (false: 1), mass not above zero (ordered): 2, pose.t = centre, 000841 (0x1c720: each centre word through fld/fchs/fstp, then 000833 with them), the three scaling arms (density with or without totalMass written, else the register ratio totalMass / mass), NxDiagonalizeInertiaTensor ([0x101041b8]) |
| 000026 | 0x19b0 | 465 | Actor.cpp | Scene.cpp:2339 `nxActorBuildRecord` | replaces the creation path's one-box density approximation (`nxActorComputeMass`, removed): 000034 now calls it |
| 000030 | 0x1c40 | 403 | Actor.cpp | Scene.cpp:2430 `nxActorDestroy` | used by releaseActor and the creation failure path (it replaces `nxSceneActorDestroy`, an empty stub) |
| 000628 | 0x123d0 | 241 | Scene.cpp | Scene.cpp:1501 `releaseActor` | rewritten: reentry 0x492, the search, E1-style code-2 report 0x4ae ("double deletion detected!"), swap-remove, 000030, the body freed through [0x101041bc] |
| 000630 | 0x124d0 | 233 | Scene.cpp | Scene.cpp:2124 `nxSceneAddBody` | +0x56c push (2n + 2 through [0x101041bc]) and 000503 |
| 000632 | 0x125c0 | 160 | Scene.cpp | Scene.cpp:2161 `nxSceneRemoveBody` | swap-remove, 000778 into +0x58c, 000760, the 004103 loop |
| 000557 | 0x10840 | 22 | Scene.cpp | Scene.cpp:3563 `pushJointWithoutBodies` | inventory `reconstructed` with no candidate until now |
| 004103 | 0x97c10 | 142 | Joint.cpp | core/Joint.cpp:1042 `Joint::row004103` | wakes and clears both bodies, 000633, flags (& ~8) \| 0x10, 000557 |
| 000722 | 0x16130 | 127 | gap:SceneRaycast.cpp..CapsuleShape.cpp | core/JointSupport.cpp:499 `Row000722Fixture::row000722` | the island snapshot |
| 000776 | 0x18570 | 117 | gap:SceneRaycast.cpp..CapsuleShape.cpp | Scene.cpp:2386 `nxBodyRecordDestroy` | id to +0x6f8 (000028), 000713, the 000722 chain, 000760, 000722, 000799 |
| 000797 | 0x1b5c0 | 402 | gap:SceneRaycast.cpp..CapsuleShape.cpp | Scene.cpp:2207 `nxBodyRecordConstruct` | id first, the 000801 model, the listing's stores, 000760, 000722, 000793 |
| 000793 | 0x1a350 | 1613 | gap:SceneRaycast.cpp..CapsuleShape.cpp | Scene.cpp:2262 `nxBodyRecordApplyDesc` (a model) | its mass block is the listing's; the other fields keep the earlier model |
| 000799 | 0x1b760 | 51 | gap:SceneRaycast.cpp..CapsuleShape.cpp | inside `nxBodyRecordDestroy` | the manager slot and the kinematic block |

Already written and reused: 000531/000533 (Task 4), 001943, 000503, 001279, 000760/000778/000713
(JointSupport.cpp, ObjectModel.cpp), 000028, 000012, 000768.

**Slot-4 audit.** Every runtime family's installed table reaches its mass row at slot 4: box
000947 (`BoxShape::nxBoxAccumulateMass`), sphere 001371, capsule 001008, plane the base 001249
(false), and the group's table (Scene.cpp) now has slot 4 = 001024. 001024's model
(`nxArrayVtCall3Args1024`, ObjectModel.cpp) called each child's slot 4 as a stdcall with no
`this`; it is now a thiscall on the element (0x229d1-0x229ea). 000845 (`nxMassFrameBuildCapsule`)
left +0x00 unwritten for selector 1; the listing stores the side term there (0x1c836) before it
branches, and the capsule's slot 4 passes 1: fixed (the object-layout differential drives 0 and
2 only). 001397 (the mesh slot 4, 0x28e10) is not reachable: the candidate has no runtime
triangle-mesh family (000032's mesh arm fails), so it stays the ObjectModel.cpp model.

**Creation path.** 000034 now runs 000026 (000008 when the tensor is zero bits, the record through
[0x101041bc], 000797, 000630) and reports its failures through the Foundation (Actor.cpp 0xe5 and
0xe6, then createActor's Scene.cpp 0x228; they were printf lines). Its group arm is the listing's
(0x2137-0x21a9): the group through [0x101041bc], its id after the allocation, 001033, +8, and each
child appended by 001041 (the arrays grow 2 -> 6 for three shapes, where the model allocated
exact-size arrays). A static group root runs its slot 6 with 1 (its +0xdc is then 2, as in the
oracle; t5_several_before). The record's +0x188/+0xc0 are now unconditional and a zero tensor
gives 1.0f (000793's arms); every existing Phase 5 transcript is byte-identical.

**Review items from Task 4.**
1. The reentry flag .data 0x10123c10 is one variable, `gNxApiReentry` (Scene.cpp), used by
   createJoint/releaseJoint, releaseActor and 000036/000024.
2. The two `// phys_fn_001273` lines: the canonical claim is ObjectModel.h:233, the declaration of
   `ShapeBase::ShapeBase` (ObjectModel.cpp; the object-layout differential drives it); Scene.cpp's `nxRuntimeShapeBaseInit` is the runtime
   shapes' copy of the same stores.
3. releaseActor is now 000628 -> 000030: the runtime shapes and groups go through their deleting
   destructors (001323's id push before each free, the [0x101041bc] frees), the record through
   000632/000776. The creation failure path uses 000030 too.
4. The stale "flag-0 refresh" bullet in `## Task 4` is corrected in place.

**Not reproduced** (recorded):
- 000793/000795 beyond the mass block: its dirty marks at creation and its other fields' exact
  arms (the model's damping, sleep and velocity stores are kept);
- 000801's full sub-object (the model writes the pose, the quaternion, +0x120, the id and the
  manager slot), the record's vptr (0x10106890) and the Observable constructor and destructor
  (the candidate's zeroed words are an empty observer list);
- 000521 -> 000517's pass over the Scene's +0x3c/+0x40 pair table (the candidate keeps none),
  003635 (no fluid manager);
- 000030 frees the public actor directly (000118, its deleting destructor, has no body to run in
  the candidate);
- the reentry flag cannot be set from the harness, so 0x492 is static-only; 0x4ae is not driven.

**Tests.** `PhysicsActorShapeMutationTests` gains two Task 5 blocks on fresh Scenes:
updateMassFromShapes over a rotated box, a sphere, a capsule, a centred box, a group of the three,
a trigger with a sphere, a plane (E1 0xa8), a group holding a plane, and a trigger (E1 0xa9),
with a density and with a total mass; every argument E1 (NaN and -0.0 included), a static actor,
a shapeless one, a denormal mass, an infinite density, FLT_MAX, G1 0x98; the creation path's mass
pass for each family (density and total mass) and its two failures; setDynamic on a static actor
with one shape, one with three (a group) and a density, one with a capsule and a density, a
dynamic actor again, a shapeless one (E1 0x66, then a tensor), a negative mass and a NaN pose
(0x63), a trigger (0x7d) and a plane (0x7c), G1 0x5b, and a jointed dynamic actor (004103 breaks
the joint: state 2, both actors null); each is used afterwards (a force, a torque, a velocity
and getLinearVelocity, getMassSpaceInertiaTensor, getCMassGlobalPosition, getMass) and released.
Each case prints the record's mass words, the root's pruning bytes and the Scene's counts. 110
oracle lines are registered verbatim (repeated report lines once); floor 5 = 1817. 99 of the
110 are absent from the Task 4 candidate's transcript (6b1a6f2's DLL under the new test).

**Task 5 review** (the follow-up commit):
- 000833 (`MassFrame::nxMassFrameTranslate`, ObjectModel.cpp) rewritten from the listing
  (0x1c040-0x1c598): P = S(o) S(o) and, unless the new centre is all zero bits, Q = S(c) S(c) in
  the listing's operand orders, with the 0.0f-multiplied diagonal terms (their zero signs and an
  infinity's NaN) and the spilled -c.y^2/-c.x^2 and -o.x^2; T = fl(P - Q), fl(T m) + I. Checked
  word for word against the oracle's 000833 and 000841 on four translations (a scratch probe calling
  the oracle rows in-process).
- 000849 (the box's mass frame) calls 000833 (0x1c8e3-0x1c8eb) instead of its inlined centred
  specialization, which rounded the squares' sums differently.
- 000829 (`nxMassFrameBuildBox`) spills the factor mass/3 and the three pairwise sums to float
  (0x1bd31-0x1bd72); the model kept them in the register and differed in the last bit at the
  review's extents.
- Signalling NaNs: 000793's mass (fld/fst), 000030's position x/y (fld/fstp) and 000841's
  negations (fld/fchs/fstp; the load quiets it) go through the x87 (`nxX87MoveFloat`,
  `nxX87NegateFloat`, Scene.cpp); a setDynamic case with an SNaN mass shows 0x7fa00000 stored
  as 0x7fe00000.
- Tests: boxes and capsules at four inexact translations (with and without rotations), through
  updateMassFromShapes and creation, and setDynamic with a quiet and a signalling NaN mass: 32
  more oracle lines, floor 5 = 1849. The object-layout differential (boxslot4 64, mftranslate 54,
  negtrans 5, mfcombo 6) still has no failure.

Counts after Task 5. Of the 53 `discovered` rows, **53 are faithful**. The 34 `reconstructed` rows
are unchanged: 30 faithful, 2 X (000086, 000088), 2 M (000116, 000118).

## Task 6: the final trace

Task 6 re-ran the cdb trace over all twelve Phase 5 actor staged-pair targets after Task 5 and its
review (`evidence/npactor-trace-final.txt`; first recorded on an incremental build, sha256 5c2e247f..., and re-recorded unchanged by Task 7 on the clean verification build, sha256 ccae6021..., which differs only in its link timestamp). The breakpoints cover the
84 unit rows with a candidate function (Task 1's set less the 000118 counterpart) and every chain
row this plan wrote or fixed, 138 in all, taken from `build/Release/NxPhysics.map` of that build.

- **Unit.** Every one of the 53 `discovered` rows executes: 000122 (14 hits) and 000164 (48), which
  Task 1 could not reach, are now driven by the Task 5 ShapeMutation blocks; 000216 has no function
  of its own and 000214's 13 hits run its epilogue. Of the 34 `reconstructed` rows, 000102 (not hit
  in Task 1) now runs 7 times; 000116 and 000118 still have no candidate function.
- **Chain rows.** Every row the plan wrote as faithful executes: 000006 2, 000008 77, 000024 19,
  000026 144, 000032 200, 000036 16, 000531 14, 000533 10, 000535 140, 000628 109, 000630 140,
  000632 3, 000722 420, 000746 185, 000782 107, 000784 7, 000785 24, 000787 10 (two instruction
  breakpoints in the disable arm of `nxNpActorTransitionKinematic`), 000789 24, 000791 24, 001018 27,
  001028 14, 001032 27, 001033 27, 001039 27, 001041 19, 001279 175, 001941 7, 001945 171, 004103 1.
  Executed only inlined, with the caller that proves it: 000756 (setCMassGlobalPose/Orientation, the
  line before 000789), 000799 (inside `nxBodyRecordDestroy`, 000776, 3 hits), 001037 (inside the
  group's deleting destructor, 001039, 27 hits), 000829 (000849's first call, 55 hits).
- **noinline.** Four Scene.cpp helpers the compiler had inlined into conditional arms are now
  `__declspec(noinline)` so that a breakpoint sees them run, as the oracle calls each as a function
  (the precedent is addJoint and 000760): 001028 `nxShapeGroupRemoveChild`, 001032
  `nxShapeGroupDeleteChildren`, 001941 `nxPruningAddShape` and 000535 `nxSceneRemoveDynamicShape`.
  Their out-of-line copies had no caller in the image (checked by disassembly). Every Phase 5
  staged-pair transcript is byte-identical after the change.
- **Stable-ID lines.** 000128 (`getGlobalPoseReference`) and 000216 (inside
  `setCMassOffsetLocalOrientation`) had none in NpActor.cpp; both now do, in the
  `// phys_fn_NNNNNN (0x%08x, N B)` form. ObjectModel.cpp's "Provisional phys_fn_000947" comment is
  now the row's line and a description checked against 0x20850-0x20874 (the wrapper is faithful).
- **Cases.** No row in scope is unexecuted, so no case was added: floor 5 stays 1849.

## Task 7: promotion

Task 7 moved to `reconstructed` every row this plan reviewed as faithful whose execution the final
trace shows, and recorded the rest. For each promoted row `inventory.json` has `implementation` = the
candidate file carrying its `// phys_fn_` line (NpActor.cpp for the unit; Scene.cpp, NpActor.cpp,
NpActorDynamicMath.h, core/JointSupport.cpp or core/Joint.cpp for the chain rows), `source` kept
where it was the oracle's attribution (Actor.cpp for 000024/000026/000036) and set to the
implementation where it was null, the owning unit in `notes`, `static_proof` = this contract's
verdict and summary with the staged-pair targets that cover it, and `dynamic_proof` = the trace
(`evidence/npactor-trace-final.txt`, candidate sha256 ccae6021...). Each owning closure ledger now
gives the row `reconstructed_not_falsified` (Phase 5: 63 rows; Phase 2: 5; Phase 3: 9; Phase 6: 1;
Phase 7: 7).

- **Unit: 53 rows, 30,673 B.** Every `discovered` row of NpActor.cpp; the unit is now 87 of 87
  `reconstructed`. 000216 is proven through 000214's hits (its range is 000214's tail and
  epilogue).
- **Chain rows: 32 rows, 12,219 B.** 000006, 000008, 000024, 000026, 000036 (Actor.cpp and the gap
  before it); 000531, 000533, 000535, 000628, 000630, 000632, 000799 (Scene.cpp and phase 7);
  000722, 000746, 000756, 000782, 000791 (phase 2, homeless shared code until this plan); 000784,
  000785, 000787, 000789 (phase 5); 001018, 001028, 001032, 001033, 001037, 001039, 001041, 001279,
  001945 (phase 3); 001941 (phase 5); 004103 (phase 6). Unreproduced arms these rows have are
  unreachable in the product today and the proof says so: the fluid-manager calls of 000036,
  000531, 000533, 000535 and 000628 (Scene +0x61c is only ever zeroed in the candidate). The
  reentry reports (0x150, 0x186, 0x492) and 000628's double-deletion report are written and
  static-only: no harness callback can set the flag.
- **Evidence appended** to rows already `reconstructed` whose candidate this plan changed: 000557
  (the product row), 000829 and 000849 (Task 5 review), 000833 (rewritten from the listing), 000845
  (the selector-1 store, which its old proof denied), 000947 (re-read; the comment), 001024 (the
  thiscall), 001273 (the runtime copy's hits).

**Left `discovered`**, each with its reason:

| Row | B | Owner | Why |
|---|---:|---|---|
| 000030 | 403 | Actor.cpp | the public actor is freed directly, not through its slot-0 deleting destructor 000118 (no table stores, no 002406); 000521 -> 000517's pass over the Scene's pair table is absent |
| 000032 | 539 | Actor.cpp | the triangle-mesh arm (0xe8 shape, 001379, Scene +0x10, its own 000503) is not reproduced. No valid mesh descriptor can exist (createTriangleMesh returns 0 and NxTriangleMeshShapeDesc::isValid rejects a null mesh), but createActor validates with NxActorDescBase::isValid(), which runs no shape loop, so reaching the arm with an invalid descriptor is not excluded |
| 000034 | 565 | Actor.cpp | the creation path is still a model (registration of a dynamic root, the flag-0 refresh) |
| 000503 | 232 | gap:PhysicsSDK.cpp..Scene.cpp | 004861's per-pruner slot-4 call is absent |
| 000768 | 1,164 | gap:SceneRaycast.cpp..CapsuleShape.cpp | not written or reviewed as a row by this plan (only its calls from 000196-000202 were checked) |
| 000776 | 117 | gap:SceneRaycast.cpp..CapsuleShape.cpp | the record vptr store (0x10106890) and the Observable destructor call are not carried |
| 000793, 000795 | 1,613, 3,090 | same | models; only 000793's mass block follows the listing |
| 000797 | 402 | same | the record vptr, the Observable constructor and 000801's full sub-object are not carried |
| 000801 | 741 | same | a model of the pose sub-object |
| 001315 | 1,061 | Shape.cpp | the pruner slot-3 call is modelled as the +0x38 counter and can be reached (a posed shape already in a pruner); the +0xa0 append is not reproduced; the candidate adds a scene-null guard |
| 001323 | 182 | Shape.cpp | 000517's pass over the pair table and the hook teardown 002406 are not carried |
| 001397 | 104 | the mesh shape (MESH table slot 4) | the mesh slot 4; no runtime mesh family |
| 001943 | 270 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | the release of the collection's cached +0x2c object is not carried |
| 001955 | 153 | gap:ContactPlaneMesh.cpp..PenetrationMap.cpp | the pair-record loop is empty |
| 004852, 004857, 004859 | 147, 213, 55 | opcode | the OPCODE pruner stays a model |

The unit's already-`reconstructed` rows keep their Task 3 verdicts: 000086 and 000088 (X, the name
table) and 000116 and 000118 (M, no member table or destructor) were not re-promoted.

## Callees the implementing tasks need

- (Task 5 wrote 000008, 000026 and the record rows; see `## Task 5`.)
- 000008 (0x10a0, 751 B, `discovered`, gap `<start>..Actor.cpp`): the body's mass-from-shapes
  computation 000164 calls (ecx = [actor+0x14]; density, &totalMass, &pose, &diag). It
  dispatches `[body+0x10]->vtbl[4](dest, 1.0f, ...)` to the per-shape mass wrappers: group
  001024, box 000947, capsule 001008, sphere 001371 (reconstructed), 001397 (0x28e10, 104 B,
  `discovered`), base 001249. Task 2 must confirm that the candidate's internal shapes really
  dispatch slot 4 (ObjectModel.h describes several ShapeBase slots as "carried opaque").
  Scene.cpp (~2064-2100) already approximates the 000026/000008 density path at actor
  creation; Task 2 decides whether to replace it with the listing-faithful 000008. 000026
  also calls 000008, so setDynamic (000122) needs 000008 too.
- 000026 (0x19b0, 465 B, `discovered`) and 000776 (0x18570): setDynamic's record build and
  record destructor; 000632/000531/000533/000535 (Scene add/remove body and shape).
- 000036/000024 (Actor.cpp createShape/releaseShape, `discovered`), 000032 (shape factory,
  539 B), 001041, 001028, 001033, 000006, 001941, 000503, 003628, 000012, 001957, 001960: the
  shape add/remove paths 000070/000072 forward to. (Task 4 wrote them; see `## Task 4: shape add/remove`.)
- 000756 (0x17420) = `nxNpActorBodyQuaternionFromMatrix(rec+0x134, rec+0x124)`; 000789
  (0x19d00) = world-mass-pose apply (000746 tensor, listing displacement order, setter
  quaternion); 000785 (0x19620) = the kinematic transition with the 000712 island-root
  refresh. It runs to 0x19cf9: the part past the inventory's 1,325 B is row 000787 (0x19b50,
  428 B), 000785's tail in the same way 000216 is 000214's.
- 000782 (0x18730) and 000784 (0x194b0): the body force/torque accumulator and the kinematic
  target writer with its wake.

Cosmetic (M5): the spill names in `nxNpActorRotationFromQuaternionGetter` (yy, yz, xz,
diagonal, xw) do not all name what the slots hold; the instruction sequence is exact.

## Dependency chains

Sizes of the rows each open item pulls in (the Task 1 review's sizing, from the inventory):

| Chain | Rows (B) | Total |
|---|---|---:|
| 000164 updateMassFromShapes | 000008 (751), 001397 (104), plus the slot-4 audit of the candidate shapes | 855 B + audit (Task 5: done, 001397 unreachable; see `## Task 5`) |
| 000122 setDynamic | 000026 (465), 000797 (402), 000630 (233), 000776 (117), 000722 (127), 000632 (160), 004103 (142), 000531 (97), 001943 (270), 000503 (232), 000533 (40), 001279 (53); plus 000008 above | 2,338 B (Task 5: done; see `## Task 5`) |
| shape add/remove (000070/000072) | 000036, 000024, 000032, 001041, 001033, 001028, 000006, 001941 | ~2.1 KB (Task 4: done, see `## Task 4`) |
| force/torque (000054-000058, 000154-000162) | 000782 | 3,428 B |
| CMass-global setters (000204-000208) | 000789 + 000746 | 1,706 B |
| body flags (000188/000190) | 000785 + 000787 | 1,753 B |
| kinematic moves (000090, 000124, 000126) | 000784 | 368 B |

## Listing review detail

The seven review passes of Task 1, per row. Callee disassemblies they mention were made
read-only from the oracle image into scratch files that are not committed; the addresses
given are enough to regenerate them from `oracle/capstone/manifest.json` or the image.

### NpActor.cpp listing review -- GROUP A

Rows: 000054, 000056, 000058, 000070, 000072, 000090, 000122, 000124, 000126, 000128.
Oracle callees that had no listing in (scratch) were disassembled read-only from
D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll with Capstone (helper script (scratch),
dumps in (scratch)): 000782 (0x18730), 000791 (0x1a2c0), 000784 (0x194b0),
000036 (0x2250), 000024 (0x1860), 000006 (0x1080), 001028 (0x22b50), 000026 (0x19b0), 000776 (0x18570).
Imports resolved: [0x101041ac] = NxFoundation::Observable::notifyObservers(unsigned),
[0x101041bc] = nxFoundationSDKAllocator (vtbl +8 malloc(size, 0), +0x14 free).
Constants: [0x101053d4] = 0x3ecccccc (0.39999998f), [0x101041f0] = 0.0f, [0x101041ec] = 1.0f, [0x101043cc] = 0.5f.

Shared observations used below:
- W1 (wake test in 000782 tail 0x1001936f-0x1001948a and 000784 0x1001950a-0x1001961c):
  wakes when [rec+0x84] < 0.39999998f (ordered, strict: `fcomp; test ah,5; jp skip` also skips on
  equal) and (+0x114 & 0x100) == 0: stores 0x3ecccccc into +0x84 and +0x4c and ORs dirty 0x10. The
  candidate tests the same condition. (The review first read this as `<=`; that was withdrawn.)
- H1 (dirty-mark inline, e.g. 0x100187a1-0x1001887c): no bounds/null checks; the candidate
  helper nxNpActorMarkRecordDirty (NpActor.cpp:91) additionally returns early when
  record+0x11c id >= 256 or any list pointer is null. For id >= 256 the candidate drops the
  dirty mark entirely. (Grow policy, malloc(4*(2n+2),0)/copy/free order and map[id]=count match.)

#### phys_fn_000054 (0x26f0, 167 B) -- addForceAtPos (slot 54)
- candidate: NpActorVtable::addForceAtPos Physics/src/NpActor.cpp:1993; helpers nxNpActorForceAtPos :1962, nxNpActorAccumulateForce :2037, nxNpActorMarkRecordDirty :91
- status: implemented
- verdict: defect
- blocks checked: 0x26f0-0x2727 write-lock try + G1 (line 0x12a); 0x272a-0x2740 record = [[actor+0x14]+8], null or `test byte [rec+0x10c]; js` (kinematic) -> error arm; 0x2742-0x2761 call 000791(force, pos, mode, 1) with ecx=record, unlock; 0x2764-0x2794 E1 arm + unlock.
  000791 (0x1a2c0-0x1a342): r = pos - rec[+0x158..+0x160]; torque = r x force; calls 000782(force, &torque, mode, 1).
  000782 (0x18730-0x19491): jump table on mode 0..4 (mode > 4 jumps straight to the wake tail); per mode linear then angular update, dirty masks; tail W1 wake gated by the 4th arg.
- defects:
  - 0x2764-0x2788 E1: oracle reports kind 1, line 0x12b, "Actor::addForceAtPos: Actor must be (non-kinematic) dynamic!" for static or kinematic actors; candidate skips silently.
  - 0x1a2c7-0x1a2cf (000791): r.x = pos.x - rec[0x158] is never spilled (used as st(2) through the cross product); r.y, r.z are fstp'd to float. Candidate builds `const NxVec3 lever` (float members), rounding r.x to float -> x87 order defect on torque.y/torque.z.
  - 0x10018759-0x1001879b (000782 mode 0 linear) and 0x10018ecf-0x10018f11 (mode 3 linear): x increment invMass*f.x stays on the x87 stack and is added to +0x88/+0xa0 unrounded; only y,z are spilled. Candidate stores all three to `float increment[3]` first -> x87 defect (NX_FORCE, NX_SMOOTH_IMPULSE).
  - 0x1001888b-0x100188fd (mode 0 angular), 0x10018b44-0x10018bca (mode 1), 0x1001900b-0x1001908d (mode 3): each row is ((I[r][2]*z + I[r][1]*y) + x*I[r][0]) over rec+0x164; modes 0/3 keep rows 0 and 1 unrounded (only row 2 spilled), mode 1 keeps row 0 unrounded (rows 1,2 spilled). Candidate sums (I0*x + I1*y) + I2*z in double and rounds each row to float -> summation-order and spill defect.
  - 0x10018740 / 0x1001936f: mode > 4 skips accumulation but still runs the wake tail; candidate `default: return;` skips the wake.
  - wake placement: the oracle runs one wake after both linear and angular updates, the candidate after each (order of dirty OR bits differs, final state is the same).
  - H1 applies.
- notes: oracle does not null-check the body ([actor+0x14]); candidate does. 000782 masks confirmed: mode0 lin +0x88 0x20 / ang +0x94 0x40; mode1/2 lin +0x6c (+0x34 copy) 4 / ang +0x78 (+0x40 copy) 8; mode3/4 lin +0xa0 0x80 / ang +0xac 0x100 -- candidate targets/masks match. Modes 2/4 are plain `old + v` adds (match).

#### phys_fn_000056 (0x27a0, 165 B) -- addForce (slot 58)
- candidate: NpActorVtable::addForce Physics/src/NpActor.cpp:2094 -> nxNpActorAccumulateForce(angular=false) :2037
- status: implemented
- verdict: defect
- blocks checked: 0x27a0-0x27d7 write-lock try + G1 (line 0x14d); 0x27da-0x27f0 record null / kinematic byte test; 0x27f2-0x280e call 000782(force, NULL, mode, 1), unlock; 0x2811-0x2842 E1 arm + unlock.
- defects:
  - 0x2811-0x2836 E1: kind 1, line 0x14e, "Actor::addForce: Actor must be (non-kinematic) dynamic!"; candidate silent.
  - 000782 linear arm x87: modes 0 and 3 keep invMass*f.x unrounded (0x10018759-0x1001877b, 0x10018ecf-0x10018ef1); candidate rounds via float increment[].
  - mode > 4: oracle still wakes (0x1001936f tail); candidate returns before the wake.
  - H1.
- notes: with force non-null and torque NULL, 000782 skips the angular arm (`test eax,eax; je` at 0x1001887f etc.); candidate equivalent.

#### phys_fn_000058 (0x2850, 165 B) -- addTorque (slot 60)
- candidate: NpActorVtable::addTorque Physics/src/NpActor.cpp:2135 -> nxNpActorAccumulateForce(angular=true) :2037
- status: implemented
- verdict: defect
- blocks checked: 0x2850-0x2887 write-lock try + G1 (line 0x160); 0x288a-0x28a0 record/kinematic test; 0x28a2-0x28be call 000782(NULL, torque, mode, 1), unlock; 0x28c1-0x28f2 E1 arm + unlock.
- defects:
  - 0x28c1-0x28e6 E1: kind 1, line 0x161, "Actor::addTorque: Actor must be (non-kinematic) dynamic!"; candidate silent.
  - 000782 angular arm: row order ((I2*z + I1*y) + I0*x) with rows 0,1 unrounded in modes 0/3 and row 0 unrounded in mode 1 (addresses as in 000054); candidate order (I0*x + I1*y) + I2*z, each row rounded.
  - mode > 4 wake; H1 (as 000056).

#### phys_fn_000070 (0x2a80, 185 B) -- createShape (slot 13)
- candidate: NpActorVtable::createShape Physics/src/NpActor.cpp:1172 -> nxActorAppendShape Physics/src/Scene.cpp:2424
- status: partial (only the single->group promotion is modelled; empty-actor and existing-group cases return 0; no scene add/remove calls)
- verdict: defect
- blocks checked: 0x2a80-0x2ab9 write-lock try + G1 (line 0x1ab), returns 0; 0x2abc-0x2b03 `desc->vtbl[2]()` (isValid) false -> E1 + unlock, return 0; 0x2b06-0x2b36 000036(body=[actor+0x14], desc); result non-null -> return [shape+0x9c] (public NxShape*), else 0; unlock on both.
  000036 Actor::createShape (0x2250-0x23f1, Actor.cpp):
  - 0x2250-0x2288 reentry guard: global [0x10123c10] != 0 -> error kind 2, Actor.cpp (0x10104278) line 0x150, "Reentry check: You may not call this API method from a callback!", return 0. 0x2294 sets guard = 1; cleared on every exit (0x2336, 0x23e6).
  - 0x228b-0x22ad new = 000032 shape factory (0x1de0, cdecl (desc, body)); old = body+0x10.
  - 0x23c8-0x23dc old == 0: body+0x10 = new; if new, scene(body+4)->000531(new, body+8 != 0) (0x10600 add shape).
  - 0x22b3-0x2341 old is group (kind [+0xd0]==5): group->001041(new) (0x22e80 add child); new->vtbl[6](1) (`call [eax+0x18]`, push 1); (scene+0x624)->001941(new, hasRecord) (0x4ba80); n = 001957(scene+0x624) + 001960(scene+0x624); scene->000503(n) (0x100a0); if scene+0x61c != 0: 003628(new, hasRecord) (0x89bb0) on it. Returns new.
  - 0x2344-0x23dc old is a single shape: scene->000535(old) if record else 000533(old) (0x106a0 / 0x10670 remove); malloc(0x110, 0); group ctor 001033 (0x22d60)(body, id from 000012(scene+0x6e4)); body+0x10 = group; group+8 = scene[+0x540] - 1; group->001041(old); group->001041(new); scene->000531(group, hasRecord). Returns new (the child, not the group). (A failed malloc writes [0+8] -- oracle would fault.)
- defects:
  - 0x2adb-0x2af4 E1: kind 1, line 0x1ac, "Actor::createShape: desc.isValid() fails!"; candidate's helper returns 0 silently.
  - 0x2250-0x2288 reentry guard (Actor.cpp line 0x150) absent in candidate.
  - 0x23c8-0x23dc: actor with no shape -> oracle installs the new shape as body+0x10 and adds it to the scene; candidate returns 0 (`!original`).
  - 0x22b3-0x2341: actor already holding a group -> oracle appends the child (001041), calls child vtbl[6](1), scene +0x624 / +0x61c bookkeeping and 000503; candidate returns 0 (`kind == 5`).
  - 0x2344-0x23b5 promotion: oracle removes the old single shape from the scene (000535/000533), builds the group with 001033 and 001041 x2 (array capacity/growth from 001041, not a fixed 2), writes group+8 = scene[+0x540]-1 and adds the group to the scene with 000531(group, hasRecord). Candidate allocates exact 2-entry arrays itself, never writes group+8, registers via nxSceneAuxRegisterShape and does not remove the old shape from the scene.
  - Allocation order: oracle factory -> group(0x110) -> arrays inside 001041; candidate factory -> group -> shapes[2] -> helpers[2].
- notes: return is [shape+0x9c] in both. Candidate returns helpers[1] which equals child+0x9c (same value).

#### phys_fn_000072 (0x2b40, 90 B) -- releaseShape (slot 14)
- candidate: NpActorVtable::releaseShape Physics/src/NpActor.cpp:1181 -> nxActorRemoveShape Physics/src/Scene.cpp:2475
- status: partial (group-array compaction only)
- verdict: defect
- blocks checked: 0x2b40-0x2b77 write-lock try + G1 (line 0x1b3); 0x2b7a-0x2b97 000024(body=[actor+0x14], [shape+8]) i.e. the INTERNAL shape pointer from the NxShape object, then unlock.
  000024 Actor::releaseShape (0x1860-0x19a5, Actor.cpp 0x10104278):
  - 0x1860-0x1898 reentry guard (kind 2, line 0x186, "Reentry check: ..."); 0x189b guard = 1, cleared on all exits.
  - 0x18a5 / 0x1978-0x1994 body+0x10 == 0 -> kind 1, line 0x1a4, "Actor::releaseShape: shape not found!".
  - group (0x18b0-0x1933): if count == 1 and body+8 == 0 (static) -> kind 1, line 0x18f, "Actor::releaseShape: Can't release shape: A static actor can't be left with no shapes!". Else 001028(shape) (0x22b50: find in +0xe0 array; not found -> false, no error; found -> overwrite slot with LAST entry in both +0xe0 and +0xf0 arrays (swap-remove), end pointers -= 4, child byte +0xdc &= ~1, group+0x10c = -1.0f (0xbf800000)). If removed and the group is now empty: 000006(body) (0x1080: scene remove of body+0x10 via 000535/000533 by record presence), group->vtbl[0](1) (deleting dtor), body+0x10 = 0.
  - single (0x193e-0x1976): static -> kind 1, line 0x19c (static-actor message); body+0x10 != shape -> kind 1, line 0x19d, "shape not found!"; equal -> 000006(body), body+0x10->vtbl[0](1), body+0x10 = 0.
- defects:
  - argument: oracle passes [NxShape+8] and matches against the internal +0xe0 array; candidate passes the public NxShape* and matches the +0xf0 helper array (same pairing, different key).
  - 0x22b85-0x22bb4: oracle swap-with-last removal; candidate shifts later entries down (order-preserving) -> different resulting array order for 3+ shapes.
  - 0x22bd0-0x22bd9: child +0xdc bit0 clear and group +0x10c = -1.0f not done by candidate.
  - 0x1923-0x1930 / 0x191c: empty-group teardown and single-shape release (scene removal + deleting dtor + body+0x10 = 0) absent; candidate returns without action for a single shape.
  - E1 set absent: lines 0x18f, 0x19c (static actor can't be left with no shapes), 0x19d, 0x1a4 (shape not found!); reentry guard line 0x186 absent.
- notes: the oracle does not free the removed child inside 001028 (it only unlinks it).

#### phys_fn_000090 (0x2df0, 211 B) -- moveGlobalPosition (slot 11)
- candidate: NpActorVtable::moveGlobalPosition Physics/src/NpActor.cpp:1147
- status: partial (target offset and wake missing)
- verdict: defect
- blocks checked: 0x2df0-0x2e2d write-lock try + G1 (line 0x29e); 0x2e30-0x2e46 record null or NOT kinematic (`jns`) -> error arm; 0x2e48-0x2e8a target = rec[+0x100..+0x108] + position (per component `fld rec; fadd pos`, z,y,x computed then all three fstp to a float temp), call 000784(&target, NULL), unlock; 0x2e8d-0x2ec0 E1 arm.
  000784 (0x194b0-0x1961d): if pos: copy to [rec+0x118]+0..8, [target+0xc] |= 1; if quat: copy to target+0x10..0x1c, [target+0xc] |= 2; then W1 wake unconditionally (not gated by an argument).
- defects:
  - 0x2e48-0x2e75: oracle adds the record vector at +0x100..+0x108 to the requested position before storing; candidate stores the raw position.
  - 0x1950a-0x1961c (000784): wake (+0x84/+0x4c = 0x3ecccccc, dirty 0x10 under W1 condition) absent in candidate.
  - 0x2e8d-0x2eb1 E1: kind 1, line 0x2a1, "Actor::moveGlobalPosition: Actor must be kinematic!"; candidate silent.
- notes: candidate's `target` null check has no oracle counterpart (000784 dereferences +0x118 directly). Flag update is `|= 1` in both.

#### phys_fn_000122 (0x3840, 761 B) -- setDynamic (slot 18)
- candidate: NpActorVtable::setDynamic Physics/src/NpActor.cpp:1226 (empty body)
- status: missing
- verdict: defect (nothing implemented)
- blocks checked (oracle summary for implementation):
  - 0x3840-0x3888 frame (`and esp,-8`), write-lock try on [actor+0xc]; G1 line 0x5b, return.
  - 0x388b-0x38a8 body = [actor+0x14], ctx, oldRecord = body+8 saved.
  - 0x3894-0x38b1 desc.mass (+0x3c) < 0.0f (`test ah,5; jnp`) -> invalid. 0x38b7-0x39ec each of the 12 floats of desc.massLocalPose (+0x00..+0x2c, M then t) passed as double to 005666 (0x100f4140, _fpclass); result & 0x207 (SNaN|QNaN|-Inf|+Inf) -> invalid. Invalid -> 0x39ee-0x3a52: kind 1, line 0x63, "Actor::setDynamic: desc.isValid() fails!", unlock, return. (NaN mass passes the mass test.)
  - 0x3a05-0x3a1f body+0x10 (shape) == 0 and desc.massSpaceInertia (+0x30,+0x34,+0x38) all zero as INTEGER bit patterns (-0.0 counts as non-zero) -> 0x3a21-0x3a52 kind 1, line 0x66, "Actor::setDynamic: we need a valid massSpaceInertia if the actor has no shapes!".
  - 0x3a55-0x3a6b / 0x3a98: if oldRecord == 0 (static) and shape != 0: scene(body+4)->000533(shape) (0x10670, remove static shape), bl = 1; else bl = 0.
  - 0x3a70-0x3a78 r = 000026(body, &desc) (0x19b0, Actor.cpp body-level). r == 1 -> kind 1, line 0x7c, "Actor::setDynamic: Compute mesh inertia tensor failed for one of the actor's mesh shapes! Please change mesh geometry or supply a tensor manually!"; r other non-zero -> kind 1, line 0x7d, "Actor::setDynamic: Can't compute mass from shapes: must have at least one non-trigger shape!"; both unlock and return WITHOUT re-adding the shape removed above.
  - 000026 (0x19b0-0x1b7e): copies desc into a local NxBodyDesc (0x10001390); if massSpaceInertia bits all zero, 0x100010a0(body+0x18, &local..., ...) computes mass/inertia from shapes and returns its error code; pose = old record's quaternion (+0x5c, same inline quat->matrix sequence as 000128) and +0x50 position, or body+0x20 pose when static; malloc(0x260, 0); ctor 0x1001b5c0(body, &pose, &localDesc); body+8 = new record (0 if malloc failed); scene(body+4)->0x100124d0(newRecord) (add body); returns 0.
  - 0x3ad7-0x3b0e success with oldRecord != 0: scene->000632(oldRecord) (0x125c0, remove body); oldRecord->Observable::notifyObservers(0x100); 000776(oldRecord) (0x18570, record destructor body: vptr 0x10106890, scene+0x6f8 id release, child/joint lists, 0x17710, 0x16130, import [0x10104194], sub-object +0x18 dtor); allocator free(oldRecord).
  - 0x3b11-0x3b22 if bl: scene->000531(shape, 1) (0x10600, re-add the shape as dynamic).
  - 0x3b27-0x3b36 unlock, `ret 4`.
- defects: entire behaviour absent (lock/G1 0x5b, validation E1 lines 0x63/0x66/0x7c/0x7d, record creation via 000026, old record teardown, shape scene re-registration).
- notes: order matters -- new record is created and installed (body+8) before the old one is removed/destroyed.

#### phys_fn_000124 (0x3b40, 1075 B) -- moveGlobalPose (slot 10)
- candidate: NpActorVtable::moveGlobalPose Physics/src/NpActor.cpp:1128
- status: partial (CMass-frame composition and wake missing)
- verdict: defect
- blocks checked: 0x3b40-0x3b83 write-lock try + G1 (line 0x28f); 0x3b86-0x3ba0 record null or not kinematic -> E1 arm; 0x3ba6-0x3c23 target position = pose.M * c + pose.t with c = rec[+0x100..+0x108]; 0x3c27-0x3d6c target rotation R = pose.M * L with L = rec[+0xdc..+0xfc] (row-major), 9 products each fstp to float then copied (rep movsd); 0x3d72-0x3f14 quaternion from R; 0x3f16-0x3f36 000784(&pos, &quat), unlock; 0x3f39-0x3f70 E1 arm.
  Exact orders (M = pose.M row-major at pose+0..+0x20, t at +0x24):
  - px = ((c0*M00 + c1*M01) + c2*M02) + t.x kept unrounded until the final store; py = float((M12*c2 + c0*M10) + M11*c1) + t.y; pz = float((c1*M21 + M20*c0) + c2*M22) + t.z.
  - R00=(L10*M01+L00*M00)+M02*L20; R01=(L21*M02+L11*M01)+L01*M00; R02=(L12*M01+M00*L02)+L22*M02; R10=(L00*M10+M12*L20)+M11*L10; R11=(L01*M10+M12*L21)+M11*L11; R12=(L22*M12+L12*M11)+L02*M10; R20=(L00*M20+M22*L20)+L10*M21; R21=(L21*M22+L11*M21)+L01*M20; R22=(M20*L02+L22*M22)+L12*M21 (each rounded to float).
  - quat: tr = (R22 + R11) + R00, with (R22+R11) also `fst` to a float temp. tr >= 0: s = sqrt(tr + 1); w = 0.5*s; r = 0.5/s; x = (R21-R12)*r, y = (R02-R20)*r, z = (R10-R01)*r. Else i = argmax diagonal (R11 > R00 -> 1; R22 > R[i][i] -> 2, strict `test ah,0x41`): case 0 s = sqrt((R00 - float(R22+R11)) + 1) (uses the spilled float partial sum); case 1 s = sqrt((R11 - (R22+R00)) + 1); case 2 s = sqrt((R22 - (R11+R00)) + 1); then the usual 0.5*s / 0.5/s products (see 0x3e2d-0x3f10). Quat stored x,y,z,w.
- defects:
  - 0x3ba6-0x3d6c: oracle moves the body's CMass frame (pose * {L, c}); candidate stores pose.t and NxQuat(pose.M) directly, ignoring rec+0xdc/+0x100.
  - 0x3d72-0x3f14: quaternion extraction sequence (spilled partial trace, case formulas) is the oracle's own inline; candidate uses NxQuat(const NxMat33&) on a different matrix.
  - 0x194d5/0x19506 (000784): flags ORed (|= 1 then |= 2); candidate assigns target+0xc = 3, clobbering other bits.
  - 000784 wake (W1) absent.
  - 0x3f39-0x3f5e E1: kind 1, line 0x291, "Actor::moveGlobalPose: Actor must be kinematic!"; candidate silent.

#### phys_fn_000126 (0x3f80, 1192 B) -- moveGlobalOrientation (slot 12)
- candidate: NpActorVtable::moveGlobalOrientation Physics/src/NpActor.cpp:1164 (getGlobalPositionVal :955 + moveGlobalPose :1128)
- status: partial (delegates; inherits 000124's gaps)
- verdict: defect
- blocks checked: 0x3f80-0x3fc3 write-lock try + G1 (line 0x2ac); 0x3fc6-0x3fe0 record null / not kinematic -> E1 arm; 0x3fe6-0x3ffa copy orientation to a local; t = rec+0x50..+0x58 (x kept on x87 stack from 0x3fed, y,z copied as ints); 0x3ffc-0x4096 position = M * c + t; 0x409d-0x421e R = M * L; 0x4225-0x43c9 same quaternion extraction as 000124; 0x43cb-0x43eb 000784(&pos, &quat), unlock; 0x43ee-0x4425 E1 arm.
  Orders here differ from 000124: px = ((M01*c1 + M02*c2) + M00*c0) + t.x (unrounded); py = float((M11*c1 + M12*c2) + M10*c0) + t.y; pz = float((M21*c1 + M22*c2) + M20*c0) + t.z. R[r][0] = (M[r][0]*L00 + M[r][1]*L10) + M[r][2]*L20; R[r][1] = (M[r][1]*L11 + M[r][2]*L21) + M[r][0]*L01; R[r][2] = (M[r][1]*L12 + M[r][2]*L22) + M[r][0]*L02.
- defects:
  - whole body: oracle holds one write lock and computes pose*{L,c} with its own product orders (different from 000124), so delegating to moveGlobalPose cannot be bit-faithful even once 000124 is fixed; candidate composes nothing.
  - candidate reads the position outside the write lock (getGlobalPositionVal, no guard) and then locks inside moveGlobalPose; any G1/E1 would report moveGlobalPose's lines instead of 0x2ac/0x2ae.
  - 0x43ee-0x4413 E1: kind 1, line 0x2ae, "Actor::moveGlobalOrientation: Actor must be kinematic!"; candidate silent.
  - 000784 flag OR / wake as in 000124.

#### phys_fn_000128 (0x4430, 333 B) -- getGlobalPoseReference (slot 9)
- candidate: NpActorVtable::getGlobalPoseReference Physics/src/NpActor.cpp:1100; nxNpActorRotationFromQuaternionGetter :970
- status: implemented
- verdict: faithful
- blocks checked: 0x4430-0x443d read-lock enter on [actor+0x10] (002362); 0x4442-0x4479 one-shot static byte [0x101237c0] set then FoundationSDK::error(0xd0, NpActor.cpp, 0x2c0, 0, "Warning: deprecated method: Actor::getGlobalPoseReference().  Please use getGlobalPose() instead.\n") -- candidate same code/line/text; 0x447c-0x4484 record = [body+8], null skips the refresh; 0x448a-0x454f quaternion (w,x,y,z loaded from +0x68,+0x5c,+0x60,+0x64) to row-major matrix -- the candidate's inline asm reproduces every instruction (same fld/fmul/fadd/fst/fsub order and spill points: yy, yz, xz, diagonal, xw temps; 1.0f operand); 0x4553-0x456b rotation copied to body+0x20..+0x40, record+0x50..+0x58 to body+0x44..+0x4c; 0x456c-0x457c unlock, return body+0x20.
- defects: none.
- notes: oracle copies position x,y through fld/fstp and z as an integer move; candidate memcpy (bit-identical for all non-signalling values).

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000054 addForceAtPos | implemented | defect | E1 line 0x12b missing; lever.x rounded (000791 keeps it on x87); 000782 x87 orders/spills for NX_FORCE/SMOOTH linear and all angular rows; mode>4 still wakes; H1 |
| 000056 addForce | implemented | defect | E1 line 0x14e missing; 000782 mode 0/3 linear x unrounded in oracle; mode>4 wake; H1 |
| 000058 addTorque | implemented | defect | E1 line 0x161 missing; angular row order (I2*z+I1*y)+I0*x with unrounded rows vs candidate order/rounding; mode>4 wake; H1 |
| 000070 createShape | partial | defect | E1 line 0x1ac + reentry guard missing; empty-actor and existing-group cases return 0; promotion lacks scene remove/add, group+8, 001041 arrays |
| 000072 releaseShape | partial | defect | internal-vs-public key; shift instead of swap-remove; no +0xdc/+0x10c updates; no single-shape or empty-group release; E1 lines 0x18f/0x19c/0x19d/0x1a4 + guard 0x186 missing |
| 000090 moveGlobalPosition | partial | defect | oracle adds rec+0x100 vector to target; 000784 wake missing; E1 line 0x2a1 missing |
| 000122 setDynamic | missing | defect | empty body; oracle validates desc (0x63/0x66), builds record via 000026, tears down old record, re-adds shape (0x7c/0x7d errors) |
| 000124 moveGlobalPose | partial | defect | no CMass-frame composition (rec+0xdc matrix, +0x100 vector) or oracle quat extraction; flags assigned =3 not ORed; no wake; E1 line 0x291 |
| 000126 moveGlobalOrientation | partial | defect | delegates to getGlobalPositionVal+moveGlobalPose; oracle single-lock inline composition with its own product orders; E1 line 0x2ae; no wake |
| 000128 getGlobalPoseReference | implemented | faithful | -- |

### NpActor.cpp listing review -- GROUP B

Reviewer notes that apply to several rows (cited by tag below):

- ROT: every row in this group builds the body rotation R from the record quaternion
  (x,y,z,w) at +0x5c with one and the same x87 instruction sequence (compared after
  normalising registers and stack slots; e.g. 0x10005e73-0x10005f36 in 000150). It keeps the
  doubled products in registers and spills five of them to float (2yy, 2xz, 2yw, 1-2xx, 2yz).
  Two candidate helpers reproduce this exactly: the inline-asm `nxNpActorRotationFromQuaternionGetter`
  (NpActor.cpp:970, checked instruction by instruction) and the C model
  `nxNpActorComposeRotation` (NpActorDynamicMath.h:242). Two do not:
  `nxNpActorRotationFromQuaternion` / `...At` (NpActorDynamicMath.h:18-38): all terms in
  double, no spills. And `NxMat33(NxQuat)` = `NxMat33::fromQuat` (Foundation/include/NxMat33.h:824):
  float expressions. The candidate build is SSE (see the candidate disassembly of
  nxNpActorRotateLocalForce at 0x10011720: mulss/addss), so every product and sum is rounded
  to float.
- RF: the listing forms W = R*F (F = mass-frame 3x3 at +0xdc, row-major f0..f8) with a
  different operand order in each row. Each element is rounded to float once:
  - 134/142: c0 (a0f0+a1f3)+a2f6, c1 (a1f4+a2f7)+a0f1, c2 (a0f2+a1f5)+a2f8 (all rows)
  - 138:     c0 (a0f0+a1f3)+a2f6, c1 (a0f1+a1f4)+a2f7, c2 (a1f5+a2f8)+a0f2 (all rows)
  - 140:     c0 (a1f3+a2f6)+a0f0, c1 (a0f1+a1f4)+a2f7, c2 (a0f2+a1f5)+a2f8 (all rows)
  - 144:     c0 (a1f3+a2f6)+a0f0, c1 (a0f1+a1f4)+a2f7, c2 row0 (a1f5+a2f8)+a0f2 but
             rows 1-2 (a0f2+a1f5)+a2f8
  (a = the R row). The candidate has one model, `nxNpActorDerivedMassFrame` (NpActor.cpp:1538,
  via asm `nxNpActorX87Dot3`). It equals the 138 pattern only. The tensor getters use
  `NxMat33::multiply` (NxMat33.h:1118): source order (a0f0+a1f3)+a2f6 for every element,
  compiled to SSE float.
- T746: rows 140/142/144 do not form the world tensor inline. They call the cdecl helper
  phys_fn_000746 (0x16e80) as (diag, W, out). The candidate already models that helper as
  `nxNpActorWorldTensorRDRt` (NpActorDynamicMath.h:213), but `nxNpActorInstantTensor`
  (NpActor.cpp:54) calls `nxNpActorWorldTensor` (NpActorDynamicMath.h:40) instead. That helper
  rounds all nine d*R products and sums xx+zz+yy, where 000746 keeps four products in
  registers and uses a different order. The math is the same (R D R^T); the rounding and
  order are not.
- FAP: rows 154/156/158 pass their (force, worldPos) to phys_fn_000791 (0x1a2c0, 133 B, the body's
  addForceAtPos(force, pos, mode, wake=1)). Disassembled from the oracle DLL, 000791 does this:
  - lever: lx = pos.x - [+0x158] stays in a register; ly and lz are spilled to float.
  - torque, each component rounded to float: tx = ly*fz - lz*fy, ty = lz*fx - lx*fz,
    tz = lx*fy - ly*fx.
  - one call: 000782(force, &torque, mode, wake).

  The candidate's `nxNpActorForceAtPos` (NpActor.cpp:1962) differs in two ways:
  - it rounds lx to float, and does the cross product in SSE float;
  - it calls `nxNpActorAccumulateForce` twice (linear, then angular) instead of once with both
    pointers.

  This belongs to row 000054 (addForceAtPos). It is flagged here because 154/156/158 run
  through the same helper.
- A782: rows 160/162 call phys_fn_000782 (0x18730, 3428 B) directly: (&v, 0, mode, 1) for a force,
  (0, &v, mode, 1) for a torque. That is the same callee and argument shape as addForce/addTorque
  (000056/000058). The candidate models it with `nxNpActorAccumulateForce` (NpActor.cpp:2039).
  Whether that model is faithful is for the 000056/000058 review; it was not re-derived here.

#### phys_fn_000134 (0x47d0, 907 B) -- getCMassGlobalPoseVal (slot 32)
- candidate: NpActorVtable::getCMassGlobalPoseVal Physics/src/NpActor.cpp:1644; helpers nxNpActorDerivedMassFrame :1538, nxNpActorX87Dot3 :1433, nxNpActorX87MassPositionX :1455, nxNpActorRotationFromQuaternionGetter :970, nxNpActorCMassMatrix/Position :1599/:1606
- status: implemented
- verdict: defect
- blocks checked:
  - 0x47d0-0x47ef: read-lock 002362 on [actor+0x10]; record = [[actor+0x14]+8]; null test.
  - 0x47f1-0x4858: null arm: E1 error, then identity M and zero t written to the hidden return, unlock, `ret 4`.
  - 0x485b-0x4920: ROT (matches Getter).
  - 0x4922-0x49bf: world CMass position: x = tx + ((R01 p1 + R02 p2) + R00 p0) kept in a register; y and z sums spilled to float, then ty+cy in a register and tz+cz spilled.
  - 0x49c3-0x4b34: RF, 134 pattern.
  - 0x4b38-0x4b58: copy the 9 floats, store the position, unlock, return.
- defects:
  - 0x49c3-0x4b34: RF columns 1 and 2 in the oracle are c1 (a1f4+a2f7)+a0f1 and c2 (a0f2+a1f5)+a2f8. The candidate (DerivedMassFrame, the 138 pattern) uses c1 (a0f1+a1f4)+a2f7 and c2 (a1f5+a2f8)+a0f2. Every orientation element in columns 1 and 2 differs in x87 operand order. Column 0 matches.
  - 0x47f1-0x480e: E1. The oracle reports kind 1, line 0x30a (778), "Actor::getCMassGlobalPose: Cannot be called on a static actor!" (0x10104ccc). The candidate returns the identity pose silently. The identity/zero output itself matches.
- notes:
  - The position arm matches exactly (X87MassPositionX for x; Dot3(a1,l1,a2,l2,a0,l0) then float(t+disp) for y and z).
  - The candidate `memcpy`s 0x260 bytes of the record into a scratch buffer. The oracle does not. This has no visible effect, but it would over-read if the record were shorter than 0x260.
  - The candidate also null-checks [actor+0x14]; the oracle dereferences it unconditionally.

#### phys_fn_000136 (0x4b60, 498 B) -- getCMassGlobalPositionVal (slot 33)
- candidate: NpActorVtable::getCMassGlobalPositionVal NpActor.cpp:1657 (same helpers as 000134)
- status: implemented
- verdict: defect (E1 only; the arithmetic is faithful)
- blocks checked:
  - 0x4b60-0x4b79: read lock and null test.
  - 0x4b7b-0x4bd1: null arm: E1, then copies the .data vector at 0x10123c1c (0,0,0) to the result, unlock.
  - 0x4bd4-0x4c9f: ROT.
  - 0x4ca1-0x4d4f: position. x = tx + ((R01p1+R02p2)+R00p0) in a register; cy and cz spilled to float; ty+cy and tz+cz rounded at the store. Candidate X87MassPositionX / X87Dot3 reproduce all three.
- defects:
  - 0x4b7b-0x4b9a: E1. The oracle reports kind 1, line 0x314 (788), "Actor::getCMassGlobalPosition: Cannot be called on a static actor!" (0x10104d10). The candidate returns (0,0,0) silently.
- notes:
  - The candidate also computes the unused world orientation and does the 0x260-byte scratch copy (see 000134); neither has a visible effect.
  - The oracle's null result comes from a .data global (0x10123c1c, zero in the image); the candidate uses a literal zero.

#### phys_fn_000138 (0x4d60, 658 B) -- getCMassGlobalOrientationVal (slot 34)
- candidate: NpActorVtable::getCMassGlobalOrientationVal NpActor.cpp:1669 (DerivedMassFrame)
- status: implemented
- verdict: defect (E1 only; the arithmetic is faithful)
- blocks checked:
  - 0x4d60-0x4d7b: read lock and null test.
  - 0x4d7d-0x4daa: null arm: E1, then source = the .data identity at 0x10122078.
  - 0x4daf-0x4e76: ROT.
  - 0x4e78-0x4fce: RF, 138 pattern. This matches DerivedMassFrame's X87Dot3 arguments exactly.
  - 0x4fd2-0x4fef: copy 9 floats, unlock, return.
- defects:
  - 0x4d7d-0x4d9c: E1. The oracle reports kind 1, line 0x31d (797), "Actor::getCMassGlobalOrientation: Cannot be called on a static actor!" (0x10104d58). The candidate returns identity silently.
- notes: DerivedMassFrame is evidently modelled on this row. Rows 134 and 142 use a different RF order.

#### phys_fn_000140 (0x5000, 706 B) -- getGlobalInertiaTensorVal (slot 39)
- candidate: NpActorVtable::getGlobalInertiaTensorVal NpActor.cpp:1741 -> nxNpActorInstantTensor(record, 0x18c, false) :54
- status: implemented
- verdict: defect
- blocks checked:
  - 0x5000-0x501e: read lock and null test.
  - 0x5020-0x504c: null arm: E1, then source = identity 0x10122078.
  - 0x5051-0x5115: ROT, copied to esp+0x24.
  - 0x5117-0x527e: RF, 140 pattern, rounded to float and copied back.
  - 0x5280-0x5295: call 000746(record+0x18c, W, tmp).
  - 0x5298-0x52bf: copy the result, unlock, return.
- defects:
  - 0x5051-0x5115: the candidate builds R with nxNpActorRotationFromQuaternion (double, no spills) rather than ROT (5 float spills). Use ComposeRotation or the Getter.
  - 0x5117-0x527e: the candidate uses NxMat33::multiply (source order (a0fk+a1fk+3)+a2fk+6, compiled as SSE float with every product and sum rounded). The oracle is x87 with one rounding per element in the 140 pattern: c0 (a1f3+a2f6)+a0f0.
  - 0x5280-0x5295: the candidate uses nxNpActorWorldTensor instead of the 000746 model nxNpActorWorldTensorRDRt (T746).
  - 0x5020-0x503e: E1. The oracle reports kind 1, line 0x32f (815), "Actor::getGlobalInertiaTensorVal: Cannot be called on a static actor!" (0x10104da0). The candidate returns identity silently.
- notes: callee 000746 (0x16e80, cdecl, 3 args, `add esp,0xc`).

#### phys_fn_000142 (0x52d0, 691 B) -- getGlobalInertiaTensorInverseVal (slot 40)
- candidate: NpActorVtable::getGlobalInertiaTensorInverseVal NpActor.cpp:1751 -> nxNpActorInstantTensor(record, 0xc4, false)
- status: implemented
- verdict: defect
- blocks checked:
  - 0x52d0-0x52dd: record = [[this+0x14]+8] and null test, with NO lock call.
  - 0x52df-0x5320: null arm: E1, identity 0x10122078 copied to the result, `ret 4`. No unlock anywhere.
  - 0x5323-0x53e7: ROT.
  - 0x53e9-0x5550: RF, 134 pattern.
  - 0x5552-0x5567: call 000746(record+0xc4, W, tmp).
  - 0x556a-0x5580: copy, return.
- defects:
  - 0x52d0 (whole row): the oracle takes no lock (no 002362/002366). The candidate enters and leaves the read guard on [actor+0x10] (nxNpSceneGuardEnter/Leave). That is extra behaviour.
  - 0x5323-0x53e7: R built without the ROT spills (as 000140).
  - 0x53e9-0x5550: NxMat33::multiply (SSE float, source order) versus the oracle's x87 134 pattern.
  - 0x5552-0x5567: nxNpActorWorldTensor versus the 000746 model (T746).
  - 0x52df-0x52fd: E1. The oracle reports kind 1, line 0x338 (824), "Actor::getGlobalInertiaTensorInverseVal: Cannot be called on a static actor!" (0x10104de8). The candidate returns identity silently.
- notes: the diagonal is +0xc4 (inverse inertia) and the candidate passes the right offset.

#### phys_fn_000144 (0x5590, 849 B) -- getAngularMomentumVal (slot 53)
- candidate: NpActorVtable::getAngularMomentumVal NpActor.cpp:1935 -> nxNpActorInstantTensor(record, 0x18c, true)
- status: implemented
- verdict: defect
- blocks checked:
  - 0x5590-0x55ad: read lock and null test.
  - 0x55af-0x5609: null arm: E1, result = .data zero vector 0x10123c1c, unlock.
  - 0x560c-0x56d1: ROT.
  - 0x56d3-0x583b: RF, 144 pattern.
  - 0x583d-0x5851: call 000746(record+0x18c, W, T).
  - 0x5854-0x58ca: L = T * w, with w = record+0x78: each component (T[i1]*wy + T[i2]*wz) + T[i0]*wx kept in x87 and stored as float.
  - 0x58cd-0x58de: unlock, return.
- defects:
  - 0x560c-0x56d1: the candidate uses NxMat33(NxQuat) (fromQuat in SSE float). The listing is the same ROT sequence as every other row. The comment at NpActor.cpp:58-59 ("The momentum getter stores quaternion products as floats before its matrix multiply") is not supported by the listing.
  - 0x56d3-0x583b: NxMat33::multiply (SSE float) versus the 144 RF pattern (x87, one rounding; note the asymmetric row-0 column 2).
  - 0x583d-0x5851: nxNpActorWorldTensor versus the 000746 model (T746).
  - 0x5854-0x58ca: the candidate computes t0*v0 + t1*v1 + t2*v2 in float (x, y, z order, SSE). The oracle computes (t1*vy + t2*vz) + t0*vx at x87 precision and rounds once at the store.
  - 0x55af-0x55cd: E1. The oracle reports kind 1, line 0x35a (858), "Actor::getAngularMomentumVal: Cannot be called on a static actor!" (0x10104e38). The candidate returns (0,0,0) silently.
- notes: the candidate reads the velocity at +0x78 and the diagonal at +0x18c, both correct.

#### phys_fn_000150 (0x5e70, 298 B) -- non-vtable helper: rotate a local vector by the body orientation (thiscall on the record, (out, v), `ret 8`, returns out)
- candidate: nxNpActorRotateLocalForce NpActor.cpp:2104 (called by addLocalForce :2124, addLocalTorque :2145, addLocalForceAtPos :2014, addLocalForceAtLocalPos :2025)
- status: implemented (inlined as a static helper, not a separate thiscall)
- verdict: defect
- blocks checked:
  - 0x5e70-0x5f3a: ROT from record+0x5c, 9 floats copied to a local.
  - 0x5f3c-0x5f97: out[i] = (R[i][1]*v.y + R[i][2]*v.z) + R[i][0]*v.x for i = 0,1,2, all kept in x87 and rounded only at the store.
- defects:
  - 0x5e73-0x5f36: the candidate builds R with `NxMat33(quaternion)` (fromQuat in SSE float: every product and difference rounded to float, no x87 intermediates). The oracle uses the ROT sequence (x87 intermediates, five specific float spills). Replace with nxNpActorComposeRotation or the Getter.
  - 0x5f3c-0x5f8a: the candidate sums (m0*x + m2*z) + m1*y. The oracle sums (m1*y + m2*z) + m0*x, identical for all three rows. Both keep the sum in extended/double until the float store; only the order differs.
- notes: callers 000156/000158 (force), 000160 (force) and 000162 (torque) all pass ecx = [[actor+0x14]+8], the same pointer as nxNpActorRecord.

#### phys_fn_000152 (0x5fa0, 346 B) -- non-vtable helper: local point to global (pose t at +0x50 plus R*p; thiscall (out, p), `ret 8`, returns out)
- candidate: nxNpActorLocalPosition NpActor.cpp:1975 (called by addForceAtLocalPos :2003 and addLocalForceAtLocalPos :2025)
- status: implemented (inlined static helper)
- verdict: defect
- blocks checked:
  - 0x5fa0-0x606b: ROT.
  - 0x606d-0x60f7: position:
    - x = tx + ((R01 p.y + R02 p.z) + R00 p.x), with the sum kept in a register;
    - y: cy = (R11 p.y + R12 p.z) + R10 p.x, spilled to float; then ty + cy;
    - z: cz = (R22 p.z + R20 p.x) + R21 p.y, spilled to float (a different order from the other rows); then tz + cz.
- defects:
  - 0x5fa5-0x6067: the candidate uses nxNpActorRotationFromQuaternion (double, no spills). The oracle uses ROT with five float spills.
  - 0x606d-0x60b2 (x): the candidate computes ((R00x + R02z) + R01y) + tx. The oracle computes ((R01y + R02z) + R00x) + tx.
  - 0x609c-0x60b4 (y): the candidate computes ((R10x + R12z) + R11y) + ty in double with no rounding. The oracle rounds (R11y + R12z) + R10x to float before adding ty.
  - 0x60b8-0x60d4 (z): the candidate computes ((R20x + R22z) + R21y) + tz with no spill. The oracle computes (R22z + R20x) + R21y, rounds it to float, then adds tz.
- notes: the phase5-public-actor-force evidence already cites FUN_10005fa0 for this rotation, but the current candidate does not follow the listing's order or spills.

#### phys_fn_000154 (0x6100, 201 B) -- addForceAtLocalPos (slot 55)
- candidate: NpActorVtable::addForceAtLocalPos NpActor.cpp:2003
- status: implemented
- verdict: defect (G1, E1, plus the helper float order)
- blocks checked:
  - 0x6100-0x6110: write-try 002364 on [actor+0xc].
  - 0x6112-0x613d: G1, line 0x131.
  - 0x6140-0x6156: record null test (`je`) and kinematic test (`test byte [rec+0x10c]; js`), both going to 0x6192.
  - 0x6158-0x617e: 000152(tmp, pos) on the record, then 000791(force, tmp, mode, 1).
  - 0x6183-0x618f: unlock, `ret 0xc`.
  - 0x6192-0x61c6: error, unlock.
- defects:
  - 0x6192-0x61b1: E1 for both a null record and a kinematic actor. The oracle reports kind 1, line 0x132 (306), "Actor::addForceAtLocalPos: Actor must be (non-kinematic) dynamic!" (0x10104e80). The candidate skips silently.
  - 0x6171: the position helper's float-order defects (see 000152).
  - 0x617e: the 000791 model's lever and cross-product precision and its two-call accumulation (FAP; owner 000054).
- notes:
  - The candidate's `& 0x80` on the dword at +0x10c tests the same bit as the oracle's signed byte.
  - The order is right: write-try, then precondition, then unlock.
  - G1 line 0x131 (305).

#### phys_fn_000156 (0x61d0, 201 B) -- addLocalForceAtPos (slot 56)
- candidate: NpActorVtable::addLocalForceAtPos NpActor.cpp:2014
- status: implemented
- verdict: defect (G1, E1, plus the helper float order)
- blocks checked:
  - 0x61d0-0x620d: write-try and G1 (line 0x13b).
  - 0x6210-0x6226: null and kinematic tests.
  - 0x6228-0x624e: 000150(tmp, force), then 000791(tmp, pos, mode, 1).
  - 0x6253-0x625f: unlock.
  - 0x6262-0x6296: error, unlock.
- defects:
  - 0x6262-0x6281: E1. The oracle reports kind 1, line 0x13c (316), "Actor::addLocalForceAtPos: Actor must be (non-kinematic) dynamic!" (0x10104ec8). The candidate is silent.
  - 0x6246: the rotation helper's defects (see 000150).
  - 0x624e: FAP.
- notes: G1 line 0x13b (315).

#### phys_fn_000158 (0x62a0, 214 B) -- addLocalForceAtLocalPos (slot 57)
- candidate: NpActorVtable::addLocalForceAtLocalPos NpActor.cpp:2025
- status: implemented
- verdict: defect (G1, E1, plus the helper float order)
- blocks checked:
  - 0x62a0-0x62dd: write-try and G1 (line 0x144).
  - 0x62e0-0x62f6: null and kinematic tests.
  - 0x62f8-0x632b: 000152(tmpA, pos) first, then 000150(tmpB, force), then 000791(tmpB, tmpA, mode, 1).
  - 0x6330-0x633c: unlock.
  - 0x633f-0x6373: error, unlock.
- defects:
  - 0x633f-0x635e: E1. The oracle reports kind 1, line 0x145 (325), "Actor::addLocalForceAtLocalPos: Actor must be (non-kinematic) dynamic!" (0x10104f10). The candidate is silent.
  - 0x6311 and 0x6323: the helper defects (see 000152 and 000150).
  - 0x632b: FAP.
- notes:
  - G1 line 0x144 (324).
  - The evaluation order (position before force) is not observable, because both helpers are pure.

#### phys_fn_000160 (0x6380, 198 B) -- addLocalForce (slot 59)
- candidate: NpActorVtable::addLocalForce NpActor.cpp:2124
- status: implemented
- verdict: defect (G1, E1, plus the helper float order)
- blocks checked:
  - 0x6380-0x63bd: write-try and G1 (line 0x156); `ret 8`.
  - 0x63c0-0x63d6: null and kinematic tests.
  - 0x63d8-0x63fb: 000150(tmp, force), then 000782(&tmp, 0, mode, 1).
  - 0x6400-0x640c: unlock.
  - 0x640f-0x6443: error, unlock.
- defects:
  - 0x640f-0x642e: E1. The oracle reports kind 1, line 0x157 (343), "Actor::addLocalForce: Actor must be (non-kinematic) dynamic!" (0x10104f58). The candidate is silent.
  - 0x63e2: the rotation helper's defects (see 000150).
- notes: G1 line 0x156 (342). The 000782 call shape (force pointer, null torque, mode, wake 1) maps to nxNpActorAccumulateForce(..., angular=false) (A782).

#### phys_fn_000162 (0x6450, 198 B) -- addLocalTorque (slot 61)
- candidate: NpActorVtable::addLocalTorque NpActor.cpp:2145
- status: implemented
- verdict: defect (G1, E1, plus the helper float order)
- blocks checked:
  - 0x6450-0x648d: write-try and G1 (line 0x169).
  - 0x6490-0x64a6: null and kinematic tests.
  - 0x64a8-0x64cb: 000150(tmp, torque), then 000782(0, &tmp, mode, 1).
  - 0x64d0-0x64dc: unlock.
  - 0x64df-0x6513: error, unlock.
- defects:
  - 0x64df-0x64fe: E1. The oracle reports kind 1, line 0x16a (362), "Actor::addLocalTorque: Actor must be (non-kinematic) dynamic!" (0x10104f98). The candidate is silent.
  - 0x64b2: the rotation helper's defects (see 000150).
- notes: G1 line 0x169 (361). The 000782 shape (null force, torque pointer) maps to AccumulateForce(..., angular=true) (A782).

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000134 | implemented | defect | RF columns 1 and 2 use the 138 order instead of the oracle's (a1f4+a2f7)+a0f1 / (a0f2+a1f5)+a2f8; E1 line 0x30a |
| 000136 | implemented | defect | E1 only (line 0x314 "getCMassGlobalPosition: Cannot be called on a static actor!"); arithmetic faithful |
| 000138 | implemented | defect | E1 only (line 0x31d); ROT and RF match DerivedMassFrame exactly |
| 000140 | implemented | defect | R without ROT spills; NxMat33::multiply (SSE float, wrong order) versus the x87 140 RF; WorldTensor versus the 000746 model; E1 line 0x32f |
| 000142 | implemented | defect | oracle takes NO lock but the candidate enters the read guard; R, RF and 000746 as in 000140 (134 RF pattern); E1 line 0x338 |
| 000144 | implemented | defect | fromQuat float R (and the comment at :58 is wrong); NxMat33::multiply versus the 144 RF; WorldTensor versus 000746; T*w in float x,y,z versus x87 (t1y+t2z)+t0x; E1 line 0x35a |
| 000150 | implemented | defect | R via fromQuat (SSE float) not ROT; rows summed (m0x+m2z)+m1y versus the oracle's (m1y+m2z)+m0x |
| 000152 | implemented | defect | R without ROT spills; x/y/z order differs; oracle spills cy and cz (z order (R22z+R20x)+R21y) before adding t |
| 000154 | implemented | defect | G1 (0x131); E1 line 0x132 for null or kinematic; inherits the 000152 and FAP helper defects |
| 000156 | implemented | defect | G1 (0x13b); E1 line 0x13c; inherits the 000150 and FAP defects |
| 000158 | implemented | defect | G1 (0x144); E1 line 0x145; inherits the 000152, 000150 and FAP defects |
| 000160 | implemented | defect | G1 (0x156); E1 line 0x157; inherits the 000150 defect (000782 model reviewed with 000056) |
| 000162 | implemented | defect | G1 (0x169); E1 line 0x16a; inherits the 000150 defect (000782 model reviewed with 000058) |

### NpActor.cpp listing review -- group C1

Constants used below (read from build/pairs/oracle/NxPhysics.dll .rdata):
`[0x101041ec]` = 1.0f, `[0x101041f0]` = 0.0f, `[0x101053d4]` = 0x3ecccccc (0.39999998f).
`0x100f4140` = phys_fn_005666 `__fpclass` (inventory label); `test eax,0x207` = SNAN|QNAN|NINF|PINF.
"Dirty push" below means the inlined scene dirty-list mark on `list = [rec+0x120]+0x40`, index `id = [rec+0x11c]`:
if `flags=[list+0]`, `flags[id]==0` then `[list+0x20][id] = (end-begin)/4`, grow the vector `begin/end/cap = list+0x10/0x14/0x18`
when `cap <= end` to `2*count+2` entries via allocator `[[0x101041bc]]` vtbl+8 (alloc(bytes,0)) / vtbl+0x14 (free), append `id`;
then `flags[id] |= mask`. Candidate equivalent: `nxNpActorMarkRecordDirty` Physics/src/NpActor.cpp:91.

Helper observation (applies to every row here, not re-listed per row): `nxNpActorMarkRecordDirty` (NpActor.cpp:91-125)
has guards the oracle does not -- it returns without setting the flag when `[rec+0x120]` or `flags` is null, when
`id >= 256`, or when the vector `begin`/`end`/`cap` is null. The oracle handles a null `begin` by allocating
(`test ecx,ecx; jne; xor edx,edx` -> capacity 0 -> grow). With a null vector the candidate drops the dirty bit
entirely. Growth is the same size (2*count+2) and copy/free order.

#### phys_fn_000164 (0x6520, 1846 B) -- updateMassFromShapes(NxReal density, NxReal totalMass)
- candidate: NpActorVtable::updateMassFromShapes Physics/src/NpActor.cpp:1220 (empty body, commented "(unimplemented)")
- status: missing (entire body)
- verdict: defect (nothing implemented)
- blocks checked (frame: `sub esp,0x50`; `this`=esi; after `push ebp; push ebx` density=[esp+0x60], totalMass=[esp+0x64]):
  - 0x6520-0x655d write-lock guard: `phys_fn_002364([actor+0xc])`; al==0 -> instance assert, FoundationSDK::error(kind 2, NpActor.cpp, line 0x98=152, 0, "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!"), return (no unlock). (G1)
  - 0x6560-0x658c argument sign check, BEFORE the dynamic check: `fld density; fcomp 0.0; test ah,1; jne` and the same for totalMass -> if density < 0 or totalMass < 0 (also NaN, C0 set when unordered) goto 0x6c1f.
  - 0x6c1f-0x6c53: error kind 1, line 0x9a=154, msg 0x10104fd8 "Actor::updateMassFromShapes: density and total Mass of a shape have to be nonnegative!"; unlock `phys_fn_002366([actor+0xc])` (ebp); return.
  - 0x6592-0x65d2 dynamic check: `core=[actor+0x14]; rec=[core+8]` (ebx); rec==0 -> error kind 1, line 0x9d=157, msg 0x10105204 "Actor::updateMassFromShapes: Actor must be dynamic!"; unlock; return. Shared error tail 0x65b5-0x65d2 (push file, push 1, call error, unlock, return) is reused by all later kind-1 errors.
  - 0x65d5-0x65f4 shapes check: `[core+0x10]==0` -> line 0x9e=158, msg 0x101051cc "Actor::updateMassFromShapes: Actor must have shapes!" (via tail 0x65b5).
  - 0x65f6-0x6673 exactly-one-nonzero check (`fucompp` vs 0.0, `test ah,0x44; jp` = not-equal):
    - density==0 && totalMass==0 -> line 0x9f=159, msg 0x10105188 "Actor::updateMassFromShapes: density or total mass must be nonzero!"
    - density!=0 && totalMass!=0 -> line 0xa0=160, msg 0x10105138 "Actor::updateMassFromShapes: density and total mass may not both be nonzero!"
    - otherwise fall to 0x6678. (NaN counts as "not equal" to 0.)
  - 0x6678-0x66f4 mass-properties call: locals are an NxMat34 `pose` at frame+0x2c (M = identity 1,0,0/0,1,0/0,0,1 at +0x2c..+0x4c, t = 0,0,0 at +0x50..+0x58) and an NxVec3 `diag` at frame+0x14 (uninitialised). Calls `phys_fn_000008` (0x10a0, __thiscall, purge 16) with **ecx = core ([actor+0x14])** and args `(density /*by value*/, &totalMass /*in-out: the stack arg slot*/, &pose, &diag)`.
  - 0x66f9-0x6736 result: eax==1 -> line 0xa8=168, msg 0x10105098 "Actor::updateMassFromShapes: Compute mesh inertia tensor failed for one of the actor's mesh shapes! Please change mesh geometry or supply a tensor manually!"; eax!=0 (else) -> line 0xa9=169, msg 0x10105030 "Actor::updateMassFromShapes: Can't compute mass from shapes: must have at least one non-trigger shape!". Both kind 1, then unlock/return via 0x65b5.
  - 0x673b-0x6772 mass: `[rec+0x188] = totalMass` (integer copy of the in-out slot, i.e. the mass 000008 wrote back); `fld m; fld 1.0; fdiv st(1)` -> `[rec+0xc0] = (float)(1.0/m)`; no zero/finite test on the mass.
  - 0x676b-0x6847 dirty push, mask 0x10000.
  - 0x6849-0x6902 inertia: `[rec+0x18c..0x194] = diag` (integer copy, before the inverse); invX/invY/invZ = `fld 1.0; fdiv diag.i; fstp float` each (spilled to float); `__fpclass((double)invX)`, then invY, then invZ; if any has a bit in 0x207 (NaN or +-Inf) all three become 0.0f, else the float-rounded inverses; stored `[rec+0xc4]=invX, [0xc8]=invY, [0xcc]=invZ`.
  - 0x6908-0x69e9 dirty push, mask 0x20000.
  - 0x69ef-0x6a07 `[rec+0x100..0x108] = pose.t` (mass-centre offset).
  - 0x6a0d-0x6af6 dirty push, mask 0x200; 0x6af8 `++[rec+0x198]`.
  - 0x6afe-0x6b0d `rep movsd` 9 dwords `pose.M` -> `[rec+0xdc..0xfc]` (raw NxMat33 storage, which is row-major `_11.._33`, i.e. what `getRowMajor` writes; matches how the candidate's setCMassOffsetLocalPose fills +0xdc).
  - 0x6b0f-0x6bf8 dirty push, mask 0x400; 0x6bfa-0x6c03 `++[rec+0x198]`.
  - 0x6c09 `call phys_fn_000768(ecx = rec)` (mass-frame refresh; candidate equivalent `nxNpActorRefreshCMass` -> `nxNpActorUpdateMassFrame`, NpActor.cpp:1238, NpActorDynamicMath.h:281).
  - 0x6c0e-0x6c1c unlock, `ret 8`.
- defects:
  - 0x6520-0x6c53 -> the candidate body is empty: no guard, no validation, no mass/inertia/pose computation, no record writes, no dirty bits, no +0x198 increments, no 000768 refresh.
- notes (for the implementing task):
  - No wake: unlike the velocity setters, this row never touches +0x84/+0x4c/+0x114 and never marks 0x10.
  - No kinematic test: only `rec != 0` is checked (not the 0x80 bit at +0x10c).
  - Order of checks: lock -> sign (0x9a) -> dynamic (0x9d) -> shapes (0x9e) -> both-zero (0x9f) / both-nonzero (0xa0) -> 000008 result (0xa8/0xa9). All errors except the lock one are kind 1 and unlock before returning.
  - Order of effects: 0x188, 0xc0, dirty 0x10000; 0x18c-0x194, 0xc4-0xcc, dirty 0x20000; 0x100-0x108, dirty 0x200, ++0x198; 0xdc-0xfc, dirty 0x400, ++0x198; 000768(rec); unlock.
  - The inverse-inertia block (0x6867-0x6902) is byte-for-byte the same idiom as setMassSpaceInertiaTensor (000168 0x6ecd-0x6f85); a shared helper would serve both (see the 000168 defect: the candidate's `> 0` test is not this idiom).
  - phys_fn_000008 (0x10a0, 751 B, inventory state `discovered`, gap `<start>..Actor.cpp`, no candidate) is a method on the core actor object: per the Ghidra hint it calls the vtable+0x10 of `[this+0x10]` (the shape/group object checked at 0x65d5) to gather mass, CoM and an inertia tensor (returns 1 when that returns false = mesh-inertia failure; returns 2 when the gathered mass <= 0 = no non-trigger shape), writes the CoM into pose.t, scales the tensor either by density (totalMass==0: writes `*totalMass = mass*density`) or by `totalMass/mass`, and ends with `NxDiagonalizeInertiaTensor(tensor, &diag, &pose.M)`. Callees 0x1c720, 0x1c880, 0x2ea70. It is unreconstructed; implementing 000164 needs it (or a listing-faithful equivalent) first.

#### phys_fn_000166 (0x6c60, 479 B) -- setMass(NxReal mass)
- candidate: NpActorVtable::setMass Physics/src/NpActor.cpp:1681
- status: implemented
- verdict: defect (E1 only; record effects faithful, G1)
- blocks checked:
  - 0x6c60-0x6c99 guard, G1 line 0xb9=185.
  - 0x6c9c-0x6ce0 `rec=[[actor+0x14]+8]`; rec==0 -> kind 1, line 0xba=186, msg 0x10105268 "Actor::setMass: Actor must be dynamic!"; unlock; return.
  - 0x6ce3-0x6d2f `fcomp 0.0; test ah,0x41; je` -> mass <= 0 or NaN -> kind 1, line 0xbb=187, msg 0x10105238 "Body::setMass: mass is %f, should be positive!" with `(double)mass` pushed as the vararg; unlock; return.
  - 0x6d32-0x6d5a `[rec+0x188]=mass` (integer copy), `[rec+0xc0]=(float)(1.0/mass)` (fld 1.0; fdiv mass) -- candidate same values/order.
  - 0x6d60-0x6e2f dirty push, mask 0x10000 -- candidate same.
  - 0x6e31-0x6e3c unlock, `ret 4`.
- defects:
  - 0x6cae-0x6ce0 -> oracle errors kind 1 line 186 "Actor::setMass: Actor must be dynamic!"; candidate returns silently (E1).
  - 0x6cf4-0x6d2f -> oracle errors kind 1 line 187 "Body::setMass: mass is %f, should be positive!" (with mass as double); candidate skips silently (E1). The candidate's `mass > 0.0f` matches the oracle's acceptance set including NaN rejection.
- notes: no wake, no +0x198, no 000768 call in the oracle; candidate adds none. Correct.

#### phys_fn_000168 (0x6e40, 577 B) -- setMassSpaceInertiaTensor(const NxVec3& m)
- candidate: NpActorVtable::setMassSpaceInertiaTensor Physics/src/NpActor.cpp:1707
- status: implemented
- verdict: defect
- blocks checked:
  - 0x6e40-0x6e7d guard, G1 line 0xc5=197.
  - 0x6e80-0x6ec6 rec==0 -> kind 1, line 0xc6=198, msg 0x10105290 "Actor::setMassSpaceInertiaTensor: Actor must be dynamic!"; unlock; return.
  - 0x6ec9-0x6eea `[rec+0x18c..0x194] = m` (integer copy) -- candidate same.
  - 0x6eed-0x6f85 inverse: `invI = (float)(1.0/m.i)` for x,y,z (each fdiv then fstp to a float local); `__fpclass((double)inv)` for x, then y, then z; any NaN/Inf -> all three 0.0f, else the three float inverses; store to +0xc4/+0xc8/+0xcc.
  - 0x6f8b-0x7069 dirty push, mask 0x20000 -- candidate same.
  - 0x7071-0x707e unlock, `ret 4`.
- defects:
  - 0x6f10-0x6f85 -> the oracle's test is "all three float-rounded inverses are finite", the candidate's is "m.x > 0 && m.y > 0 && m.z > 0". They differ:
    - a negative component (e.g. m = (-2,1,1)): oracle stores the negative inverses (-0.5,1,1); candidate zeroes all three.
    - -0.0 component: 1/-0 = -Inf -> oracle zeroes; candidate also zeroes (same, by accident).
    - a positive component small enough that 1/x overflows float (x < ~2.94e-39, denormals): oracle sees +Inf after the fstp-to-float and zeroes all three; candidate stores +Inf.
    - +Inf component: 1/Inf = 0 -> oracle stores (0, ...); candidate also stores 0 (same).
  - 0x6e92-0x6ec6 -> E1: kind 1, line 198, "Actor::setMassSpaceInertiaTensor: Actor must be dynamic!"; candidate silent.
- notes: the inverse computation itself (fdiv in x87 then single rounding at fstp) matches the candidate's `1.0f / m.x` value. The fix is to compute all three inverses unconditionally and zero only when `_fpclass` reports NaN/Inf (same idiom as 000164 0x6867-0x6902).

#### phys_fn_000170 (0x7090, 431 B) -- setLinearDamping(NxReal damping)
- candidate: NpActorVtable::setLinearDamping Physics/src/NpActor.cpp:1761
- status: implemented
- verdict: defect (E1 only; record effects faithful, G1)
- blocks checked:
  - 0x7090-0x70c9 guard, G1 line 0xd0=208.
  - 0x70cc-0x7117 value check FIRST (before the dynamic test): `fcomp 0.0; test ah,1; je` -> damping < 0 (or NaN) -> kind 1, line 0xd1=209, msg 0x101052d0 "Actor::setLinearDamping: The linear damping must be nonnegative!"; unlock; return.
  - 0x711a-0x713b rec==0 -> kind 1, line 0xd2=210, msg 0x101046bc "Actor::setLinearDamping: Actor must be dynamic!" (via shared tail 0x70fd).
  - 0x713d-0x714a `[rec+0xb8] = damping` -- candidate same.
  - 0x7150-0x722f dirty push, mask 0x800 -- candidate same.
  - 0x7231-0x723c unlock, `ret 4`.
- defects:
  - 0x70e5-0x7117 -> E1: kind 1, line 209, "Actor::setLinearDamping: The linear damping must be nonnegative!"; candidate silent.
  - 0x7124-0x713b -> E1: kind 1, line 210, "Actor::setLinearDamping: Actor must be dynamic!"; candidate silent. Note the oracle reports the value error even on a static actor (value check precedes the dynamic check); an E1 fix must keep that order.
- notes: candidate `damping >= 0.0f` has the same acceptance set (NaN rejected).

#### phys_fn_000172 (0x7240, 431 B) -- setAngularDamping(NxReal damping)
- candidate: NpActorVtable::setAngularDamping Physics/src/NpActor.cpp:1786
- status: implemented
- verdict: defect (E1 only; record effects faithful, G1)
- blocks checked: instruction-for-instruction the same shape as 000170 (diffed); differences only: G1 line 0xdf=223 (0x7240-0x7279), value check 0x727c-0x72c7 line 0xe0=224 msg 0x10105348, dynamic check 0x72ca-0x72eb line 0xe1=225 msg 0x10105314, store `[rec+0xbc]`, dirty mask 0x1000 (0x73d1-0x73df), unlock/ret 4.
- defects:
  - 0x7295-0x72c7 -> E1: kind 1, line 224, "Actor::setAngularDamping: The angular damping must be nonnegative!"; candidate silent.
  - 0x72d4-0x72eb -> E1: kind 1, line 225, "Actor::setAngularDamping: Actor must be dynamic!"; candidate silent. Same check order as 000170 (value before dynamic).
- notes: store offset (+0xbc) and mask (0x1000) match the candidate.

#### phys_fn_000174 (0x73f0, 770 B) -- setLinearVelocity(const NxVec3& v)
- candidate: NpActorVtable::setLinearVelocity Physics/src/NpActor.cpp:1811
- status: partial (the wake block 0x7566-0x76a7 is absent)
- verdict: defect
- blocks checked:
  - 0x73f0-0x7431 guard, G1 line 0xf3=243.
  - 0x7434-0x7452 `rec=[[actor+0x14]+8]`; rec==0 or `byte [rec+0x10c]` sign bit (kinematic 0x80) -> 0x76bc.
  - 0x76bc-0x76ef -> kind 1, line 0xf4=244, msg 0x10105390 "Actor::setLinearVelocity: Actor must be (non-kinematic) dynamic!"; unlock; return.
  - 0x7458-0x747d `[rec+0x6c..0x74] = v`, then `[rec+0x34..0x3c] = [rec+0x6c..0x74]` -- candidate same order.
  - 0x7480-0x7564 dirty push, mask 4 -- candidate same.
  - 0x7566-0x7595 wake test 1: `s = (v.y*v.y + v.z*v.z) + v.x*v.x` in x87 (loaded from the argument `[ebx]`, re-read after the stores), `fcomp [rec+0xd0]` (sleep-linear-velocity squared); `s < [rec+0xd0]` -> skip wake.
  - 0x759b-0x75a4 wake test 2: `[rec+0x114] & 0x100` set -> skip wake.
  - 0x75aa-0x75bb wake test 3: `fld [rec+0x84]; fcomp 0.39999998f; test ah,5; jp` -> proceed only if `[rec+0x84] < 0.39999998f` (NaN/>=/== skip).
  - 0x75c1-0x75db `[rec+0x84] = [rec+0x4c] = 0x3ecccccc`.
  - 0x75de-0x76a7 dirty push, mask 0x10.
  - 0x76a9-0x76b9 unlock, `ret 4`.
- defects:
  - 0x7566-0x76a7 -> the oracle wakes the body when `|v|^2 >= [rec+0xd0]`, the 0x100 bit of +0x114 is clear and `[rec+0x84] < 0.39999998f`: writes 0x3ecccccc to +0x84 and +0x4c and marks dirty 0x10. The candidate has no wake at all. The existing `nxNpActorWakeAfterCMassWrite` (NpActor.cpp:1243) reproduces tests 2-3 and the writes but not test 1 (the squared-speed threshold against +0xd0, summed in the order (y*y + z*z) + x*x, unrounded x87 sum compared with the float at +0xd0).
  - 0x7406-0x7431 -> G1.
  - 0x76bc-0x76ef -> E1: kind 1, line 244, "Actor::setLinearVelocity: Actor must be (non-kinematic) dynamic!" for a static OR kinematic actor; candidate silent.
- notes: candidate's `(dword [rec+0x10c] & 0x80) == 0` is equivalent to the oracle's `test cl,cl; js` on the low byte.

#### phys_fn_000176 (0x7700, 786 B) -- setAngularVelocity(const NxVec3& v)
- candidate: NpActorVtable::setAngularVelocity Physics/src/NpActor.cpp:1825
- status: partial (the wake block is absent)
- verdict: defect
- blocks checked (diffed against 000174; register allocation differs, behaviour differs only as listed):
  - 0x7700-0x7741 guard, G1 line 0xfc=252.
  - 0x7744-0x7762 rec==0 or kinematic (byte sign of +0x10c) -> 0x79dc: kind 1, line 0xfd=253, msg 0x101053d8 "Actor::setAngularVelocity: Actor must be (non-kinematic) dynamic!" (read from oracle .rdata; the bundle's string list omits it).
  - 0x7768-0x778d `[rec+0x78..0x80] = v`, then `[rec+0x40..0x48] = [rec+0x78..0x80]` -- candidate same.
  - 0x7790-0x7884 dirty push, mask 8 -- candidate same.
  - wake block (same layout as 000174 0x7566-0x76a7): `(v.y*v.y + v.z*v.z) + v.x*v.x` vs `fcomp [rec+0xd4]` (sleep-angular-velocity squared); `[rec+0x114] & 0x100`; `[rec+0x84] < 0.39999998f`; then `[rec+0x84] = [rec+0x4c] = 0x3ecccccc`, dirty 0x10; exits to unlock at 0x79c9.
  - 0x79c9-0x79d9 unlock, `ret 4`.
- defects:
  - wake block (0x7881-0x79c7) -> absent in the candidate (same as 000174 but thresholded on +0xd4).
  - 0x7716-0x7741 -> G1.
  - 0x79dc-0x7a0f -> E1: kind 1, line 253, "Actor::setAngularVelocity: Actor must be (non-kinematic) dynamic!"; candidate silent.
- notes: none beyond 000174.

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000164 updateMassFromShapes | missing | defect | Empty body; oracle: sign check (154), dynamic (157), shapes (158), exactly-one-nonzero (159/160), phys_fn_000008(core; density,&totalMass,&pose,&diag) with errors 168/169, then writes 0x188/0xc0 (d0x10000), 0x18c-0x194/0xc4-0xcc fpclass-zeroed (d0x20000), 0x100 (d0x200,++0x198), 0xdc (d0x400,++0x198), 000768(rec); no wake |
| 000166 setMass | implemented | defect (E1) | Missing errors kind 1 line 186 "Actor must be dynamic!" and line 187 "Body::setMass: mass is %f, should be positive!"; record effects faithful |
| 000168 setMassSpaceInertiaTensor | implemented | defect | Inverse gate is `all > 0` instead of `_fpclass` finite on the float inverses (negatives kept by oracle, denormal->Inf zeroed by oracle); plus E1 line 198 |
| 000170 setLinearDamping | implemented | defect (E1) | Missing errors line 209 (nonnegative, checked before dynamic) and line 210 (must be dynamic); record effects faithful |
| 000172 setAngularDamping | implemented | defect (E1) | Missing errors line 224 (nonnegative, checked before dynamic) and line 225 (must be dynamic); record effects faithful |
| 000174 setLinearVelocity | partial | defect | No wake: oracle wakes (0x84/0x4c=0x3ecccccc, dirty 0x10) when (y*y+z*z)+x*x >= [rec+0xd0], +0x114&0x100 clear, +0x84<0.4f; plus E1 line 244 |
| 000176 setAngularVelocity | partial | defect | Same missing wake thresholded on [rec+0xd4]; plus E1 line 253 |

### NpActor.cpp listing review -- group C2

Rows: 000178, 000180, 000182, 000184, 000186, 000188, 000190, 000192, 000194.
Oracle bytes checked against D:/vfy-base/Binaries/NxPhysics.dll (sha256 4b7db3e1..., the manifest image).
Constants read from the image: [0x101053d4] = 0x3ecccccc (0.39999998f), [0x101041ec] = 1.0f.
Error strings resolved from the image (all passed with file 0x10104690 and kind 1):
0x1010541c "Actor::setMaxAngularVelocity: Actor must be dynamic!",
0x10105454 "Actor::setLinearMomentum: Actor must be dynamic!",
0x10105488 "Actor::setAngularMomentum: Actor must be (non-kinematic) dynamic!",
0x101054cc "Actor::raiseBodyFlag: Actor must be dynamic!",
0x101054fc "Actor::clearBodyFlag: Actor must be dynamic!".

#### Shared helper finding (cite as H1)

Every row here marks the body record dirty with the same inlined sequence (the oracle has no
separate function for it): `aux = [rec+0x120]+0x40; id = [rec+0x11c]; if(!aux.flags[id]) { aux.index[id]
(= [aux+0x20]) = (end-begin)/4; if(!(cap > end)) { newcap = count*2+2; curcap = begin ? (cap-begin)/4 : 0;
if(curcap < newcap) { p = alloc->vt[+8](newcap*4, 0); copy; if(begin) alloc->vt[+0x14](begin); begin=p;
end=p+count; cap=p+newcap } } *end++ = id; } flags[id] |= mask`.
Candidate nxNpActorMarkRecordDirty (Physics/src/NpActor.cpp:91-124) matches the push/grow arithmetic
(count*2+2, index table at aux+0x60, persistent allocation, free old block) but adds early returns
the oracle does not have:
- `if(!active || !end || !capacity) return;` (line 104): when the active-id vector is unallocated
  (begin=end=cap=0) the oracle allocates a 2-entry block and pushes the id, then ORs the mask;
  the candidate returns before both the push AND `flags[id] |= mask`, so the dirty bit is lost.
- `id >= 256` (line 98): the oracle indexes flags[id]/index[id] with no bound; the candidate
  silently drops the mark for any body id >= 256.
- `!aux`, `!flags`, `!grown` returns: the oracle dereferences unconditionally (benign in valid states).
- Minor/unobservable: the oracle writes index[id] before growing; the candidate writes it after.
H1 is reported once here and cited per row; it is a real behavioural defect of the candidate path
for every row below.

#### phys_fn_000178 (0x7a20, 385 B) -- setMaxAngularVelocity (slot 49)
- candidate: NpActorVtable::setMaxAngularVelocity Physics/src/NpActor.cpp:1863 (+ nxNpActorMarkRecordDirty :91)
- status: implemented
- verdict: defect
- blocks checked: 0x7a20-0x7a59 write-lock try + G1 report (line 0x108); 0x7a5c-0x7aa0 null record ->
  error kind 1 line 0x109 + unlock; 0x7aa3-0x7abb `fld arg; fmul arg; fstp [rec+0xd8]` (matches
  `limit*limit`); 0x7abb-0x7b91 dirty mark 0x8000 (H1 sequence); 0x7b93-0x7b9e unlock, ret 4.
- defects:
  - 0x7a6c-0x7aa0 (E1): null body record -> FoundationSDK::error(1, NpActor.cpp, 0x109 (265), 0,
    "Actor::setMaxAngularVelocity: Actor must be dynamic!") then unlock; candidate skips silently.
  - H1 on the 0x8000 dirty mark.
- notes: G1 line 0x108 (264). No wake logic in this row. Store and mask otherwise identical.

#### phys_fn_000180 (0x7bb0, 782 B) -- setLinearMomentum (slot 50)
- candidate: NpActorVtable::setLinearMomentum Physics/src/NpActor.cpp:1876 (+ :91)
- status: partial -- the conditional wake-up block (0x7d62-0x7ea9) is absent
- verdict: defect
- blocks checked: 0x7bb0-0x7bf1 write-lock try + G1 (line 0x113); 0x7bf4-0x7c3a null record ->
  error kind 1 line 0x114 + unlock; 0x7c3d-0x7c80 invMass=[rec+0xc0] spilled to a stack temp,
  `invMass*m.x, *m.y, *m.z`, fstp to +0x6c/+0x70/+0x74 and x also fstp to +0x34, then +0x38/+0x3c
  copied as dwords from +0x70/+0x74 (candidate's float products + memcpy give the same bits);
  0x7c83-0x7d60 dirty 0x4 (H1); 0x7d62-0x7d92 wake test 1: s = (x*x + y*y) + z*z over +0x6c/+0x70/+0x74
  (x87 extended, not spilled), `fcomp [rec+0xd0]`, skip if s < thr or unordered (C0);
  0x7d98-0x7da7 wake test 2: skip if [rec+0x114] & 0x100; 0x7dad-0x7dbe wake test 3:
  `fld [rec+0x84]; fcomp [0x101053d4]; test ah,5; jp` -> proceed only if +0x84 < 0.39999998f (ordered);
  0x7dc4-0x7ea9 store 0x3ecccccc to +0x84 and +0x4c, dirty 0x10 (H1); 0x7eab-0x7ebb unlock, ret 4.
- defects:
  - 0x7bfc-0x7c3a (E1): null record -> error(1, NpActor.cpp, 0x114 (276), 0,
    "Actor::setLinearMomentum: Actor must be dynamic!"); candidate silent.
  - 0x7d62-0x7ea9: candidate has no wake-up at all. Oracle: if |v|^2 (x*x+y*y then +z*z, extended
    precision) >= [rec+0xd0] (sleep linear velocity^2) and !([rec+0x114] & 0x100) and [rec+0x84] < 0.4f,
    then [rec+0x84] = [rec+0x4c] = 0x3ecccccc and dirty |= 0x10.
  - H1 on masks 0x4 and 0x10.
- notes: G1 line 0x113 (275). No kinematic test in this row (candidate agrees). Bit 0x100 of +0x114
  is the same wake-suppress bit putToSleep sets (see JointSupport.cpp phys_fn_000760 comment).

#### phys_fn_000182 (0x7ec0, 867 B) -- setAngularMomentum (slot 51)
- candidate: NpActorVtable::setAngularMomentum Physics/src/NpActor.cpp:1893 (+ :91)
- status: partial -- wake-up block (0x808e-0x81d8) absent; error report absent
- verdict: defect
- blocks checked: 0x7ec0-0x7f01 write-lock try + G1 (line 0x11c); 0x7f04-0x7f22 `record==0 -> 0x81ed`,
  `test byte [rec+0x10c]; js 0x81ed` (kinematic); 0x7f28-0x7f8c three row dots over the inverse
  inertia I = +0x164..+0x184; 0x7f8e-0x7fad stores row0->+0x78, row1->+0x7c, row2->+0x80, row0 fstp
  also to +0x40, then +0x44/+0x48 dword-copied from +0x7c/+0x80; 0x7fb0-0x808c dirty 0x8 (H1);
  0x808e-0x80c1 wake test 1 on +0x78/+0x7c/+0x80 vs [rec+0xd4]; 0x80c7-0x80d6 +0x114 & 0x100;
  0x80dc-0x80ed +0x84 < 0.4f; 0x80f3-0x81d8 +0x84=+0x4c=0x3ecccccc, dirty 0x10 (H1);
  0x81da-0x81ea unlock ret 4; 0x81ed-0x8220 error kind 1 line 0x11d + unlock.
- defects:
  - 0x7f32-0x7f8c (x87 order): oracle sums are
    row0 = (I1*m.y + I2*m.z) + I0*m.x,
    row1 = (I4*m.y + I3*m.x) + I5*m.z,
    row2 = (I7*m.y + I6*m.x) + I8*m.z
    (first product loaded is always the +y term). Candidate source (lines 1901-1909) writes
    row0 = (I0*x + I2*z) + I1*y, row1 = (I5*z + I3*x) + I4*y, row2 = (I8*z + I6*x) + I7*y -- a different
    association in all three rows (and it routes through `double` casts the listing does not show;
    the oracle keeps every term on the x87 stack and rounds only at the fstp).
  - 0x81ed-0x8220 (E1): null record OR kinematic (+0x10c bit 0x80) -> error(1, NpActor.cpp,
    0x11d (285), 0, "Actor::setAngularMomentum: Actor must be (non-kinematic) dynamic!") then unlock;
    candidate skips silently (its kinematic test itself matches: `& 0x80`).
  - 0x808e-0x81d8: no wake-up in candidate. Oracle: if (x*x+y*y)+z*z over +0x78/+0x7c/+0x80 >=
    [rec+0xd4] (sleep angular velocity^2) and !(+0x114 & 0x100) and +0x84 < 0.4f, then +0x84 = +0x4c =
    0x3ecccccc, dirty |= 0x10.
  - H1 on masks 0x8 and 0x10.
- notes: G1 line 0x11c (284).

#### phys_fn_000184 (0x8230, 333 B) -- setSleepLinearVelocity (slot 70)
- candidate: NpActorVtable::setSleepLinearVelocity Physics/src/NpActor.cpp:2384 (+ :91)
- status: implemented
- verdict: defect (H1 only; row body faithful (G1))
- blocks checked: 0x8230-0x8269 write-lock try + G1 (line 0x195); 0x826c-0x827b null record ->
  straight to unlock (no error, same as candidate); 0x8281-0x8299 `fld arg; fmul arg; fstp [rec+0xd0]`;
  0x829f-0x836c dirty 0x2000 (H1); 0x836f-0x837a unlock, ret 4.
- defects: H1 only.
- notes: G1 line 0x195 (405).

#### phys_fn_000186 (0x8380, 333 B) -- setSleepAngularVelocity (slot 72)
- candidate: NpActorVtable::setSleepAngularVelocity Physics/src/NpActor.cpp:2409 (+ :91)
- status: implemented
- verdict: defect (H1 only; row body faithful (G1))
- blocks checked: byte-for-byte the 000184 listing shape (diffed): G1 line 0x1a2; null record ->
  unlock; `arg*arg` fstp [rec+0xd4]; dirty 0x4000 (H1); unlock ret 4.
- defects: H1 only.
- notes: G1 line 0x1a2 (418).

#### phys_fn_000188 (0x84d0, 407 B) -- raiseBodyFlag (slot 78)
- candidate: NpActorVtable::raiseBodyFlag Physics/src/NpActor.cpp:2485; nxNpActorTransitionKinematic :130;
  nxNpActorMarkRecordDirty :91
- status: partial -- error report absent; part of the 000785 kinematic transition absent (below)
- verdict: defect
- blocks checked: 0x84d0-0x8509 write-lock try + G1 (line 0x1cf); 0x850c-0x8550 null record ->
  error kind 1 line 0x1d0 + unlock; 0x8553-0x855f `test bl,bl; jns` (flag bit 0x80) -> thiscall
  0x19620 (phys_fn_000785) with ecx=record, push 1; 0x8564-0x8581 [rec+0x10c] |= flag;
  0x8587-0x8656 dirty 0x80000 (H1); 0x8658-0x8664 unlock ret 4.
  Callee 000785 (0x19620-0x19cf9; disassembled from the image -- NOTE the function runs to 0x19cf9,
  0x6da bytes, longer than the inventory's 1325 B): enable arm 0x19635-0x19987: return if
  +0x10c already has 0x80 (`js`); 0x19643-0x1966f if [rec+0x1bc] != rec call 0x10015d30
  (phys_fn_000712, island-root find with path compression, result stored to +0x1bc); then
  root=[rec+0x1bc], if [root+0x1e0] != 0: [root+0x1e4] |= 2; 0x1967e +0xc0 = 0, dirty 0x10000;
  0x19760 +0xc4/+0xc8/+0xcc = 0, dirty 0x20000; 0x19861 +0x10c |= 0x80, dirty 0x80000;
  0x19953-0x1997c if [rec+0x118]==0 alloc vt[+8](0x20, 0) into +0x118; [state+0xc] = 0; ret 4.
- defects:
  - 0x8514-0x8550 (E1): null record -> error(1, NpActor.cpp, 0x1d0 (464), 0,
    "Actor::raiseBodyFlag: Actor must be dynamic!") then unlock; candidate silent.
  - 000785 0x19643-0x1966f: candidate transition has no island-root refresh (phys_fn_000712 on
    +0x1bc, which also writes the compressed root back to +0x1bc) and never sets [root+0x1e4] |= 2
    when the root has an island object at +0x1e0.
  - H1 on masks 0x10000/0x20000/0x80000 (candidate ORs them in one call; the final flags word is the
    same as the oracle's three sequential marks, so the merge itself is not a defect).
- notes: G1 line 0x1cf (463). Candidate's `if(state)` null check after the 0x20 allocation is extra
  (oracle stores to [state+0xc] unconditionally) -- benign. Ordering inside the enable arm
  (flags |= 0x80 before vs after the zeroing) is not observable.

#### phys_fn_000190 (0x8670, 415 B) -- clearBodyFlag (slot 79)
- candidate: NpActorVtable::clearBodyFlag Physics/src/NpActor.cpp:2501; nxNpActorTransitionKinematic :130; :91
- status: partial -- error report absent; part of the 000785 disable transition absent/different
- verdict: defect
- blocks checked: same shape as 000188 (diffed): G1 line 0x1d9; null record -> error kind 1 line 0x1da;
  `test bl,bl; jns` -> 0x19620 with push 0; `not ebx; and [rec+0x10c], ebx`; dirty 0x80000 (H1); unlock.
  Callee 000785 disable arm 0x1998a-0x19cf9: return if +0x10c lacks 0x80 (`jns`); 0x19992-0x199be
  000712 root refresh + [root+0x1e4] |= 2 (as enable arm); 0x199be +0x10c &= ~0x80, dirty 0x80000;
  0x19aa7-0x19ad4 `fld 1.0; fdiv [rec+0x188]; fstp [rec+0xc0]`, dirty 0x10000;
  0x19ba2-0x19bea `1.0/[+0x18c]`, `1.0/[+0x190]`, `1.0/[+0x194]` all on the stack, then stored to
  +0xcc (via stack spill), +0xc4, +0xc8; dirty 0x20000; 0x19cd0-0x19ce8 if +0x118: free vt[+0x14], +0x118 = 0.
- defects:
  - 0x86b4-0x86f0 (E1): null record -> error(1, NpActor.cpp, 0x1da (474), 0,
    "Actor::clearBodyFlag: Actor must be dynamic!") then unlock; candidate silent.
  - 000785 0x19992-0x199be: no island-root refresh / [root+0x1e4] |= 2 in the candidate (as 000188).
  - 000785 0x19aa7-0x19bea: oracle computes inverse mass/inertia as unconditional `1.0f / m`
    (m <= 0 gives +/-inf or a negative inverse); candidate (line 147-148) substitutes 0.0f when
    `mass <= 0` -- a branch the oracle does not have.
  - H1 on masks 0x80000/0x10000/0x20000.
- notes: G1 line 0x1d9 (473). Per-value results for m > 0 match (each quotient rounded to float at
  its store).

#### phys_fn_000192 (0x8810, 371 B) -- wakeUp (slot 73)
- candidate: NpActorVtable::wakeUp Physics/src/NpActor.cpp:2422 (+ :91)
- status: implemented
- verdict: defect (H1 only; row body faithful (G1))
- blocks checked: 0x8810-0x8851 write-lock try + G1 (line 0x207); 0x8854-0x8863 null record ->
  unlock (no error, candidate agrees); 0x8869-0x8886 arg dword stored to +0x84 and +0x4c;
  0x8889-0x8956 dirty 0x10 (H1); 0x8959-0x896c +0x114 &= ~0x100; 0x8973-0x8980 unlock ret 4.
- defects: H1 only.
- notes: G1 line 0x207 (519). Store order +0x84, +0x4c, dirty, +0x114 matches the candidate.

#### phys_fn_000194 (0x8990, 354 B) -- putToSleep (slot 74)
- candidate: NpActorVtable::putToSleep Physics/src/NpActor.cpp:2437 (+ :91)
- status: implemented
- verdict: defect (H1 only; row body faithful (G1))
- blocks checked: 0x8990-0x89d1 write-lock try + G1 (line 0x211), plain `ret` (no args);
  0x89d2-0x89e4 null record -> unlock; 0x89ea-0x8a01 +0x84 = +0x4c = 0; 0x8a04-0x8ac7 dirty 0x10 (H1);
  0x8aca-0x8add +0x114 |= 0x100; 0x8ae4-0x8aed tail-jmp to 002366 unlock.
- defects: H1 only.
- notes: G1 line 0x211 (529).

#### Summary

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000178 | implemented | defect | E1 line 0x109 "setMaxAngularVelocity: Actor must be dynamic!" missing; H1 |
| 000180 | partial | defect | wake-up block missing (|v|^2 >= +0xd0, !(+0x114&0x100), +0x84<0.4f -> +0x84=+0x4c=0x3ecccccc, dirty 0x10); E1 line 0x114; H1 |
| 000182 | partial | defect | x87 row-sum association differs in all 3 rows (+y term first in oracle); wake-up block (vs +0xd4) missing; E1 line 0x11d for null/kinematic; H1 |
| 000184 | implemented | defect (H1 only) | body faithful (G1 0x195); only the shared dirty-helper early returns |
| 000186 | implemented | defect (H1 only) | body faithful (G1 0x1a2); only H1 |
| 000188 | partial | defect | E1 line 0x1d0 missing; 000785 enable arm lacks 000712 root refresh (+0x1bc) and [root+0x1e4]|=2; H1 |
| 000190 | partial | defect | E1 line 0x1da missing; 000785 disable arm lacks root refresh/+0x1e4, candidate adds mass<=0 -> 0 guard vs unconditional 1.0f/m; H1 |
| 000192 | implemented | defect (H1 only) | body faithful (G1 0x207); only H1 |
| 000194 | implemented | defect (H1 only) | body faithful (G1 0x211); only H1 |

H1 = nxNpActorMarkRecordDirty (NpActor.cpp:91) returns without marking when the active-id vector is
unallocated (oracle allocates 2 slots and pushes) and when body id >= 256 (oracle has no bound).

### Review D1 -- NpActor.cpp pose setters and CMass-global setters

Rows: 000196, 000198, 000200, 000202, 000204, 000206, 000208.
Listings: (scratch)<row>.asm. Callee listings disassembled for this review from the oracle
DLL ((scratch) = 000756 @0x17420, m768.asm = 000768 @0x17f10, m789.asm = 000789 @0x19d00).
Constants: [0x101041ec] = 1.0, [0x101041f0] = 0.0, [0x101043cc] = 0.5, [0x101053d4] = 0.39999998.
Candidate: Physics/src/NpActor.cpp, helpers Physics/src/include/NpActorDynamicMath.h.
Precision note: as in joint-open-items Task 4, x87 register values are modelled as double
(53-bit PC). The helper comparisons below assume that convention.

#### Cross-cutting items local to this group (cited by tag)

- **S1: owned-shape update.** Every row here ends with `mov ecx,[actor+0x14]; push 1; call 0x10001070`
  (phys_fn_000004 = `nxForwardSubobjectCall`, ObjectModel.cpp:3650). That forwarder does a *virtual*
  call of slot 6 (+0x18) on [body+0x10] with arg 1, or does nothing if that pointer is null. The candidate
  instead uses `nxNpActorNotifyOwnedShapes` (NpActor.cpp:847). It makes a non-virtual call of
  `ShapeBase::nxApplyOwnerUpdate(1)`, and for kind 5 (a group, [shape+0xd0]==5) it loops over the child
  array at +0xe0..+0xe4. For the four single-shape families this has the same effect: each of their
  internal vtables has slot 6 = `ShapeBase::nxApplyOwnerUpdate` (ObjectModel.cpp:3871/3901/3933/3965).
  The group loop, though, is the candidate's assumption about the group's slot-6 method. Nothing in
  these listings supports it: the oracle makes one virtual call and none of this unit's rows show the
  group's slot 6. This is the remaining "shape-family closure". The faithful spelling is
  `nxForwardSubobjectCall(body, (void*)1)`, i.e. a real slot-6 dispatch.
- **S2: `nxNpActorMarkRecordDirty` (NpActor.cpp:91) versus the inline dirty push.** The inline push appears
  in every row (e.g. 000196 0x8d19-0x8e04). The oracle has no null or range guards. The candidate returns
  early without setting the flag in several cases: `!aux`, `!flags`, `id >= 256`, or any of
  begin/end/cap null. The oracle's grow path handles an unallocated vector (begin = end = cap = 0):
  `cmp cap,end; ja` fails, begin is 0 so the capacity count is 0, it allocates 2 dwords through allocator
  slot +8 (size, 0), skips the free because begin is 0, then pushes the id and ORs the mask. So on the
  first dirty mark into an empty list the candidate loses both the list entry and the `flags[id] |= mask`
  bit. This is a real (shared) defect. The oracle stores `[aux+0x60][id] = count` before growing; the
  candidate stores it after, with the same value, which cannot be observed. Allocator slot +8 with
  (size, 0) matches `malloc(size, NX_MEMORY_PERSISTENT)`, and slot +0x14 matches `free`.
- **S3: extra body-null test.** 000196-000202 read `[actor+0x14]` and then `[body+8]` without a null
  test (e.g. 0x8b44-0x8b4b). The candidate adds `if(body)`. This is benign for a live actor, but it is
  a branch the oracle does not have.
- **G1 message** = 0x10104760 "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a
  deadlock!", kind 2, file 0x10104690.

#### phys_fn_000196 (0x8b00, 1114 B) -- setGlobalPose (slot 1)
- candidate: `NpActorVtable::setGlobalPose` Physics/src/NpActor.cpp:695. Helpers:
  `nxNpActorSetterQuaternionFromMatrix` NpActorDynamicMath.h:155, `nxNpActorMarkRecordDirty` :91,
  `nxNpActorRefreshCMass` :1238 -> `nxNpActorUpdateMassFrame` NpActorDynamicMath.h:281,
  `nxNpActorNotifyOwnedShapes` :847.
- status: partial. Missing: the G1 report (line 0x21d); the virtual owned-shape dispatch (S1).
- verdict: defect (S1, S2; G1 line 0x21d = 541)
- blocks checked:
  - 0x8b00-0x8b41 write-try on [actor+0xc]; on failure, G1 at line 0x21d, then `ret 4`.
  - 0x8b44-0x8b55 body = [actor+0x14], record = [body+8]; a null record goes to the static arm at 0x8f1b.
  - 0x8b5c-0x8ce8 matrix-to-quaternion conversion:
    - the spill fst [esp+0x34] = float(m8+m4); trace = +m0; fcom 0 with `test ah,1` (NaN goes to the
      arms);
    - trace arm: w = 0.5s, register r = 0.5/s, x = (m7-m5)r, y = (m2-m6)r, z = (m3-m1)r;
    - axis select: strict `>` via `test ah,0x41`, m4 > m0, then m8 > m[axis*4];
    - z arm 0x8bf7: s spilled to float, z = s*0.5, r = 0.5/float(s), x = (m6+m2), y = (m7+m5),
      w = (m3-m1);
    - y arm 0x8c46: r spilled to float, z = (m7+m5), x = (m3+m1), w = (m2-m6);
    - x arm 0x8c98: radicand m0 - float(m8+m4), r spilled, y = (m3+m1), z = (m6+m2), w = (m7-m5);
    - 0x8ce8 is an unreachable default.
    - `nxNpActorSetterQuaternionFromMatrix` reproduces every arm, operand order and spill. Faithful.
  - 0x8cec-0x8d16: stores q to rec+0x5c..0x68 and copies it to +0x24..0x30. Matches.
  - 0x8d19-0x8e04: dirty |= 2 (S2).
  - 0x8e06-0x8e33: pose.t to rec+0x50..0x58, copied to +0x18..0x20. Matches.
  - 0x8e36-0x8f0f: dirty |= 1 (S2).
  - 0x8f11: `call 0x17f10` (000768) with ecx = record, called once after both stores. The candidate
    calls `nxNpActorRefreshCMass` once. Matches.
  - 0x8f1b-0x8f3c static arm: t to body+0x44..0x4c, then 9 dwords of M to body+0x20. Matches.
  - 0x8f3e-0x8f57: 000004(body, 1) (S1); unlock [actor+0xc] (002366).
- defects:
  - 0x8b16-0x8b41: G1 (line 0x21d).
  - 0x8f3e-0x8f43: S1, virtual slot-6 call replaced by a non-virtual call plus the group loop.
  - 0x8d19-0x8e04, 0x8e36-0x8f0f: S2, a null/empty dirty vector drops the mark.
- notes: 000768 is checked against `nxNpActorUpdateMassFrame` under 000204's notes and is faithful.
  The listing has no scene or broadphase call beyond 000004. The "scene" work the inventory mentions is
  the scene-stamp/pruner code inside the shape's slot 6 (`ShapeBase::nxApplyOwnerUpdate`), which is
  outside this row.

#### phys_fn_000198 (0x8f60, 418 B) -- setGlobalPosition (slot 2)
- candidate: `NpActorVtable::setGlobalPosition` NpActor.cpp:862
- status: partial. Missing: G1 (line 0x232); S1.
- verdict: defect (S1, S2; G1 line 0x232 = 562)
- blocks checked:
  - 0x8f60-0x8fa1 write-try; G1 at line 0x232.
  - 0x8fa4-0x8fb4 record = [[actor+0x14]+8]; if null, go to 0x90d2.
  - 0x8fba-0x8fef: position to rec+0x50..0x58, copied to +0x18..0x20.
  - 0x8ff2-0x90c5: dirty |= 1 (S2).
  - 0x90c7: 000768(record).
  - 0x90d2-0x90e4 static arm: position to body+0x44..0x4c.
  - 0x90e7-0x90ff: 000004(body, 1) (S1); unlock.
  - The field writes, order and the single refresh all match the candidate.
- defects: G1 at 0x8f76-0x8fa1; S1 at 0x90e7-0x90ec; S2 at 0x8ff2-0x90c5.
- notes: no other scene or group call is in the listing.

#### phys_fn_000200 (0x9110, 821 B) -- setGlobalOrientation (slot 3)
- candidate: `NpActorVtable::setGlobalOrientation` NpActor.cpp:887
- status: partial. Missing: G1 (line 0x242); S1.
- verdict: defect (S1, S2; G1 line 0x242 = 578)
- blocks checked:
  - 0x9110-0x9151 write-try; G1 at line 0x242.
  - 0x9154-0x9165: record test; null goes to 0x941b.
  - 0x916b-0x92f7: conversion. It is instruction for instruction 000196's sequence: the spill at
    [esp+0x2c], the trace arm, z arm 0x9206 with float(s), y arm 0x9255 and x arm 0x92a7 with the
    reciprocal spilled. `nxNpActorSetterQuaternionFromMatrix` is faithful here.
  - 0x92fb-0x9325: q to +0x5c, copied to +0x24.
  - 0x9328-0x940f: dirty |= 2 (S2).
  - 0x9411: 000768.
  - 0x941b-0x9427 static arm: 9 dwords to body+0x20.
  - 0x9429-0x9442: 000004(body, 1) (S1); unlock.
- defects: G1 at 0x9126-0x9151; S1 at 0x9429-0x942e; S2 at 0x9328-0x940f.

#### phys_fn_000202 (0x9450, 611 B) -- setGlobalOrientationQuat (slot 4)
- candidate: `NpActorVtable::setGlobalOrientationQuat` NpActor.cpp:915; static-arm helper
  `nxNpActorRotationFromQuaternionGetter` NpActor.cpp:970.
- status: partial. Missing: G1 (line 0x254); S1.
- verdict: defect (S1, S2; G1 line 0x254 = 596)
- blocks checked:
  - 0x9450-0x9491 write-try; G1 at line 0x254.
  - 0x9494-0x94a5: record test; null goes to 0x95d2.
  - 0x94ab-0x94db: the 4 quaternion words to +0x5c..0x68, copied to +0x24..0x30. Matches.
  - 0x94de-0x95c5: dirty |= 2 (S2).
  - 0x95c7: 000768.
  - 0x95d2-0x9695 static arm: the x87 quaternion-to-matrix into a stack buffer, then rep movsd of 9
    dwords to body+0x20. `nxNpActorRotationFromQuaternionGetter`'s MSVC/x86 `__asm` body matches it
    instruction for instruction:
    - loads in the order w, x, y, z;
    - yy spilled; m0 = (1 - yy) - zz;
    - m1 from register xy - zw;
    - [0x48] and [0xc] spills, then m2 = xz + yz;
    - m3; diagonal spill [0x14]; m4;
    - xw spill [0x10]; m5; m6 = yz - xz; m7 = + xw; m8 = diag - yy.
    The only difference is that the helper's `one` is a local float 1.0 where the oracle uses the global
    constant 0x101041ec. The value is the same.
  - 0x9697-0x96b0: 000004(body, 1) (S1); unlock.
- defects: G1 at 0x9466-0x9491; S1 at 0x9697-0x969c; S2 at 0x94de-0x95c5.
- notes: the helper's non-MSVC `#else` fallback (NpActor.cpp:1042-1051) does not follow the listing's
  spills. It is only an issue on a non-x86 build.

#### phys_fn_000204 (0x96c0, 520 B) -- setCMassGlobalPose (slot 26)
- candidate: `NpActorVtable::setCMassGlobalPose` NpActor.cpp:1419. Helpers:
  `nxNpActorUpdateCMassQuaternion` NpActorDynamicMath.h:323, `nxNpActorApplyWorldMassPose`
  NpActor.cpp:1484, `nxNpActorWakeAfterCMassWrite` :1243.
- status: partial. Missing: the error reports (E1 and G1), the owned-shape update call, and correct
  precision in both callee reproductions.
- verdict: defect
- blocks checked:
  - 0x96c6-0x96e0 precondition before the lock: record = [[actor+0x14]+8] must be non-null and
    `byte [rec+0x10c]` must not have its sign bit set (kinematic 0x80). Otherwise go to 0x989a. The
    candidate's pre-lock test matches.
  - 0x96e6-0x971e: write-try; G1 at line 0x269.
  - 0x9721-0x975b: pose.t to rec+0x158..0x160 and 9 dwords of M to rec+0x134. Matches; `getRowMajor` is
    a straight copy for the untransposed NxMat33.
  - 0x975d: `call 0x17420` (000756), the quaternion of +0x134 into +0x124.
  - 0x9766: `call 0x19d00` (000789), which applies the world mass pose.
  - 0x976b-0x987b wake: if `(rec+0x114 & 0x100)==0` and `rec+0x84 < 0.39999998` (fcomp, `test ah,5; jp`,
    so NaN does not wake), then +0x84 = +0x4c = 0x3ecccccc and dirty |= 0x10.
    `nxNpActorWakeAfterCMassWrite` matches.
  - 0x987d-0x9897: 000004(body, 1) on every path; unlock.
  - 0x989a-0x98c5: E1 report.
- defects:
  - 0x989a-0x98c5: E1. The oracle reports kind 1, line 0x268 (616),
    "Actor::setCMassGlobalPose: Actor must be (non-kinematic) dynamic!" (0x10105530). The candidate
    returns silently.
  - 0x96f2-0x971e: G1 (line 0x269 = 617).
  - 0x987d-0x9882: **the owned-shape update is missing**. The oracle calls 000004(body, 1) after the
    wake test. The candidate's setCMassGlobalPose makes no shape-update call at all.
  - 0x975d (000756) against `nxNpActorUpdateCMassQuaternion`, which uses `nxNpActorQuaternionFromMatrix`:
    - trace sum: the oracle computes (m8 + m4) + m0 (0x17421-0x17430); the candidate computes
      (m0 + m4) + m8.
    - x arm: the oracle's radicand is m0 - float(m8 + m4), with a float spill at [esp] (0x17427/0x175ce).
      The candidate computes m0 - (m4 + m8) in the register.
    - The trace, y and z arms are otherwise identical: register reciprocal, same operands.
    - 000756 is exactly `nxNpActorBodyQuaternionFromMatrix(rec+0x134, rec+0x124)`, the 000801 pattern
      that 000768 also uses. The trace-sum order also decides which arm is taken at the boundary.
  - 0x9766 (000789) against `nxNpActorApplyWorldMassPose`:
    - 0x19d09-0x19d1e: the world tensor comes first, from `call 0x16e80` (000746, cdecl (d=+0xc4,
      R=+0x134, out=+0x164)), which is `nxNpActorWorldTensorRDRt`. The candidate uses
      `nxNpActorWorldTensor` instead, which rounds all nine d[k]R[i][k] products to float and sums
      (x + z) + y; 000746 keeps four products in the register. The results differ in rounding. That the
      oracle computes it first and the candidate last cannot be observed, since the inputs are unchanged.
    - 0x19d23-0x19e5a: actor rotation A[0..8] from W (+0x134) and F (+0xdc). All nine
      `nxNpActorX87Dot3` operand orders match. Faithful.
    - 0x19e5e-0x19ecc: the displacement. For x the oracle computes d0 = (A2 p2 + A1 p1) + A0 p0 and
      keeps it **in the register, not rounded**, then does W0 - d0 (fsubr [+0x158]). For y and z it
      computes d = (A5 p2 + A4 p1) + A3 p0 (and the A8/A7/A6 equivalent), rounds each to float, then
      does W - float(d). The candidate sums (A0 p0 + A1 p1) + A2 p2 in every row and rounds all three
      through a `volatile float`. So the summation order is wrong in all rows and x has an extra
      rounding.
    - 0x19eed-0x19f06: position to +0x50..0x58 and +0x18..0x20; 0x19f18-0x19ff7: dirty |= 1.
      The order matches.
    - 0x19fe1-0x1a1a0: quaternion of A. This is the **setter conversion**: spill float(A8+A4) at
      [esp+0x10]; trace (A8+A4)+A0; z arm 0x1a093 with float(s); y arm 0x1a0ec and x arm 0x1a148 with
      the reciprocal spilled. It is `nxNpActorSetterQuaternionFromMatrix`. The candidate uses
      `nxNpActorQuaternionFromMatrix` (order (A0+A4)+A8, no spills) and patches only the y arm, which
      leaves:
      - the trace order wrong, and with it the patch condition;
      - the x-arm radicand and reciprocal spill wrong;
      - the z-arm 0.5/float(s) wrong.
    - 0x1a1a4-0x1a2ae: q to +0x5c, copied to +0x24; dirty |= 2. The order matches.
    - 000789 does not call 000768, and neither does the candidate. Correct.
- notes:
  - `nxNpActorUpdateMassFrame` was checked against 000768 (0x17f10-0x1839b) and is **faithful**:
    - rotation from +0x24 with five spills (0x17f18-0x17fdb) = `nxNpActorComposeRotation`;
    - the centre: x in the register, y and z rounded, then + t (0x17fe1-0x1807b; stores at
      0x181ec-0x181f4);
    - R*F with per-element operand order matching the `NX_RF` table (0x1807f-0x181e6);
    - +0x124 via the 000801 pattern (0x181f7-0x18379);
    - 000746(+0xc4, +0x134, +0x164).
  - joint-open-items-contract.md ("Task 4 review follow-ups") already records that the CMass-global
    setters "keep their earlier code" and are not driven by the rotated-body tests. The findings above
    are those un-ported sequences.
  - Suggested closure: 000756 -> `nxNpActorBodyQuaternionFromMatrix`; in 000789, use
    `nxNpActorWorldTensorRDRt` and `nxNpActorSetterQuaternionFromMatrix`, and use the listing's
    displacement order and spills.

#### phys_fn_000206 (0x98d0, 504 B) -- setCMassGlobalPosition (slot 27)
- candidate: `NpActorVtable::setCMassGlobalPosition` NpActor.cpp:1574
- status: partial. Missing: E1, G1, the owned-shape update, and 000789's precision.
- verdict: defect
- blocks checked:
  - 0x98d6-0x98f0 pre-lock record/kinematic test (goes to 0x9a9a).
  - 0x98f6-0x992e: write-try; G1 at line 0x277.
  - 0x9931-0x9959: position to rec+0x158..0x160.
  - 0x995f: 000789 only; no 000756, which the candidate also omits. Correct.
  - 0x9964-0x9a7e: the wake, identical to 000204's.
  - 0x9a7f-0x9a97: 000004(body, 1); unlock.
  - 0x9a9a-0x9ac5: E1.
- defects:
  - 0x9a9a-0x9ac5: E1, kind 1, line 0x276 (630),
    "Actor::setCMassGlobalPosition: Actor must be (non-kinematic) dynamic!" (0x10105578). The
    candidate is silent.
  - 0x9902-0x992e: G1 (line 0x277 = 631).
  - 0x9a7f-0x9a84: 000004(body, 1) is missing from the candidate.
  - 0x995f: the 000789 defects listed under 000204 (tensor helper, displacement order and spill,
    quaternion conversion).

#### phys_fn_000208 (0x9ad0, 492 B) -- setCMassGlobalOrientation (slot 28)
- candidate: `NpActorVtable::setCMassGlobalOrientation` NpActor.cpp:1586
- status: partial. Missing: E1, G1, the owned-shape update, and the 000756/000789 precision.
- verdict: defect
- blocks checked:
  - 0x9ad6-0x9af0 pre-lock test.
  - 0x9af6-0x9b2e: write-try; G1 at line 0x282.
  - 0x9b31-0x9b4c: 9 dwords to rec+0x134.
  - 0x9b54: 000756; 0x9b5b: 000789.
  - 0x9b60-0x9c6f: the wake.
  - 0x9c71-0x9c8b: 000004(body, 1); unlock.
  - 0x9c8e-0x9cb9: E1.
- defects:
  - 0x9c8e-0x9cb9: E1, kind 1, line 0x281 (641),
    "Actor::setCMassGlobalOrientation: Actor must be (non-kinematic) dynamic!" (0x101055c0). The
    candidate is silent.
  - 0x9b02-0x9b2e: G1 (line 0x282 = 642).
  - 0x9c71-0x9c76: 000004(body, 1) is missing.
  - 0x9b54: the 000756 trace order and x-arm spill (see 000204).
  - 0x9b5b: the 000789 defects (see 000204).

#### Summary

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000196 | partial | defect | G1 line 0x21d; owned-shape update via 000004 is virtual slot 6 but the candidate hard-codes ShapeBase call plus an unverified group loop (S1); dirty helper drops mark on empty vector (S2); conversion, stores and 000768 are faithful |
| 000198 | partial | defect | G1 line 0x232; S1; S2; arms and refresh are faithful |
| 000200 | partial | defect | G1 line 0x242; S1; S2; setter conversion is faithful (same sequence as 000196) |
| 000202 | partial | defect | G1 line 0x254; S1; S2; static quat-to-matrix x87 sequence is faithful |
| 000204 | partial | defect | E1 kind 1 line 0x268 and G1 line 0x269 silent; 000004 shape update missing; 000756 != UpdateCMassQuaternion (trace order, x-arm spill); 000789 != ApplyWorldMassPose (tensor helper not 000746, displacement order/spill, quaternion is the setter conversion) |
| 000206 | partial | defect | E1 kind 1 line 0x276 and G1 line 0x277 silent; 000004 shape update missing; 000789 precision defects |
| 000208 | partial | defect | E1 kind 1 line 0x281 and G1 line 0x282 silent; 000004 shape update missing; 000756 and 000789 precision defects |

### NpActor.cpp listing review -- GROUP D2 (000210, 000212, 000214, 000216, 000218, 000220, 000222)

Candidate: Physics/src/NpActor.cpp. Shared pieces used by every row below:
- nxNpActorContext / nxNpActorRecord (NpActor.cpp:86, :42): ctx = [this+0xc], record = [[this+0x14]+8] (null body gives record 0). Matches the oracle's `mov ecx,[ebp+0x14]; mov ebx,[ecx+8]; test ebx,ebx; je <error>`.
- Kinematic test: oracle `mov al,[rec+0x10c]; test al,al; js <error>` = bit 0x80 of the byte; candidate `(*(unsigned*)(rec+0x10c) & 0x80) == 0`. Same bit, same sense.
- nxNpActorRefreshCMass -> nxNpActorUpdateMassFrame (NpActor.cpp:1238, NpActorDynamicMath.h:281) stands in for `call 0x10017f10` (phys_fn_000768, ecx = record). Not re-reviewed here (000768 is not in D2); every row calls it once, after all stores/dirties/+0x198 increments and before the wake test, and so does the candidate.
- nxNpActorWakeAfterCMassWrite (NpActor.cpp:1243-1252): skip if [rec+0x114] & 0x100 (oracle `test ah,1; jne`); otherwise `fld [rec+0x84]; fcomp [0x101053d4]; fnstsw; test ah,5; jp skip` = wake only on an ordered strict "below" (equal, above, NaN skip). .rdata 0x101053d4 = 0x3ecccccc (Physics/src/core/Joint.cpp:44), candidate compares `< 0.39999998f` (= 0x3ecccccc). Writes 0x3ecccccc to +0x84 then +0x4c, then dirty 0x10 -- same order as every row's wake block. No +0x198 increment on the wake dirty in either. Faithful.
- Unlock: oracle calls 002366 with the saved ctx ([this+0xc]) on both the success and the static/kinematic error exits; candidate nxNpSceneGuardLeave(ctx) on both. G1 exit returns without unlocking in both.
- NxMat33 storage is row-major (Foundation/include/NxMat33.h data.s._11.._33), so `getRowMajor(dst)` is a 36-byte straight copy, equal to the oracle's `rep movsd` (ecx=9).

H1 (shared helper observation, nxNpActorMarkRecordDirty NpActor.cpp:91-121 -- applies to every dirty mark in these rows; cite, do not re-describe):
The oracle's inline dirty-mark (e.g. 000210 0x9d48-0x9e37) reads flags=[[rec+0x120]+0x40], if flags[id]==0 writes [aux+0x60][id] = (end-begin)/4, then grows when `capacity <= end` (`cmp [aux+0x58],[aux+0x54]; ja skip`) to max((end-begin)/4*2+2) elements -- **with a null begin treated as capacity 0 (`test edx,edx; jne; xor ebp,ebp`), so an empty/null list is allocated (2 slots) via allocator vtbl+8 (size, 0), old block freed via vtbl+0x14 only if non-null** -- pushes id, then ORs the mask. The candidate instead returns without marking anything if aux, flags, begin, end or capacity is null, or if id >= 256 (neither guard exists in the oracle). So on a record whose dirty list has never been allocated (begin == null) the oracle allocates and marks; the candidate drops the mask entirely. The id>=256 bound is also absent from the oracle. For non-null, in-range state the effect (index table write, growth size 2n+2, push, OR) matches. Whether begin==null is reachable at these call sites depends on scene setup (outside D2); flag for whoever owns the helper.

#### phys_fn_000210 (0x9cc0, 998 B) -- setCMassOffsetLocalPose (slot 20)
- candidate: NpActorVtable::setCMassOffsetLocalPose Physics/src/NpActor.cpp:1254-1271
- status: implemented
- verdict: defect (E1; G1; H1)
- blocks checked:
  - 0x9cc0-0x9d01 write-lock try (002364 on [this+0xc]); failure -> G1 error kind 2, line 0x387 (903), msg 0x10104760.
  - 0x9d04-0x9d23 record = [[this+0x14]+8]; null or byte[+0x10c] sign bit -> 0xa06f.
  - 0x9d29-0x9d42 copy pose.t (arg+0x24..+0x2c) to rec+0x100..+0x108.
  - 0x9d48-0x9e37 dirty 0x200 (inline list push/grow, H1); 0x9e39 inc [rec+0x198].
  - 0x9e3f-0x9e50 record reloaded from body; `rep movsd` 9 dwords pose.M (arg+0) -> rec+0xdc.
  - 0x9e52-0x9f36 dirty 0x400; 0x9f39 inc [rec+0x198].
  - 0x9f3f-0x9f45 call 000768 (ecx = record).
  - 0x9f4a-0xa05a wake block (0x114 bit 0x100, 0x84 < [0x101053d4], 0x3ecccccc to +0x84/+0x4c, dirty 0x10).
  - 0xa05c-0xa06c unlock 002366, ret 4.
  - 0xa06f-0xa0a3 error kind 1 line 0x388 (904), msg 0x10105610 "Actor::setCMassOffsetLocalPose: Actor must be (non-kinematic) dynamic!", then unlock, ret.
- defects:
  - 0xa06f-0xa093 (E1): oracle reports FoundationSDK::error(1, NpActor.cpp, 904, 0, "Actor::setCMassOffsetLocalPose: Actor must be (non-kinematic) dynamic!") before unlocking; candidate skips silently.
- notes: store order t -> 0x200 -> ++198 -> M -> 0x400 -> ++198 -> 000768 -> wake matches the candidate exactly. G1 line 903.

#### phys_fn_000212 (0xa0b0, 739 B) -- setCMassOffsetLocalPosition (slot 21)
- candidate: NpActorVtable::setCMassOffsetLocalPosition Physics/src/NpActor.cpp:1273-1287
- status: implemented
- verdict: defect (E1; G1; H1)
- blocks checked:
  - 0xa0b0-0xa0f1 write-lock try; G1 kind 2 line 0x393 (915).
  - 0xa0f4-0xa113 record null / kinematic -> 0xa35c.
  - 0xa119-0xa131 copy position (arg+0..+8) to rec+0x100..+0x108.
  - 0xa137-0xa219 dirty 0x200 (`or dword [eax],0x200`, H1); 0xa21f inc [rec+0x198].
  - 0xa225-0xa22b call 000768.
  - 0xa230-0xa347 wake block (same as shared description).
  - 0xa349-0xa359 unlock, ret 4.
  - 0xa35c-0xa390 error kind 1 line 0x394 (916), msg 0x10105658, unlock, ret.
- defects:
  - 0xa35c-0xa380 (E1): oracle reports error(1, NpActor.cpp, 916, 0, "Actor::setCMassOffsetLocalPosition: Actor must be (non-kinematic) dynamic!"); candidate silent.
- notes: G1 line 915.

#### phys_fn_000214 (0xa3a0, 557 B) -- setCMassOffsetLocalOrientation (slot 22), head part
- candidate: NpActorVtable::setCMassOffsetLocalOrientation Physics/src/NpActor.cpp:1289-1303
- status: implemented (together with 000216, which is its tail -- see below)
- verdict: defect (E1; G1; H1)
- blocks checked:
  - 0xa3a0-0xa3e1 write-lock try; G1 kind 2 line 0x39e (926).
  - 0xa3e4-0xa403 record null / kinematic -> 0xa63c (inside 000216's range).
  - 0xa409-0xa419 `rep movsd` 9 dwords orientation (arg+0) -> rec+0xdc.
  - 0xa41b-0xa4fa dirty 0x400 (H1); 0xa500 inc [rec+0x198].
  - 0xa506-0xa50c call 000768.
  - 0xa511-0xa5c9 wake block first half (0x114/0x84 test, 0x3ecccccc stores, dirty-list push/grow up to the old-element copy loop), 0xa5cb `jmp 0xa5d0` into 000216.
- defects:
  - 0xa63c-0xa660 (in 000216's range) (E1): oracle reports error(1, NpActor.cpp, 0x39f = 927, 0, "Actor::setCMassOffsetLocalOrientation: Actor must be (non-kinematic) dynamic!") (msg 0x101056a8); candidate silent.
- notes: G1 line 926. 000214's own range (0xa3a0-0xa5cc, 557 B) has no `ret` on its success path and no error-report tail; both live in 000216.

#### phys_fn_000216 (0xa5d0, 163 B) -- tail of 000214 (not a separate function)
- candidate: covered by NpActorVtable::setCMassOffsetLocalOrientation Physics/src/NpActor.cpp:1289-1303 (wake dirty via nxNpActorWakeAfterCMassWrite :1243-1252, unlock via nxNpSceneGuardLeave, E1 path absent)
- status: implemented (as part of 000214); not a function in its own right
- verdict: defect (E1 of 000214; H1)
- blocks checked:
  - 0xa5d0-0xa5dc old-element copy loop of the wake dirty-list growth (loop top; the loop's back edge `jne 0xa5d0` targets it, and 000214 enters it by `jmp 0xa5d0` at 0xa5cb).
  - 0xa5de-0xa619 free old block, set begin/end/capacity, push id.
  - 0xa61c-0xa627 OR 0x10 into flags[id] (wake dirty).
  - 0xa629-0xa639 unlock 002366 with [esp+0x18] (= ctx saved by 000214 at 0xa3f1), pop edi/esi/ebx/ebp, `add esp,0xc; ret 4` -- 000214's epilogue (its frame: `sub esp,0xc` + 4 pushes).
  - 0xa63c-0xa670 000214's static/kinematic error path (kind 1, line 927), unlock via esi = ctx, `pop esi/ebx/ebp; add esp,0xc; ret 4`.
- Determination: 000216 is 000214's tail. Evidence: it has no prologue; it uses 000214's registers (esi = aux+0x40, edi = new block, ebx = new byte size, ebp = id) and 000214's stack slots ([esp+0x20] id, [esp+0x18] ctx); its epilogue pops exactly 000214's saves and purges 000214's 4-byte arg; it is reached from 000214 by `jmp 0xa5d0` (0xa5cb), `je 0xa5e2` (0xa5c9), `ja/jae 0xa610` (0xa583, 0xa5a4), `jne 0xa61c` (0xa564), `jne/jp 0xa629` (0xa520, 0xa537) and `je/js 0xa63c` (0xa3f5, 0xa403). 000214 (0xa3a0 + 557 = 0xa5cd) + 3 bytes alignment padding = 0xa5d0; the split is the capstone descent seeding the loop-top jump target as an entry. The candidate covers every block of it inside setCMassOffsetLocalOrientation, except the E1 report (0xa63c-0xa660) and H1.
- defects: E1 as listed under 000214 (the report is physically here).
- notes: no callers other than 000214; no separate candidate function should exist.

#### phys_fn_000218 (0xa680, 1614 B) -- setCMassOffsetGlobalPose (slot 23)
- candidate: NpActorVtable::setCMassOffsetGlobalPose Physics/src/NpActor.cpp:1374-1387, helpers nxNpActorStoreGlobalMassPosition (:1315-1333, poseRow=true), nxNpActorStoreGlobalMassOrientation (:1335-1372, poseRow=true), nxNpActorComposeRotation (NpActorDynamicMath.h:242-265)
- status: implemented
- verdict: defect (E1; G1; H1). Arithmetic is faithful (every element checked).
- blocks checked:
  - 0xa680-0xa6c7 write-lock try; G1 kind 2 line 0x3ab (939).
  - 0xa6ca-0xa6e9 record null / kinematic -> 0xac93.
  - 0xa6ef-0xa7bd rotation R from quaternion +0x5c (x=+0x5c, y=+0x60, z=+0x64, w=+0x68) into [esp+0x68..0x88]; identical instruction pattern to 000220/000222 and to nxNpActorComposeRotation (yy2 float spill [0x10]; zz2 reg; r0=(1-yy2)-zz2; xy2,zw2 reg; r1=xy2-zw2; xz2 float [0x60]; yw2 fst [0x58]; r2=yw2(reg)+xz2; r3=zw2+xy2; xx1=1-2xx fst [0x14]; r4=xx1-zz2; yz2 float [0x5c]; xw2 reg; r5=yz2-xw2; r6=xz2-yw2spill; r7=xw2+yz2; r8=xx1spill-yy2). Every element matches.
  - 0xa7c4 R copied to [esp+0x28..0x48] (rc[k] = [0x28+4k]).
  - 0xa7c6-0xa7e9 d = pose.t (arg+0x24) - t (+0x50): dx `fstp` float [0x1c], dy stays in st, dz float [0x24] (t.z via edx -> [0x54]).
  - 0xa7ed-0xa83b position (R^T d): e0=(r0*dx + r6*dz) + r3*dy; e1=(r1*dx + r7*dz) + r4*dy; e2=(r2*dx + r8*dz) + r5*dy; e2 spilled float [0x24], e0/e1 kept in st. Candidate poseRow: `(P(c,dx)+P(6+c,dz))+P(3+c,dy)` -- matches all three.
  - 0xa83f-0xa94d orientation (R^T W, W = pose.M at arg+0) into temp [esp+0x68..0x88], each fstp float:
    l0=(r6w6+r3w3)+r0w0; l1=(r0w1+r6w7)+r3w4; l2=(r6w8+r3w5)+r0w2;
    l3=(r1w0+r7w6)+r4w3; l4=(r1w1+r7w7)+r4w4; l5=(r1w2+r7w8)+r4w5;
    l6=(r2w0+r8w6)+r5w3; l7=(r2w1+r8w7)+r5w4; l8=(r2w2+r8w8)+r5w5.
    Candidate poseRow block (:1343-1351) matches every element and operand grouping.
  - 0xa941-0xa95c stores +0x108 (e2 via eax), +0x100 (e0), +0x104 (e1); 0xa971 pop dy.
  - 0xa962-0xaa53 dirty 0x200 (H1); 0xaa55 inc +0x198.
  - 0xaa5b-0xaa70 record reloaded; `rep movsd` temp -> rec+0xdc.
  - 0xaa72-0xab55 dirty 0x400 (H1); 0xab58 inc +0x198.
  - 0xab5e-0xab64 call 000768; 0xab69-0xac7b wake block; 0xac7d-0xac90 unlock, ret 4.
  - 0xac93-0xaccb error kind 1 line 0x3ac (940), msg 0x101056f8, unlock, ret.
- defects:
  - 0xac93-0xacb2 (E1): oracle reports error(1, NpActor.cpp, 940, 0, "Actor::setCMassOffsetGlobalPose: Actor must be (non-kinematic) dynamic!"); candidate silent.
- notes: the oracle builds R once and computes both the position and the orientation (into a stack temp) before any record store; the candidate rebuilds R inside each helper and stores +0x100 (with its dirty/++) before computing +0xdc. Results are identical (neither +0x5c nor the argument is written in between; dirty marking touches only the aux list), so this is not a behavioural defect -- only if pose aliased rec+0x100 could it differ. Order of record effects (+0x108,+0x100,+0x104 / 0x200 / ++ / +0xdc / 0x400 / ++ / 000768 / wake) matches. Constant [0x101041ec] read as 1.0 (candidate `1.0`), consistent with the contract doc's shared-sequence claim.

#### phys_fn_000220 (0xacd0, 1059 B) -- setCMassOffsetGlobalPosition (slot 24)
- candidate: NpActorVtable::setCMassOffsetGlobalPosition Physics/src/NpActor.cpp:1389-1401, nxNpActorStoreGlobalMassPosition (:1315-1333, poseRow=false)
- status: implemented
- verdict: defect (E1; G1; H1). Arithmetic faithful.
- blocks checked:
  - 0xacd0-0xad11 write-lock try; G1 kind 2 line 0x3b7 (951).
  - 0xad14-0xad33 record null / kinematic -> 0xb0bc.
  - 0xad39-0xae01 rotation build into [esp+0x38..0x58] -- same sequence as nxNpActorComposeRotation (checked instruction by instruction, spills [0x20]=yy2, [0x10]=xz2, [0x14]=yw2, [0x1c]=xx1, [0x18]=yz2).
  - 0xae05 copy R -> [esp+0x5c..0x7c] (rc[k] = [0x5c+4k]).
  - 0xae07-0xae2f d = position - t: dx float [0x2c], dy in st, dz float [0x34].
  - 0xae33-0xae7f e0=(r6*dz + r3*dy) + r0*dx; e1=(r7*dz + r4*dy) + r1*dx; e2=(r8*dz + r5*dy) + r2*dx. Candidate non-pose `(P(6+c,dz)+P(3+c,dy))+P(c,dx)` -- matches all three.
  - 0xae81-0xae97 e2 via float [0x34] -> +0x108, e0 -> +0x100, e1 -> +0x104 (one rounding each); 0xaeac pop dy.
  - 0xae9d-0xaf7d dirty 0x200 (H1); 0xaf83 inc +0x198.
  - 0xaf89-0xaf8f call 000768; 0xaf94-0xb0a7 wake block; 0xb0a9-0xb0b9 unlock, ret 4.
  - 0xb0bc-0xb0f0 error kind 1 line 0x3b8 (952), msg 0x10105740, unlock, ret.
- defects:
  - 0xb0bc-0xb0da (E1): oracle reports error(1, NpActor.cpp, 952, 0, "Actor::setCMassOffsetGlobalPosition: Actor must be (non-kinematic) dynamic!"); candidate silent.
- notes: G1 line 951.

#### phys_fn_000222 (0xb100, 1187 B) -- setCMassOffsetGlobalOrientation (slot 25)
- candidate: NpActorVtable::setCMassOffsetGlobalOrientation Physics/src/NpActor.cpp:1403-1415, nxNpActorStoreGlobalMassOrientation (:1335-1372, poseRow=false)
- status: implemented
- verdict: defect (E1; G1; H1). Arithmetic faithful.
- blocks checked:
  - 0xb100-0xb141 write-lock try; G1 kind 2 line 0x3c0 (960).
  - 0xb144-0xb163 record null / kinematic -> 0xb56c.
  - 0xb169-0xb22e rotation build into [esp+0x2c..0x4c] -- same sequence as nxNpActorComposeRotation.
  - 0xb232 copy R -> [esp+0x50..0x70] (rc[k] = [0x50+4k]).
  - 0xb234-0xb341 R^T W (W = orientation at arg+0) into [esp+0x2c..0x4c], each fstp float:
    l0=(r0w0+r3w3)+r6w6; l1=(r3w4+r6w7)+r0w1; l2=(r0w2+r3w5)+r6w8;
    l3=(r1w0+r4w3)+r7w6; l4=(r4w4+r7w7)+r1w1; l5=(r4w5+r7w8)+r1w2;
    l6=(r2w0+r5w3)+r8w6; l7=(r5w4+r8w7)+r2w1; l8=(r5w5+r8w8)+r2w2.
    Candidate non-pose block (:1355-1363) matches every element and grouping.
  - 0xb345 `rep movsd` -> rec+0xdc.
  - 0xb347-0xb429 dirty 0x400 (H1); 0xb42f inc +0x198.
  - 0xb435-0xb43b call 000768; 0xb440-0xb557 wake block; 0xb559-0xb569 unlock, ret 4.
  - 0xb56c-0xb5a0 error kind 1 line 0x3c1 (961), msg 0x10105790, unlock, ret.
- defects:
  - 0xb56c-0xb58a (E1): oracle reports error(1, NpActor.cpp, 961, 0, "Actor::setCMassOffsetGlobalOrientation: Actor must be (non-kinematic) dynamic!"); candidate silent.
- notes: G1 line 960. The poseRow (000218) and non-pose (000222) sum orders genuinely differ, and the candidate encodes both correctly.

#### Summary

| row | status | verdict | one-line defect summary |
|---|---|---|---|
| 000210 | implemented | defect | E1 only: no error(1, line 904, "setCMassOffsetLocalPose: Actor must be (non-kinematic) dynamic!"); G1 line 903; H1 helper note |
| 000212 | implemented | defect | E1 only: no error(1, line 916, "setCMassOffsetLocalPosition: ..."); G1 line 915; H1 |
| 000214 | implemented | defect | E1 only: no error(1, line 927, "setCMassOffsetLocalOrientation: ...") (report sits in 000216's range); G1 line 926; H1 |
| 000216 | implemented (as 000214's tail) | defect | Not a separate function: 000214's wake-dirty growth loop, unlock/epilogue and error tail, entered by jumps from 000214; covered by setCMassOffsetLocalOrientation except E1 |
| 000218 | implemented | defect | E1 only: no error(1, line 940, "setCMassOffsetGlobalPose: ..."); G1 line 939; rotation, position and all 9 orientation sums match; H1 |
| 000220 | implemented | defect | E1 only: no error(1, line 952, "setCMassOffsetGlobalPosition: ..."); G1 line 951; rotation and 3 position sums match; H1 |
| 000222 | implemented | defect | E1 only: no error(1, line 961, "setCMassOffsetGlobalOrientation: ..."); G1 line 960; rotation and all 9 sums match; H1 |

H1 (all rows, shared helper nxNpActorMarkRecordDirty NpActor.cpp:91-121): the oracle allocates a 2-slot dirty list when its begin pointer is null and has no id bound; the candidate silently drops the dirty mask if any list pointer is null or id >= 256.

### NpActor.cpp listing review -- GROUP R (34 already-`reconstructed` rows)

Conventions:
- **OM** is the ObjectModel.cpp listing model, which the NxPhysicsObjectLayoutTests oracle differential drives.
- **NA** is the public-path `NpActorVtable` method in Physics/src/NpActor.cpp. The public vtable dispatches to it, and the staged-pair tests run it.

Each form gets its own verdict. Rows 000120, 000146 and 000148 have no OM form.

Additional cross-cutting tags used in this review:
- **NG** (no guard): the NA method takes no scene lock at all. The oracle brackets the read with 002362/002366 on [actor+0x10], or with the 002364 write-try on [actor+0xc] followed by 002366. NG is a real defect, not G1: the lock traffic is absent and the G1 report cannot fire.
- **SSE**: the candidate DLL is built with SSE scalar float (confirmed by disassembling build/Release/NxPhysics.dll, fresh at 08:02 against NpActor.cpp at 07:58). So any NA float chain written in plain C++ (not `__asm`) rounds every intermediate to float. The x87 oracle keeps intermediates in extended precision until its explicit fstp spills. The `__asm` blocks in NA are x87 and are unaffected.
- Constants: 0x101041f0 = 0.0f, 0x101041ec = 1.0f, 0x101043cc = 0.5f, 0x10122078 = identity 3x3. The .data triple at 0x10123c1c is (0,0,0) in the image.
- Callee rows referenced below:
  - 000742 (0x16dd0): kinetic-energy x87 chain.
  - 000744 (0x16e30): group-root compress plus +0x1fc walk, testing [+0x84] > 0.
  - 000713 (0x15d50): recursive root fix.
  - 000015 (0x14f0): shape count. 000019 (0x1540): shape array.
  - 000454 (0xdf90) / 000480 (0xedc0): SDK pointer-binding get/set (the name map).
  - 002406 (0x5ba90): member reset to table 0x101088b8.

#### phys_fn_000050 (0x2610, 107 B) -- getLinearDamping (slot 42)
- candidate:
  - OM `nxActorGetLinearDamping` Physics/src/ObjectModel.cpp:1162
  - NA `NpActorVtable::getLinearDamping` Physics/src/NpActor.cpp:1774
- status: OM implemented; NA partial (no E1 report).
- verdict: OM faithful; NA defect (E1).
- blocks checked:
  - 0x2610-0x2627: read guard, record = [[this+0x14]+8].
  - 0x2629-0x2661: null record: assert, then error(1, NpActor.cpp, 0xd9, 0, "Actor::setLinearDamping: Actor must be dynamic!"), unguard, return 0.0f.
  - 0x2662-0x267a: return [rec+0xb8] after the unguard.
- defects (NA): 0x2629-0x264e. The static-actor report (kind 1, line 0xd9, the literal is the *setLinearDamping* string) is absent. NA returns 0.0f silently.
- notes: NA adds a null-body check; the oracle faults on a null body. This is harmless.

#### phys_fn_000052 (0x2680, 107 B) -- getAngularDamping (slot 44)
- candidate:
  - OM `nxActorGetAngularDamping` ObjectModel.cpp:1182
  - NA `NpActorVtable::getAngularDamping` NpActor.cpp:1799
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x2680-0x2697 guard/record; 0x2699-0x26d1 report arm; 0x26d2-0x26ea returns [rec+0xbc].
- defects (NA): 0x2699-0x26be. The missing report is kind 1, line 0xe8, "Actor::getAngularDamping: Actor must be dynamic!".

#### phys_fn_000060 (0x2900, 72 B) -- computeKineticEnergy (slot 62)
- candidate:
  - OM `nxActorRecordEnergyWord` ObjectModel.cpp:1121, with `nxBodyRecordEnergyWord` ObjectModel.cpp:1044
  - NA `NpActorVtable::computeKineticEnergy` NpActor.cpp:2156
- status: both implemented.
- verdict: OM faithful; NA defect (x87 order).
- blocks checked:
  - 0x2900-0x2917: guard/record.
  - 0x2919-0x2930: call 000742, fstp to float, unguard.
  - 0x2931-0x2947: null record returns 0.
  - 000742 at 0x16dd0-0x16e28: `(((((v74^2+v70^2)+v6c^2)*m188 + (I194*w80)*w80) + (I190*w7c)*w7c) + (I18c*w78)*w78) * 0.5`.
- defects (NA):
  - 000742 addition order. NA computes `((I0w0w0 + I1w1w1) + I2w2w2) + ((v6c^2+v70^2)+v74^2)*m`, then *0.5.
  - The oracle adds the translational term first, then the angular terms in reverse order (I2, I1, I0). Its linear sum starts with v74.
  - The results can differ in the last bit.
- notes: OM's `I*(w*w)` association is exactly equivalent to the oracle's `(I*w)*w` in double, because both inner products are exact. This assumes the 53-bit x87 precision-control default.

#### phys_fn_000062 (0x2950, 60 B) -- isGroupSleeping (slot 67)
- candidate:
  - OM `nxActorChainSettled` ObjectModel.cpp:3431, with `nxBodyRecordChainSettled` :3409 and `nxBodyRecordFixRoot` :3393
  - NA `NpActorVtable::isGroupSleeping` NpActor.cpp:2336, with `nxNpActorGroupRoot` :2329
- status: both implemented.
- verdict: both faithful.
- blocks checked:
  - 0x2950-0x297c: guard, then 000744(rec) when rec != 0, else true.
  - 000744 (0x16e30-0x16e73): path-compress [rec+0x1e8] via 000713, then walk from the root through +0x1fc. Returns false on the first [+0x84] > 0.0f (`test ah,0x41`, so NaN counts as not-awake). Otherwise true.
- notes: NA starts its walk at [root+0x1e8] (== root). That is the same node.

#### phys_fn_000064 (0x2990, 73 B) -- isSleeping (slot 68)
- candidate:
  - OM `nxActorRecordWord84Zero` ObjectModel.cpp:1138
  - NA `NpActorVtable::isSleeping` NpActor.cpp:2360
- status: both implemented.
- verdict: both faithful.
- blocks checked: 0x2990-0x29a8 guard/record; 0x29aa-0x29c8 `sete` of the dword [rec+0x84] == 0 (integer test, so -0.0f is awake); 0x29c9-0x29d8 null record returns true.

#### phys_fn_000066 (0x29e0, 75 B) -- getSleepLinearVelocity (slot 69)
- candidate:
  - OM `nxActorSqrtFieldD0` ObjectModel.cpp:844
  - NA `NpActorVtable::getSleepLinearVelocity` NpActor.cpp:2372
- status: both implemented.
- verdict: both faithful.
- blocks checked: 0x29e0-0x29f7 guard/record; 0x29f9-0x2a13 fsqrt [rec+0xd0], then fstp float; 0x2a14-0x2a2a null returns 0.
- notes: NA's sqrtf (SSE sqrtss) is correctly rounded. That equals the x87 extended fsqrt followed by a float store, because double rounding of sqrt is innocuous at 64 >= 2*24+2 bits.

#### phys_fn_000068 (0x2a30, 75 B) -- getSleepAngularVelocity (slot 71)
- candidate:
  - OM `nxActorSqrtFieldD4` ObjectModel.cpp:864
  - NA `NpActorVtable::getSleepAngularVelocity` NpActor.cpp:2397
- status: both implemented.
- verdict: both faithful. Same shape as 000066, over [rec+0xd4] (0x2a49-0x2a53).

#### phys_fn_000074 (0x2ba0, 85 B) -- raiseActorFlag (slot 75)
- candidate:
  - OM `nxActorRaiseFlags` ObjectModel.cpp:1108, with `nxActorWriteFlagsGuarded` :1085
  - NA `NpActorVtable::raiseActorFlag` NpActor.cpp:2453
- status: both implemented.
- verdict: OM faithful; NA faithful (G1, line 0x1bb).
- blocks checked: 0x2ba0-0x2bd7 write-try on [this+0xc], plus G1 report line 0x1bb; 0x2bda-0x2bf2 [body+0x14] |= mask, then unguard.

#### phys_fn_000076 (0x2c00, 87 B) -- clearActorFlag (slot 76)
- candidate:
  - OM `nxActorClearFlags` ObjectModel.cpp:1114
  - NA `NpActorVtable::clearActorFlag` NpActor.cpp:2463
- status: both implemented.
- verdict: OM faithful; NA faithful (G1, line 0x1c1).
- blocks checked: 0x2c00-0x2c37 write-try, plus G1 line 0x1c1; 0x2c3a-0x2c54 &= ~mask.

#### phys_fn_000078 (0x2c60, 39 B) -- readActorFlag (slot 77)
- candidate:
  - OM `nxActorFlagsMasked` ObjectModel.cpp:828
  - NA `NpActorVtable::readActorFlag` NpActor.cpp:2473
- status: both implemented.
- verdict: both faithful.
- blocks checked: 0x2c60-0x2c84. Read guard, load [body+0x14], unguard, then `test mask; setne`.

#### phys_fn_000080 (0x2c90, 103 B) -- readBodyFlag (slot 80)
- candidate:
  - OM `nxActorReadBodyFlag` ObjectModel.cpp:3779
  - NA `NpActorVtable::readBodyFlag` NpActor.cpp:2517
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked:
  - 0x2c90-0x2ca6: guard/record.
  - 0x2ca8-0x2cdb: null record reports kind 1, line 0x1e3, "Actor::readBodyFlag: Actor must be dynamic!", then returns false.
  - 0x2cde-0x2cf4: `test` of the dword [rec+0x10c] against the mask.
- defects (NA): 0x2ca8-0x2ccd. The report is absent.
- notes: ObjectModel.h:1736 says "byte AND mask". The listing and the OM .cpp both use a dword, so the header comment is stale.

#### phys_fn_000082 (0x2d00, 36 B) -- getNbShapes (slot 15)
- candidate:
  - OM `nxActorShapeRecordCount` ObjectModel.cpp:914, with `nxBodyShapeRecordCount` :880
  - NA `NpActorVtable::getNbShapes` NpActor.cpp:1192
- status: both implemented.
- verdict: OM faithful; NA defect (NG).
- blocks checked:
  - 0x2d00-0x2d23: read guard, 000015(body), unguard.
  - 000015 (0x14f0): null shape gives 0; [sh+0xd0] != 5 gives 1; otherwise ([+0xe4]-[+0xe0]) sar 2.
- defects (NA): 0x2d09/0x2d1a. The read guard pair is absent. The count logic matches. NA returns 0 when first == null; the oracle returns (last-first)>>2, which is identical under the vector invariant.

#### phys_fn_000084 (0x2d30, 36 B) -- getShapes (slot 16)
- candidate:
  - OM `nxActorCollisionObject` ObjectModel.cpp:927, with `nxBodyCollisionObject` :895
  - NA `NpActorVtable::getShapes` NpActor.cpp:1207
- status: both implemented.
- verdict: OM faithful; NA defect (NG).
- blocks checked:
  - 0x2d30-0x2d53: guard, 000019(body), unguard.
  - 000019 (0x1540): null gives null; kind 5 gives [+0xf0]; otherwise sh+0x9c.
- defects (NA): 0x2d39/0x2d4a. There is no read guard.

#### phys_fn_000086 (0x2d60, 40 B) -- getName (slot 84)
- candidate:
  - OM `nxActorBoundTarget` ObjectModel.cpp:941, with `nxGetSdkPointerBinding` Physics/src/PhysicsInternal.cpp:184
  - NA `NpActorVtable::getName` NpActor.cpp:2580, with `nxShapeGetName` NpActor.cpp:450
- status: both implemented.
- verdict: OM faithful; NA defect (NG, plus a different table).
- blocks checked: 0x2d60-0x2d87. Read guard, 000454([this+0x14]) (cdecl, add esp 4), unguard.
- defects (NA):
  - 0x2d69/0x2d7e: no read guard.
  - 0x2d72: the oracle looks the name up in 000454's SDK pointer-binding table (candidate `gPointerBindings`, the same table joint names use). NA reads a separate `gNxShapeNames` table.
  - The lookup semantics (linear scan, miss gives 0) match.

#### phys_fn_000088 (0x2d90, 91 B) -- setName (slot 83)
- candidate:
  - OM `nxActorSetBoundTarget` ObjectModel.cpp:1224, with `nxSetSdkPointerBinding` PhysicsInternal.cpp:205
  - NA `NpActorVtable::setName` NpActor.cpp:2573, with `nxShapeSetName` NpActor.cpp:392
- status: both implemented.
- verdict: OM faithful; NA defect.
- blocks checked: 0x2d90-0x2dc7 write-try on [this+0xc], plus G1 report line 0x1ff; 0x2dca-0x2de8 000480(body, name), then unguard.
- defects (NA):
  - 0x2d96-0x2dc7 and 0x2de1: NA takes no write-try, makes no G1 report (line 0x1ff) and does no unlock.
  - 0x2dd7: NA writes `gNxShapeNames`, not 000480's binding table. Table semantics match 000480: null value removes the entry, freeing the table on last removal; the table is created only for a non-null value; growth is 2n+2.

#### phys_fn_000092 (0x2ed0, 90 B) -- getGlobalPositionVal (slot 6)
- candidate:
  - OM `nxActorGetPoseWords` ObjectModel.cpp:1243
  - NA `NpActorVtable::getGlobalPositionVal` NpActor.cpp:955
- status: both implemented.
- verdict: OM faithful; NA defect (NG).
- blocks checked: 0x2ed0-0x2eea guard/record; 0x2eec-0x2f08 [rec+0x50..0x58]; 0x2f0b-0x2f27 [body+0x44..0x4c]; both arms return out.
- defects (NA): 0x2ed9/0x2eff/0x2f1e. There is no read guard. The values match. NA's extra `actor+0x44` fallback for a null body is dead in the oracle, which faults instead.

#### phys_fn_000094 (0x2f30, 517 B) -- getGlobalOrientationQuatVal (slot 8)
- candidate:
  - OM `nxOrientation0094` ObjectModel.cpp:3208
  - NA `NpActorVtable::getGlobalOrientationQuatVal` NpActor.cpp:1082, which calls `NxQuat(const NxMat33&)`, i.e. `NxMat33::toQuat` Foundation/include/NxMat33.h:845. Candidate code is at 0x1000d560 in build/Release/NxPhysics.dll.
- status: both implemented.
- verdict: OM defect; NA defect.
- blocks checked:
  - 0x2f30-0x2f49: guard/record.
  - 0x2f4b-0x2f74: dynamic arm copies [rec+0x5c..0x68].
  - 0x2f77-0x2f8f: trace = (m11+m22) [fst spill to float at esp+8] + m00, then `fcom 0; test ah,1`. A negative or NaN trace goes to the else arm.
  - 0x2f91-0x2fcb: trace arm.
    - s = fsqrt(t+1), w = float(0.5*s).
    - inv = 0.5/s is kept extended.
    - x = (m21-m12)*inv, y = (m02-m20)*inv; z stays on the stack.
  - 0x2fd0-0x3001: largest-diagonal select (strict `>`, stride 16 bytes).
  - 0x3011-0x305c: case 2.
    - s = fsqrt(m22-(m00+m11)+1), spilled to float.
    - inv = 0.5/float(s).
    - z = 0.5*s with s extended.
  - 0x3061-0x30b2: case 1.
    - s = fsqrt(m11-(m00+m22)+1), y = float(0.5*s).
    - inv is spilled to float.
    - z stays extended.
  - 0x30b4-0x3103: case 0.
    - s = fsqrt(m00 - float(m11+m22) + 1), reusing the trace spill.
    - inv is spilled to float.
  - 0x3105-0x3132: store x, y, z (from st0), w; unguard.
- defects:
  - OM:
    - 0x2f3c/0x3126: no read guard.
    - 0x2f77-0x2f81: the trace is summed as m00+m11+m22. The oracle sums (m11+m22)+m00.
    - 0x2f84: `!(trace<0.0)` sends NaN to the trace arm; the oracle sends it to the else arm.
    - 0x30b4-0x30b7: case 0 uses m00-m11-m22. The oracle uses m00 - float(m11+m22).
    - 0x3011-0x3017 and 0x3061-0x3067: cases 1 and 2 use left-to-right subtraction. The oracle uses m - (a+b).
    - OM computes everything in double with no float spills. The oracle spills inv (cases 0 and 1), float(s) (case 2) and w.
  - NA:
    - 0x2f3c/0x3126: no read guard (NG).
    - The static arm is compiled as SSE single precision (tag SSE). The trace is (m11+m22)+m33 in candidate order (_11+_22 first), against the oracle's (m22+m33)+m11.
    - Every intermediate is rounded to float, so the x87 extended precision of s, inv and the difference terms is lost.
- notes: the dynamic arm (a 4-word copy) is faithful in both forms.

#### phys_fn_000096 (0x3140, 179 B) -- getCMassLocalPoseVal (slot 29)
- candidate:
  - OM `nxActorGetCMassLocalPose` ObjectModel.cpp:3457
  - NA `NpActorVtable::getCMassLocalPoseVal` NpActor.cpp:1613, with `nxNpActorCMassMatrix` :1599 and `nxNpActorCMassPosition` :1606
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x3140-0x3159 guard/record; 0x315b-0x31b9 static arm reports, then identity plus zero into out; 0x31bc-0x31f0 copies 12 words from rec+0xdc.
- defects (NA): 0x315b-0x317e. The missing report is kind 1, line 0x2f2, "Actor::getCMassLocalPose: Cannot be called on a static actor!".

#### phys_fn_000098 (0x3200, 151 B) -- getCMassLocalPositionVal (slot 30)
- candidate:
  - OM `nxActorGetCMassLocalPosition` ObjectModel.cpp:3560, with `nxActorFillThreeWords` :3521
  - NA `NpActorVtable::getCMassLocalPositionVal` NpActor.cpp:1624
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x3200-0x3216 guard/record; 0x3218-0x3268 report, then the .data triple 0x10123c1c (zeros); 0x326b-0x3294 [rec+0x100..0x108].
- defects (NA): 0x3218-0x323d. The missing report is kind 1, line 0x2fa, "Actor::getCMassLocalPosition: Cannot be called on a static actor!".

#### phys_fn_000100 (0x32a0, 108 B) -- getCMassLocalOrientationVal (slot 31)
- candidate:
  - OM `nxActorGetCMassLocalOrientation` ObjectModel.cpp:3486
  - NA `NpActorVtable::getCMassLocalOrientationVal` NpActor.cpp:1634
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x32a0-0x32b8 guard/record; 0x32ba-0x32e7 report, then source = identity 0x10122078; 0x32e9-0x3309 copies 9 words.
- defects (NA): 0x32ba-0x32df. The missing report is kind 1, line 0x301, "Actor::getCMassLocalOrientation: Cannot be called on a static actor!".

#### phys_fn_000102 (0x3310, 151 B) -- getMassSpaceInertiaTensorVal (slot 38)
- candidate:
  - OM `nxActorGetMassSpaceInertia` ObjectModel.cpp:3568
  - NA `NpActorVtable::getMassSpaceInertiaTensorVal` NpActor.cpp:1729
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x3310-0x3326; report arm 0x3328-0x3378 (zeros from .data); 0x337b-0x33a4 [rec+0x18c..0x194].
- defects (NA): 0x3328-0x334d. The missing report is kind 1, line 0x328, "Actor::getMassSpaceInertiaTensorVal: Cannot be called on a static actor!".
- notes: the OM comment at ObjectModel.cpp:3566 and ObjectModel.h:1683 says "slot 44". The public table puts this row at slot 38.

#### phys_fn_000104 (0x33b0, 137 B) -- getLinearVelocityVal (slot 47)
- candidate:
  - OM `nxActorGetLinearVelocity` ObjectModel.cpp:3576
  - NA `NpActorVtable::getLinearVelocityVal` NpActor.cpp:1839
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x33b0-0x33c6; 0x33c8-0x3413 report, then inline zeros; 0x3416-0x3436 [rec+0x6c..0x74].
- defects (NA): 0x33c8-0x33ed. The missing report is kind 1, line 0x343, "Actor::getLinearVelocity: Actor must be dynamic!".

#### phys_fn_000106 (0x3440, 140 B) -- getAngularVelocityVal (slot 48)
- candidate:
  - OM `nxActorGetAngularVelocity` ObjectModel.cpp:3584
  - NA `NpActorVtable::getAngularVelocityVal` NpActor.cpp:1851
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x3440-0x3456; 0x3458-0x34a3 report, then zeros; 0x34a6-0x34c9 [rec+0x78..0x80].
- defects (NA): 0x3458-0x347d. The missing report is kind 1, line 0x34a, "Actor::getAngularVelocity: Actor must be dynamic!".

#### phys_fn_000108 (0x34d0, 173 B) -- getLinearMomentumVal (slot 52)
- candidate:
  - OM `nxActorGetLinearMomentum` ObjectModel.cpp:3593
  - NA `NpActorVtable::getLinearMomentumVal` NpActor.cpp:1918
- status: OM implemented; NA partial.
- verdict: OM faithful; NA defect (E1).
- blocks checked: 0x34d0-0x34e7; 0x34e9-0x353a report, then .data zeros; 0x353d-0x357a m=[rec+0x188] is spilled, then m*v74, m*v70 and m*v6c are each rounded once by fstp.
- defects (NA): 0x34e9-0x350e. The missing report is kind 1, line 0x353, "Actor::getLinearMomentumVal: Cannot be called on a static actor!".
- notes: each component is a single float*float product, so SSE and x87 give identical results.

#### phys_fn_000110 (0x3580, 35 B) -- isDynamic (slot 19)
- candidate:
  - OM `nxActorBodyPresent` ObjectModel.cpp:803
  - NA `NpActorVtable::isDynamic` NpActor.cpp:685
- status: both implemented.
- verdict: OM faithful; NA defect (NG).
- blocks checked: 0x3580-0x35a2. Read guard, load [body+8], unguard, `setne`.
- defects (NA): 0x3589/0x3596. There is no read guard. The comment at NpActor.cpp:682-684 acknowledges this.

#### phys_fn_000112 (0x35b0, 82 B) -- setGroup (slot 85)
- candidate:
  - OM `nxActorSetGroupWord` ObjectModel.cpp:3631
  - NA `NpActorVtable::setGroup` NpActor.cpp:2588
- status: both implemented.
- verdict: OM faithful; NA faithful (G1, line 0x3cd).
- blocks checked: 0x35b0-0x35e7 write-try, plus G1 line 0x3cd; 0x35ea-0x35ff word store to [body+0x1c], then unguard.

#### phys_fn_000114 (0x3610, 34 B) -- getGroup (slot 86)
- candidate:
  - OM `nxActorGetGroupWord` ObjectModel.cpp:815
  - NA `NpActorVtable::getGroup` NpActor.cpp:2597
- status: both implemented.
- verdict: both faithful.
- blocks checked: 0x3610-0x3631. Read guard, word [body+0x1c], unguard.

#### phys_fn_000116 (0x3640, 8 B) -- member-table (+8) this-adjust thunk to slot 0 (table word 87)
- candidate:
  - OM `nxActorDeletingDtorThunk` ObjectModel.cpp:1022
  - NA: none.
- status: OM implemented; NA missing.
- verdict: OM faithful, except that it inherits 000118's allocator defect. NA has nothing to compare.
- blocks checked: 0x3640-0x3643 `sub ecx,8; jmp 000118`.
- notes (NA):
  - The candidate actor has no +8 member subobject and no 0x1010468c table, so word 87 does not exist. `nxActorConstruct` (Scene.cpp:1188) writes 0 at +8.
  - The candidate never deletes actors through a class destructor. `NxSceneInternal::releaseActor` (Physics/src/Scene.cpp:1332) frees the 0x18-byte wrapper directly with `nxGetSdkAllocator()->free(actor)` at Scene.cpp:1369. The createActor failure path does the same (Scene.cpp:1281, after the empty `nxSceneActorDestroy` stub at Scene.cpp:1924).

#### phys_fn_000118 (0x3650, 55 B) -- ~NxActor scalar-deleting destructor (slot 0)
- candidate:
  - OM `nxActorDeletingDtor` ObjectModel.cpp:1005
  - NA: none. NpActorVtable's slot 0 is the compiler-generated `~NpActorVtable` scalar-deleting dtor (empty body, CRT delete), and no candidate path calls it.
- status: OM implemented; NA missing.
- verdict: OM defect; NA missing.
- blocks checked:
  - 0x3650-0x3662: [this] = 0x10104530, [this+8] = 0x1010468c, then 002406 (member reset: [this+8] = 0x101088b8).
  - 0x3667-0x3672: [this] = 0x101043d0.
  - 0x3674-0x367e: if flags&1, free(this) through the IMPORTED `nxFoundationSDKAllocator` (IAT 0x101041bc) at vtable slot +0x14.
  - 0x3681-0x3684: return this.
- defects:
  - OM:
    - 0x3674: frees through `nxGetSdkAllocator()`, the PhysicsSDK allocator (phys_fn_004803, .data 0x12845c), not the NxFoundation import `nxFoundationSDKAllocator`. The candidate declares `nxFoundationSDKAllocator` and uses it elsewhere, e.g. Joint.cpp.
    - 0x3681: returns void. The oracle returns `this` in eax.
  - NA:
    - No destructor semantics are reproduced: none of the table stores and no 002406.
    - Oracle release path: the body teardown at 0x1c40 calls [actor vtbl+0](1), zeroes body[0], then calls 000480(body,0).
    - Candidate release path: Scene.cpp:1369 frees the wrapper with `nxGetSdkAllocator()->free(actor)` and calls `nxShapeSetName(body,0)` (Scene.cpp:1370). The free goes through a different allocator accessor and happens without a vtable dispatch.

#### phys_fn_000120 (0x3690, 429 B) -- saveToDesc (slot 82)
- candidate: NA `NpActorVtable::saveToDesc` NpActor.cpp:2543, with `nxNpActorRotationFromQuaternionGetter` NpActor.cpp:970. There is no OM form.
- status: implemented.
- verdict: faithful (G1, line 0x22).
- blocks checked:
  - 0x3690-0x36d2: WRITE-try on [this+0xc], plus G1 line 0x22.
  - 0x36d5-0x37aa: dynamic arm. The quaternion-to-rows x87 sequence matches the helper's `__asm` instruction for instruction (w, x, y, z loads; the yy/yz/xz/diagonal/xw spills), and its result goes into a local 3x3.
  - 0x37ac-0x37c8: t = [rec+0x50..0x58].
  - 0x37ca: static arm source = body+0x20 (12 words).
  - 0x37cd-0x3811: copy the pose into desc+0..0x2c.
  - 0x3812-0x382b: desc+0x34 = [body+0x18] (density); desc+0x3c = WORD [body+0x1c] (group); desc+0x38 = [body+0x14] (flags); desc+0x40 = [this+4] (userData). Then unguard.
- defects: none beyond G1.
- notes: NA's extra null-body guards are harmless. The oracle moves t.x/t.y by fld/fstp, which only differs for sNaN.

#### phys_fn_000130 (0x4580, 318 B) -- getGlobalPoseVal (slot 5)
- candidate:
  - OM `nxPoseFromQuat0130` ObjectModel.cpp:1363, with `nxQuatToMatrix9` :1344
  - NA `NpActorVtable::getGlobalPoseVal` NpActor.cpp:944. This makes virtual calls to getGlobalOrientationVal :1058 and getGlobalPositionVal :955.
- status: both implemented.
- verdict: OM defect; NA defect (NG).
- blocks checked:
  - 0x4580-0x45a3: ONE read guard, record test.
  - 0x45a9-0x4669: the standard x87 quaternion-to-rows sequence (spills at esp+0x20/0x10/0x14/0x1c/0x18) into a local.
  - 0x466d-0x4685: t = [rec+0x50..0x58].
  - 0x4687: static source = body+0x20 (rotation, then body+0x44 translation).
  - 0x468a-0x46bb: copy 12 words to out, unguard, return out.
- defects:
  - OM:
    - 0x458e/0x46ad: no read guard.
    - 0x45c5-0x4669: `nxQuatToMatrix9` evaluates in double with no float spills, e.g. m[0] = 1-2(yy+zz). The oracle computes (1 - float(2yy)) - 2zz, and likewise spills yz, xz, diagonal and xw to float, so the x87 order differs.
  - NA:
    - 0x4596/0x46ad: no guard in either sub-call.
    - The pose is read in two separate unguarded calls instead of one guarded snapshot. The values match: the x87 rotation comes through the shared `__asm` helper.

#### phys_fn_000132 (0x46c0, 259 B) -- getGlobalOrientationVal (slot 7)
- candidate:
  - OM `nxQuatToMatrix0132` ObjectModel.cpp:3279
  - NA `NpActorVtable::getGlobalOrientationVal` NpActor.cpp:1058, with the `__asm` helper :970
- status: both implemented.
- verdict: OM defect; NA defect (NG).
- blocks checked:
  - 0x46c0-0x46d9: guard/record.
  - 0x46df-0x478c: x87 sequence writing straight into out, with spills yy, yz, xz, diagonal, xw.
  - 0x479e-0x47c0: static arm copies 9 words from body+0x20.
- defects:
  - OM: no guard, and the same double/no-spill x87 difference as 000130.
  - NA: 0x46cc/0x478f/0x47b2 have no read guard. The helper `__asm` matches 0x46df-0x478c exactly.

#### phys_fn_000146 (0x58f0, 579 B) -- getPointVelocityVal (slot 65)
- candidate: NA `NpActorVtable::getPointVelocityVal` NpActor.cpp:728 (`__asm` path :745-821). There is no OM form.
- status: implemented.
- verdict: faithful.
- blocks checked:
  - 0x58f0-0x590c: read guard/record.
  - 0x5912-0x59d4: rotation rows, same as the helper.
  - 0x59d6-0x5a5e: center terms from the mass frame at [rec+0x100]:
    - d0 = (R0m0+R1m1)+R2m2, kept extended.
    - d1 = (R4m1+R5m2)+R3m0, spilled to float.
    - d2 = (R7m1+R8m2)+R6m0, spilled to float.
  - 0x5a62-0x5a97: center and radius:
    - cX = float(px+d0).
    - rX = float(point.x - cX).
    - rY = float(point.y - (py+d1)).
    - rZ = point.z - (pz+d2), kept extended.
  - 0x5a99-0x5ae9: cross product and velocity:
    - crossX = float(rZ*w1 - rY*w2).
    - crossY = float(rX*w2 - rZ*w0).
    - z = (rY*w0 - rX*w1) + v74, kept extended.
    - Then y = crossY + v70 and x = crossX + v6c; each is stored.
  - 0x5aff-0x5b30: null record returns the .data zeros.
- defects: none. The `__asm` block matches operand for operand, including the commutative px+d0 and pz+d2 orderings.

#### phys_fn_000148 (0x5b40, 811 B) -- getLocalPointVelocityVal (slot 66)
- candidate: NA `NpActorVtable::getLocalPointVelocityVal` NpActor.cpp:2186 (`__asm` paths :2208-2224 and :2245-2310). There is no OM form.
- status: implemented.
- verdict: faithful.
- blocks checked:
  - 0x5b40-0x5b59: guard/record.
  - 0x5b5f-0x5c21: rotation rows.
  - 0x5c23-0x5d86: R x F(rec+0xdc), each cell spilled to float. Column 0 and cell (0,1) accumulate in 1,2,0 order; the other cells in 0,1,2 order. This matches the candidate's `reordered` predicate.
  - 0x5d88-0x5dde: local radii kept on the x87 stack:
    - r0 = (C1p1+C2p2)+C0p0.
    - r1 = (C3p0+C4p1)+C5p2.
    - r2 = (C6p0+C7p1)+C8p2.
  - 0x5de0-0x5e26: cross product and velocity:
    - crossX = float(r2w1 - r1w2).
    - crossY = float(r0w2 - r2w0).
    - z = (r1w0 - r0w1) + v74, kept extended.
    - Then x and y are formed; stores follow.
  - 0x5e39-0x5e68: null record returns zeros.
- defects: none.

#### Summary

| row | status (OM / NA) | verdict (OM / NA) | one-line defect summary |
|---|---|---|---|
| 000050 | impl / partial | faithful / defect | NA: no kind-1 report, line 0xd9 "Actor::setLinearDamping: Actor must be dynamic!" (E1) |
| 000052 | impl / partial | faithful / defect | NA: no kind-1 report, line 0xe8 getAngularDamping (E1) |
| 000060 | impl / impl | faithful / defect | NA: energy sum order differs from 000742, which adds linear*m first, then I2, I1, I0 terms (x87 order) |
| 000062 | impl / impl | faithful / faithful | -- |
| 000064 | impl / impl | faithful / faithful | -- |
| 000066 | impl / impl | faithful / faithful | -- |
| 000068 | impl / impl | faithful / faithful | -- |
| 000074 | impl / impl | faithful / faithful (G1 0x1bb) | -- |
| 000076 | impl / impl | faithful / faithful (G1 0x1c1) | -- |
| 000078 | impl / impl | faithful / faithful | -- |
| 000080 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x1e3 readBodyFlag (E1) |
| 000082 | impl / impl | faithful / defect | NA: no read guard (NG) |
| 000084 | impl / impl | faithful / defect | NA: no read guard (NG) |
| 000086 | impl / impl | faithful / defect | NA: no read guard; reads gNxShapeNames rather than 000454's binding table |
| 000088 | impl / impl | faithful / defect | NA: no write-try, G1 report (0x1ff) or unlock; writes gNxShapeNames rather than 000480's table |
| 000092 | impl / impl | faithful / defect | NA: no read guard (NG) |
| 000094 | impl / impl | defect / defect | OM: no guard, trace/case association and float spills differ, NaN arm differs. NA: no guard; static arm is SSE float with a different trace order |
| 000096 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x2f2 (E1) |
| 000098 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x2fa (E1) |
| 000100 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x301 (E1) |
| 000102 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x328 (E1); OM comment says slot 44 (should be 38) |
| 000104 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x343 (E1) |
| 000106 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x34a (E1) |
| 000108 | impl / partial | faithful / defect | NA: no kind-1 report, line 0x353 (E1) |
| 000110 | impl / impl | faithful / defect | NA: no read guard (NG) |
| 000112 | impl / impl | faithful / faithful (G1 0x3cd) | -- |
| 000114 | impl / impl | faithful / faithful | -- |
| 000116 | impl / missing | faithful* / missing | NA: no +8 member table or word-87 thunk; actors are freed directly (Scene.cpp:1369). *OM inherits 000118's allocator defect |
| 000118 | impl / missing | defect / missing | OM: frees via nxGetSdkAllocator, not nxFoundationSDKAllocator (0x101041bc), and returns void. NA: no dtor; Scene.cpp:1369 frees the wrapper with no table stores |
| 000120 | -- / impl | -- / faithful (G1 0x22) | -- |
| 000130 | impl / impl | defect / defect | OM: no guard; double quat-to-matrix without the oracle's float spills. NA: no guard; two unguarded sub-reads |
| 000132 | impl / impl | defect / defect | OM: no guard plus the double/no-spill x87 difference. NA: no read guard (x87 helper exact) |
| 000146 | -- / impl | -- / faithful | -- |
| 000148 | -- / impl | -- / faithful | -- |
