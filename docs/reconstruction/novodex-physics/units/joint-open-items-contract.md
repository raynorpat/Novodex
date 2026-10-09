# Joint open items: contracts

Contracts recovered by the tasks of `docs/superpowers/plans/2026-09-25-joint-open-items.md`.
Each task adds its own section.

## Scene joint rows

Recovered by joint-open-items Task 2 (open items 1, 2 and 9 of
`evidence/joint-families.md` `## Open items carried forward`) from the Capstone listing
(authoritative), with the manifest decompiles as a cross-check, the NpScene public table at
.rdata 0x10105a98 and the Scene table at 0x101066f4. The strings the rows report were read
from the image. Ghidra was not run and `oracle/ghidra/supplement.json` is unchanged.

### How the Scene keeps its joints

| Scene field | What it is | Written by |
|---|---|---|
| +0x59c | head of the list of joints whose flag bit 0 is set, linked through Joint +0x10 (`mNextJoint`) | 000661 (push), 000633 (unlink), 000606 (teardown) |
| +0x5a0 | head of a second list through Joint +0x10, for joints with bit 0 clear. 000557 (0x10840, a link-insert already `reconstructed` as an ObjectModel model) pushes onto it for 004103 and 004105, two Joint rows no family path calls and that are not written; 000633 unlinks from it, 000606 destroys it | 000557, 000633, 000606 |
| +0x58c / +0x590 / +0x594 | {begin, end, capacity} array of Joint pointers | 000661 (append), 000778/000780 (append island joints), 000633 (swap-remove), 000663 (free, 0x14195) |
| +0x5b8 / +0x5bc / +0x5c0 | the 0x50-byte JointSupportRecord array, used count, capacity | 004093 (take), 000598 (grow), 000663 (free, 0x13ffb) |
| +0x620 | head of the break-event list, linked through event +4 | 000571 |
| +0x6bc | the enumeration cursor | 000563, 000567, 000653, 000665 |
| +0x6c8 | the joint count getNbJoints returns | 000665 (++ on every exit after the type switch), 000653 (--) |

Joint fields: +0x08/+0x0c `mBody[2]` (body records), +0x10 `mNextJoint`, +0x2c `mFlags` bit 0
("in the +0x59c list"), +0x30 `mScene` (open item 9: written only by 000661, cleared by
000633 and 000606), +0x34 the island link 000780 clears, +0x48 `mPublicObject`.

The count is not the list length. 000665 increments +0x6c8 on every exit after the type
switch (success, allocation failure, null public object, a type above 9), while only a
success pushes onto the list. The transcript cannot show the difference (no failure after the
switch is reachable from the public API with a valid descriptor), but the rows keep it.

### Rows

| Row | RVA | B | Phase | Purpose | Writes | Callers | Decision |
|---|---|---:|---:|---|---|---|---|
| 000661 | 0x13e00 | 290 | 7 | Scene::addJoint | bit 0 set (report `Scene::addJoint: joint is already in a scene.`, code 2, line 0x752, when already set); Joint +0x10 = old head, +0x59c = joint; append to +0x58c (grow to 2n + 2 when full); Joint +0x30 = Scene last | 000665 (0x14524), 004107 (0x97e4e) | write, `NxSceneInternal::addJoint` (Scene.cpp) |
| 000633 | 0x12660 | 370 | 7 | Scene::removeJoint | bit 0 clear: unlink from +0x5a0 or report (code 0xce, line 0x77c), nothing else; bit 0 set: 000778 on the first non-null body with &Scene+0x58c, swap-remove every occurrence from +0x58c, unlink from +0x59c (or report, code 2, line 0x7a6, and stop), then Joint +0x10 = 0, bit 0 cleared, Joint +0x30 = 0 | 000653, 004095 ~Joint (when +0x30 is set), 004103 (0x97c80), 004105 (0x97caa), 004107 (false branch), 004119 (0x9876c) | write, `NxSceneInternal::removeJoint` |
| 000653 | 0x13760 | 126 | 7 | Scene::releaseJoint | the createJoint re-entry flag (.data 0x10123c10; report code 2, line 0x4e2, message via .data 0x10122050); 000633; slot 5 with 1 (scalar deleting destructor) when non-null; --[+0x6c8]; +0x6bc = +0x59c | 000299, 004113 (0x98091) | write, `NxSceneInternal::releaseJoint` |
| 000598 | 0x10f50 | 138 | 7 | grow the record array | +0x5c0 = 2 * capacity, or 4; new block of capacity * 0x50; copy +0x5bc records; free the old, zero, store the new | 004093 | write, `NxSceneInternal::growJointRecords` |
| 000571 | 0x108e0 | 22 | 7 | post a break event | event +4 = [+0x620]; [+0x620] = event (no null test) | 004111, revolute 004374, spherical 004308 | write, `NxSceneInternal::addJointBreakEvent` (the ObjectModel model stays) |
| 000559 | 0x10860 | 7 | 7 | Scene::getNbJoints | none; returns [+0x6c8] | 000321 | write, `NxSceneInternal::getNbJoints` |
| 000563 | 0x10880 | 13 | 7 | Scene::resetJointIterator | +0x6bc = +0x59c | 000323 | write |
| 000567 | 0x108a0 | 23 | 7 | Scene::getNextJoint | +0x6bc = cursor's +0x10; returns the cursor, or 0 | 000325 | write |
| 000299 | 0xc5d0 | 87 | 7 | NxScene::releaseJoint (NpScene slot 6) | write lock +0xc (002364; report `PhysicsSDK: WriteLock is still aquired. ...`, code 2, NpScene.cpp line 0x7f); 000653 on NxJoint +0x08 (appData = the internal joint); unlock the link loaded before the call | public | write, `NpScene::releaseJoint` (NpScene.cpp) |
| 000321 | 0xc910 | 36 | 7 | NxScene::getNbJoints (slot 19) | read lock +0x10 (002362/002366) around 000559 | public | write |
| 000323 | 0xc940 | 31 | 7 | NxScene::resetJointIterator (slot 20) | the same lock around 000563; tail-jump unlock | public | write |
| 000325 | 0xc960 | 55 | 7 | NxScene::getNextJoint (slot 21) | the same lock around 000567; returns [joint+0x48] or 0 | public | write |
| 000778 (+000780) | 0x185f0 (+0x18630) | 58 (+243) | 7 | dissolve a body's island | root refresh through 000712 on the parent; per island body (+0x1d0 chain): every joint on its +0x1d8 list (linked through Joint +0x34) except the removed one appended to the array, every +0x34 cleared, then 000760 on the body; `ret 8` | 000633, 000632 (0x1261d) | write, `Row000778Fixture::row000778` (JointSupport.cpp) |
| 000712 | 0x15d30 | 32 | 2 | island root with path compression | body +0x1bc | 000712 (recursion), 000716, 000720, 000722, 000748, 000762, 000764, 000778, 000785 | write, `Row000712Fixture::row000712` |
| 000760 | 0x17710 | 168 | 2 | reset a body's island fields | when its own root: 004167 on +0x1e0 and free it; +0x1bc = self, +0x1c0/+0x1c4/+0x1d0/+0x1d8/+0x1dc = 0, +0x1c8 = 1, +0x1cc = 0x4b7afafa, +0x1d4 = self, +0x1e4 &= ~2; +0x4c raised to 0.39999998f unless +0x114 bit 8 | 000780, 000604, 000632 (0x12624), 000776 (0x185c8), 000797 (0x1b6fb) | write, `Row000760Fixture::row000760` |
| 000758 | 0x17630 | 214 | 2 | body +0x124 quaternion (x, y, z, w) to the +0x134 3x3 | +0x134..+0x154 | revolute 004356 (0xa9f2a), 000770 (0x1840c), 000772 (0x18514) | write, `Row000758Fixture::row000758` (x87: spills 2yy, 2xz, 2yw, 2yz, 1 - 2xx) |
| 000022 | 0x1840 | 27 | 2 | refresh an actor body after a pose change | 000754 on +0x08; then the +0x10 object's slot 6 with the argument (tail jump) when set | revolute 004356 (0xa9f3d), spherical 004298, D6 004207, 000615 (0x11402), 000774 (0x18559) | write, `Row000022Fixture::row000022` |
| 000754 | 0x17010 | 1027 | 7 | body pose from the mass pose (x87) | - | 000022 | write, `Row000754Fixture::row000754` (written by Task 6, 21b275d) |
| 004167 | 0x9ad10 | 156 | 6 | island-object teardown | - | 000760 | write, `Physics/src/core/JointSupport.cpp`; direct oracle differential and no-op mutation evidence in `evidence/phase6-joint-row004167-island-teardown.md` |
| 000604 / 000606 | 0x110b0 / 0x110f0 | 57 / 158 | 7 | Scene destructor helper: 000760 over +0x56c, then destroy both joint lists | - | 000663 | not claimed; the joint-list loops are reproduced inside the candidate's `nxSceneDelete` |

The joint side's fixtures `Row000571Fixture`, `Row000598Fixture` and `Row000633Fixture` are
gone: `core/Joint.cpp`, `core/RevoluteJoint.cpp` and `core/SphericalJoint.cpp` call the
`NxSceneInternal` members through `static_cast<NxSceneInternal*>(mScene)`, and 004107 calls
`addJoint` where it called the no-op `nxSceneAddJoint`. `Row000022Fixture` and
`Row000758Fixture` keep their names and now have bodies.

`__declspec(noinline)` is on `NxSceneInternal::addJoint` and `Row000760Fixture::row000760`:
the oracle calls both as separate functions, and without it MSVC folds them into createJoint
and row000778, where no breakpoint on the row can see them run.

### Release chain

NxScene::releaseJoint (000299) -> Scene::releaseJoint (000653) -> Scene::removeJoint (000633)
-> 000778/000780 on the first body -> 000760 per island body; then the family's scalar
deleting destructor (slot 5) -> the Np scalar deleting destructor (from the family
destructor's `delete mPublicObject`) -> 004095 ~Joint. 000633 has cleared `mScene` by then, so
~Joint does not call it a second time; the "internal deleting destructor -> 004095 -> 000633"
chain of the joint-families contract is the one a joint takes only when it is destroyed while
still registered, which no public path does (000606 clears `mScene` before its destroy).

Scene release (000663 -> 000604/000606) destroys the joints still on either list after the
actors. The candidate's `nxSceneDelete` reproduces the two list loops after its actor loop and
frees the +0x5b8 and +0x58c arrays; 000604's first loop (000760 over +0x56c) is not reproduced.

### Differences from the pre-task candidate that the new transcript lines catch

- `NpScene::getNbJoints` returned 0, and `resetJointIterator`/`getNextJoint` were empty, so
  every `before_release` line (count=1, enumerated=1, self=yes) failed on the old candidate.
- `NpScene::releaseJoint` was empty and 000661 a no-op, so no joint was ever destroyed; the
  cycle's `count=` values and orders (`8.3.1`, `8.1`, `9.8.1`, `8.1`, `8`, `0.8`) need the list
  push order (new joints at the head) and the unlink of head, middle and tail.

## Body record +0x204

Open item 8 (revolute contract open issue 7). Scan: every Capstone instruction whose operand
is `[reg + 0x204]`. 54 hits. Two stack stores (001878, 003349) and 000973's store (a
shape-side field of another object, next to its +0x150..+0x1e0 stores of 4) are unrelated.
The rest are the joint solver-slot readers (004135, 004194, 004196, 004228, 004240, 004246,
004258, 004272, 004294, 004296, 004308, 004310, 004326, 004358, 004360, 004362, 004374,
004386; 004210's and 004296's own +0x204 are D6/Spherical object fields), four body-side
readers (000708, 000879, 000883, 000897) and exactly two writers on the body record:

| Row | Site | Store | Path |
|---|---|---|---|
| 000797 (0x1b5c0, 402 B) | 0x1b713 `mov [ebx+0x204], esi` (esi = 0) | 0 | the body constructor (vptr 0x10106890; also zeroes +0x198..+0x1b4, +0x1e0/+0x1e4, calls 000760 and 000722). Reached from actor creation (000026 <- 000034 <- 000626 <- 000651/000293). |
| 000611 (0x11260, 269 B) | 0x11305 `mov [eax+0x204], ebx` | `&[Scene+0x5ac][k]` | the simulation step only: 002400 (slot 1 of the 2-slot table phys_data_000982 at .rdata 0x10108894, installed by 002396; an endless loop on the step events) -> 000659 -> 000655 (0x13989) -> 000611. |

The body destructor (000776, which reinstalls vptr 0x10106890 and calls 000760) does not touch
+0x204. Nothing clears it after a step: it keeps pointing into the Scene array until the next
step rewrites it, and 000600 may free and reallocate that array in between.

### What +0x204 points at

Not an allocation of its own. It is element k of a Scene-owned array of 0x60-byte records, where
k is the body's position in its island's body list, not a stable per-body index. 000611 reloads
the element base from +0x5ac for every island (0x112b6, after its 000600 call) and restarts at
element 0, so bodies in different islands are given the same elements in turn, and 000600 may
free and reallocate the array between islands. After a step, a body's +0x204 points at the
element its island last used, in the allocation that was current for that island:

| Scene field | Meaning | Rows |
|---|---|---|
| +0x5ac | array pointer; the allocation starts 4 bytes earlier with the element count | 000600 (grow: free `[+0x5ac]-4` through allocator slot +0x14, malloc `n * 0x60 + 4` through slot +8 with 0, store n at the front, vector-construct n elements of 0x60 with the empty constructor 001391 (0x27f00, 3 B) through 000001), 000663 (free `[+0x5ac]-4` at 0x14019-0x14034) |
| +0x5b0 | size requested by the last step (000600 stores its argument) | 000600 |
| +0x5b4 | capacity; 000600 reallocates only when the argument exceeds it | 000600 |

All three are zeroed by the Scene constructor 000647 (0x12e65-0x12e71); the candidate's
constructor zeroes dwords 0x16b-0x16d.

000611 (thiscall on the Scene, no arguments) sets +0x70c bit 2, then for each entry of the
{begin, end} array at +0x57c/+0x580 (island head bodies) whose head has +0x1f0 != 0: calls
000600 with the head's +0x1f4 (the island body count), then walks the bodies through +0x1fc and
fills one element each (record base `ebx`, `edx = ebx + 0x18`):

| Element | From body | Site |
|---|---|---|
| +0x00..+0x08 | +0x34..+0x3c (the candidate stores linearVelocity here) | 0x112c1-0x112cf |
| +0x0c | +0xc0 (inverse mass) | 0x112d2 |
| +0x10..+0x18 | +0x40..+0x48 (the candidate stores angularVelocity here) | 0x112db-0x112ea |
| +0x1c | the body pointer | 0x112ec |
| +0x20..+0x40 | +0x164..+0x184 (world inverse inertia, `rep movsd` of 9) | 0x112ef-0x112fd |
| +0x5c | +0x110 (the candidate stores solverIterationCount here); also raises the global 0x1012718c to it | 0x112ff, 0x1130b-0x11316 |
| body +0x204 | = the element (element k of this island's pass; `ebx` restarts at `[Scene+0x5ac]` per island, 0x112b6) | 0x11305 |

+0x44..+0x58 are not written here (004174 writes them, below). Then 000730 runs (thiscall on the island head body, with
Scene +0x548/+0x54c). If the Scene has joint constraint records (+0x5bc != 0), 004176 runs.
It zeroes the global 0x10127184, calls 004174 with +0x548 and zeroes 0x1012718c afterwards.
004174 is the record solver:
- it loops 0x1012718c times (the largest +0x5c seen);
- on each pass it calls, for every 0x50-byte record at +0x5b8, the kind's handler from the
  .data table 0x1012234c (`[flags & 0x1f]`, cdecl `(arg, pass, record)`). A record is skipped
  when neither of its two elements (+0x10/+0x14) has +0x5c >= the pass counter,
  which counts down from 0x1012718c to 1;
- it then copies each element's +0x00..+0x08 to +0x44..+0x4c and +0x10..+0x18 to
  +0x50..+0x58 (0x9b1a0-0x9b1ca);
- last, it makes one pass with pass = -1 (0x9b1e4-0x9b22d).
Last, 000613 walks the island's elements and calls
000708 on each element's +0x1c body: 000708 copies element +0x00 -> body +0x34, +0x10 -> body
+0x40, +0x44 -> body +0x1a0, +0x50 -> body +0x1ac, and sets body +0x1e4 bit 5 (0x20). 000613
then clears +0x5bc and moves to the next island; after the last it clears +0x70c bit 2.

The readers agree with this layout: 004374 adds +0x0c times an impulse to +0x00 and the +0x20
3x3 times r x impulse to +0x10 (a velocity update with inverse mass and inverse inertia);
004389 multiplies +0x00 and +0x10 with the constraint record's vectors; 004358 forms
+0x00 + (+0x10 x r). `JointSupportBody` (core/JointSupport.h) now declares all 0x60 bytes with
a `sizeof == 0x60` assert; field names stay by offset.

### Candidate

- **Construction.** The body record is `nxActorComputeMass`'s 0x260-byte block (Scene.cpp),
  memset to 0, so +0x204 was already 0; the store is now explicit, through
  `JointBodyRecord::mUnknown204`, after the +0x1bc/+0x1e8 stores, with the 000797 site in the
  comment. No allocation is added. The oracle makes none for +0x204 at actor creation, so the
  Phase 5 allocation sequences (e.g. the 34-allocation first-dynamic-actor path) stay as they are.
- **Teardown.** Actor release needs nothing: the oracle's body destructor does not touch +0x204.
  `nxSceneDelete` now frees `[+0x5ac]-4` when +0x5ac is non-null (000663's 0x14019-0x14034) and
  clears it. In the candidate +0x5ac is always null, because only the step grows it.
- **Written in the step path:** 000611 now prepares and solves active islands in
  `NxSceneInternal::row000611`; its 000613 continuation copies solved records back through
  `row000708`. Both are mutation-falsified through `NxPhysicsSimulationTests`. The separate
  000600/000708 behavior outside this continuation remains tracked with the simulation-step rows.
- **No public observable.** Before the first simulate, both DLLs hold +0x204 = 0 on every
  body. No public call reads +0x204 without a step.

### Consequence for the internal-slot differential (Task 4)

004358 loads `[body+0x204]` and dereferences it without a null test (0xa9f5a -> `fld [eax+0x14]`,
0xa9fe3 -> `fld [edx+0x14]`). So does 004374 once its body is non-null (0xad2f4 -> `fld [ecx+0xc]`,
0xad41b likewise). With a body attached and no step taken, +0x204 is 0 in the oracle as well,
so slot 0 (the impulse slot) of a body joint faults near address 0 in **both** DLLs. 004360 and
004362 only copy the pointer into their records, and 004389 null-tests the record's +0x10/+0x14
before it reads them (0xaf2dd, 0xaf318), so a null there is harmless. A direct slot differential
has two options. It can leave the slot-0 rows uncalled on bodies that have not been simulated.
Or it can supply a JointSupportBody the same way in both DLLs. Writing one into +0x204 by hand
is test scaffolding, not something the oracle does.

## Rotated bodies and near-z axes

Open items 3 and 6 (revolute contract open issue 3). Test: `tests/PhysicsJointTests.cpp`.
- Every family runs over two near-z axes on the identity fixture: index 4 is (0.1, 0.2, 0.97)
  normalised and index 5 is (0, 0, 1).
- It then runs over a rotated-body fixture in a second scene (`nxBuildRotatedFixture`):
  - actor a is turned 90 degrees about y;
  - actor b is at the unit quaternion (1, 2, 3, 4)/sqrt(30);
  - the matrices are formed in the harness and printed as input.
- The rotated cases are indices 10-13: a general axis, the diagonal, and the two near-z axes.
- Besides the existing fields, the Task 4 cases print:
  - the revolute saveToDesc frames;
  - the rotated actors' read-back pose and body-record words +0x5c, +0xdc, +0x124, +0x134,
    +0x158 and +0x164;
  - each internal joint's +0x4c..+0x14b block and its family tail from +0x16c, read through the
    public object's +0x18. Pulley's tail is left out because its lever words are uninitialised.

The near-z cases matched on the first run. The rotated cases showed four differences. In each
one the first differing word was on the candidate side:

| # | First differing word | Cause (oracle listing) | Fix |
|---|---|---|---|
| 1 | actor b +0x5c quaternion x, y (one bit) | Actor creation fills +0x24 and copies it to +0x5c. It uses the body pose constructor 000801's conversion (0x1b82e-0x1b987): the trace is summed as (m11 + m22) + m00 in the register, (m11 + m22) is spilled to float for the x arm, and each arm is s = sqrt(... + 1), 0.5 * s, then products with the register reciprocal 0.5 / s. The candidate used the public `NxQuat(NxMat33)`, which rounds to float as it goes. | `nxNpActorBodyQuaternionFromMatrix` (NpActorDynamicMath.h), called from `nxActorComputeMass` |
| 2 | every rotated case: `createJoint` returned null (`desc.isValid()` failed) | `NxJointDesc_SetGlobalAnchor`/`SetGlobalAxis` (004115/004117) compose the rotation from +0x5c with the standard row-major formula, spilling five doubled products (0x983c0-0x98483). The candidate's composition was right only for the identity quaternion. It also rounded the axis length to float, which the listing keeps in the register (0x982f9-0x98326), and it summed M^T v in x, y, z order in float. The listing sums (m[6+c] z + m[3+c] y) + m[c] x in the register, with x - t.x unrounded in the anchor row (0x981fb-0x982a0). | JointDesc.cpp: `nxJointWorldMatrix`, `nxJointTransposeMultiply`, the double length |
| 3 | actor a +0x124 w and +0x134 m00 (one bit); this moved 004378's and 004244's relative rotations | The creation path refreshes the mass frame through 000768 (0x17f10, called from 000795 at 0x1b497). 000768 builds R from +0x24 with the 004117 pattern, then computes +0x134 = R F (F = +0xdc) in the listing's per-element operand order, and +0x158 = R p + t (x in the register, y and z rounded first). Last it forms +0x124 from +0x134 by 000801's conversion. The candidate used a double-precision formula and a different quaternion routine. | `nxNpActorUpdateMassFrame`, which replaces `nxActorComputeMass`'s world-centre code and its two calls |
| 4 | actor b +0x164 off-diagonals | 000746 (0x16e80, cdecl) forms R diag(d) R^T as nine products d[k] R[i][k]. Four of them stay in the register and five are spilled to float, and each element sums in the listing's order. | `nxNpActorWorldTensorRDRt` |

After the fixes the whole transcript is byte-identical to the oracle's (1,999 lines).

No joint row needed a change. 004097, 004101, 004121, 004125 and 004129 (anchors, axes, frame
quaternions, world copies), 004378 (prismatic +0x16c) and 004244 (fixed +0x16c..+0x187) match word
for word once their inputs match. The conventions the pilot assumed are therefore the oracle's:
- +0x5c is (x, y, z, w);
- +0xdc and +0x134 are row-major;
- 004378 and 004244 read +0x124, +0x134 and +0x158 as written by 000768.

Scope of the candidate changes:
- (Superseded by the Task 4 review follow-ups, 686cce0: the setters now end in the 000768
  reproduction and `nxNpActorUpdateInertiaMatrices` is deleted; see `### Task 4 review follow-ups`.)
  ~~The Np setters (NpActor.cpp) keep their own sequences. They are not on this transcript.~~
- ~~`nxNpActorUpdateInertiaMatrices` and `nxNpActorUpdateCMassQuaternion` are still used by
  NpActor.cpp:1247-1248/1403/1569. Those paths call 000768 in the oracle (000164, 000196-000222),
  so the same one-bit differences can be expected there on rotated bodies.~~
- Phase 5's registered actor lines are unchanged and still match.

Dynamic evidence (cdb on the oracle pair; hardware write breakpoints on the third and fourth
body records, the rotated actors):
- +0x5c is written at 0x1b98e (000801);
- +0x134 is written at 0x181ea (000768);
- +0x130 (w of +0x124) is written at 0x18226 (000768).

### Task 4 review follow-ups

- **Setters.** The rows 000196, 000198, 000200, 000202, 000210, 000212, 000214, 000218, 000220 and
  000222 each call 000768 with ecx = the record, after their stores and +0x198 increments and
  before the wake test. `nxNpActorRefreshCMass` is now `nxNpActorUpdateMassFrame`. Rotated bodies
  showed three more differences:
  - 000196 and 000200 share an inline conversion (0x8b5c-0x8d07). Its trace arm matches 000801.
    Its z arm uses 0.5 / float(s), and its x and y arms spill the reciprocal. Now
    `nxNpActorSetterQuaternionFromMatrix`.
  - 000218, 000220 and 000222 build R from +0x5c with the same inline sequence as 004115/004117.
    The listings match instruction for instruction once registers and stack slots are normalised.
    Now `nxNpActorComposeRotation`.
  - Each of those rows sums R^T (w - t) and R^T W in its own order. Its dy stays in the register,
    and dx and dz are spilled.
- **000164** (updateMassFromShapes) also calls 000768. The candidate has no body for it.
- **The CMass-global setters** (setCMassGlobalPose/Position/Orientation) do not call 000768 and
  keep their earlier code. The rotated-body tests do not drive them.

## Joint allocator

Rule change (Task 5 follow-up, controller decision). The joint-families plan's Global Constraint
"allocation through `nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT)`" does not hold for the
joint rows and is superseded for them. The plan itself is not edited. This section supersedes the
joint-families plan's "Allocation through `nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT)`"
Global Constraint for every joint row.

- Every joint allocation and free goes through `nxFoundationSDKAllocator` (the Foundation's imported
  `NxUserAllocator*`, `[[0x101041bc]]` in the oracle): `malloc(size, NX_MEMORY_PERSISTENT)` is slot
  +8, `free(p)` slot +0x14. This is `NxAllocateable`'s operator new/delete body; the classes keep
  their own `operator delete` and placement new, which the constructors' null check needs.
- Evidence, per row: a scan of each row's listing for `[0x101041bc]` and for calls to 004803
  (`nxGetSdkAllocator`, 0xb4000). All `[0x101041bc]`, no 004803: the family constructors' 0x1c Np
  allocations (004366, 004380, 004320, 004300, 004276, 004262, 004234, 004222, 004250, 004210),
  000665 (ten internal allocations), 004111/004374/004308 (break events), 004143/004089 (limit
  planes), 000780 and 000661 (pointer-array grow), 000598 (record array), 000760 (island free),
  000600 and 000663 (body array, joint arrays), and every joint deleting destructor (e.g. 004119,
  004368, 004729).
- Other Scene rows keep whatever their own listing says; this section changes only the joint sites.
  The rest of Scene.cpp's `nxGetSdkAllocator()` calls were not audited here.
- Observable: the two allocators differ when the Foundation was created before NxCreatePhysicsSDK
  with another allocator, or when no allocator is passed. `NxPhysicsJointAllocatorTests` is the
  staged-pair target that shows it (phases 6 and 7, 12 oracle lines).

## Scene initialisation

Found by `NxPhysicsJointAllocatorTests` with an allocator that does not return zeroed memory: the
candidate faulted in createActor (`nxSceneAddActorObject` -> `nxSceneArrayReserve` freeing the
uninitialised +0x56c array header); the oracle ran clean.

### Cause

Not a missing field list but a unit error. 98f2625 ("166 more dword-vs-byte offsets") introduced
`nxDword(p, byteOffset)` and converted the constructor's `p[n]` (dword index `n`, correct) into
`nxDword(p, n)` without multiplying by four. From then on phys_fn_000647 wrote its stores to bytes
0x00..0x1c7 (many unaligned) instead of 0x000..0x70c, and placed its sub-objects at byte `n`
instead of `4n` (phys_fn_004147 at 0x0b instead of 0x2c, the SdkContainer at 0x14 instead of 0x50,
and so on). The sub-object helpers (`nxSceneMember*`) had the same error inside them. Everything
the Scene constructor should have written above byte 0x1c7 was left to the allocation, and the
page-guarded allocator's fresh pages are zero, so every harness saw zeros. The fields the candidate
reads after construction that were affected include +0x55c/+0x56c (the actor and body arrays,
the fault), +0x58c/+0x59c/+0x5a0 (joint arrays and lists), +0x5ac..+0x5b8, +0x640 (the static
pruner, now zeroed through 0x0004ca30's base 0x000b4fe0), +0x6d4..+0x704 (the ID recyclers) and
+0x70c (read-modify-written by 000651). The lines at Scene.cpp:436-438 that Task 3's review
matched against 0x12e65..0x12e71 were the dword-index spelling of those three fields, and wrote
bytes 0x16b..0x173.

### Oracle stores (phys_fn_000647, 0x00012c10, Capstone listing)

All stores are reproduced at their byte offsets and in the listing's order:

| Listing | Store |
|---|---|
| 0x12c18 | +0x000 vtable (0x101066f4; the candidate installs its own) |
| 0x12c21..0x12c3f | +0x004..+0x028 = 0 |
| 0x12c42 / 0x12c4c / 0x12c54 | 0x0009a4e0 at +0x2c; 0x000b4d70 (SdkContainer) at +0x50; 0x000e1510 at +0x60 |
| 0x12c5f..0x12c6b | +0x0a8, +0x0ac = 0; 0x000de7e0 at +0xb0 |
| 0x12c70..0x12ca5 | +0x0f4..+0x108 = 0; +0x10c = 1.1f; 0x000d4d00 at +0x110 |
| 0x12caa..0x12d17 | +0x244, +0x248 = 0; +0x288 = 1.1f; +0x254, +0x250, +0x24c, +0x260, +0x25c, +0x258 = 0; +0x264..+0x284 = 0; +0x284, +0x274, +0x264 = 1.0f; 0x000d3490 at +0x28c |
| 0x12d24..0x12d74 | +0x30c, +0x310, +0x314, +0x304, +0x308, +0x318, +0x31c, +0x320, +0x324 = 0; +0x328 = 1.1f; 0x000bb510 at +0x32c |
| 0x12d84..0x12dc8 | +0x448, +0x44c, +0x440 = 0; +0x444 = 1; 0x000b5720 at +0x450; SdkContainer at +0x4e0, +0x4f0, +0x500, +0x510 |
| 0x12dcd..0x12ea5 | +0x52c = 0.1f; +0x530 = 10; +0x534..+0x544 = 0; +0x55c..+0x564, +0x56c..+0x574, +0x57c..+0x584, +0x58c..+0x594 = 0; +0x59c..+0x5cc = 0; +0x5d0 = -1; 0x0005ab50 at +0x5d4 |
| 0x12eaa..0x12ee0 | +0x5fc..+0x604, +0x60c..+0x614, +0x61c, +0x620 = 0; 0x0004ca30 at +0x624 |
| 0x12ee5..0x12f69 | +0x6ac..+0x6dc = 0; +0x6e4..+0x6f0 = 0; +0x6f8..+0x704 = 0; +0x70c = 1; call 0x0002ea70 (a bare `ret`) |
| 0x12f6e..0x12f92 | +0x528, +0x524, +0x520 = 0; +0x0a8, +0x0f4, +0x244, +0x304 = Scene + 0x50 (ebx, the SdkContainer's address, not the Scene's) |
| 0x12f98..0x12fb8 | 0x28-byte NpScene (0x0000c310) -> +0x6cc |
| 0x12fbe..0x12fe9 | 0xa8-byte auxiliary manager (0x0005bc10) -> +0x48, or 0 |

The four "self-references" were stored as the Scene's own address; the listing stores
`lea ebx, [esi + 0x50]`. No candidate code reads the four fields.

Sub-objects, each after its base constructor (0x000f0510: vtable, +4, +8, +0xc = 0; 0x000f0660:
0x000f0510, then +0x10, +0x2c, +0x30 = 0, vtable):

| Row | Stores after the base |
|---|---|
| 004147 (0x0009a4e0) | +0..+0x14 = 0, +0x18 = -1 (unchanged) |
| 004836 (0x000b4d70) | +0, +4, +8 = 0, +0xc = 2.0f (unchanged, `SdkContainer`) |
| 005109 (0x000e1510) | 0x000f0660; +0x34, +0x38 = 0 |
| 005071 (0x000de7e0) | 0x000f0660; +0x3c, +0x38, +0x34, +0x40 = 0 |
| 005029 (0x000d4d00) | 0x000f0660; byte +0x130 = 1 |
| 004996 (0x000d3490) | 0x000f0660 only |
| 004938 (0x000bb510) | 0x000f0510; SdkContainer at +0x10; +0x20..+0x30 = 0; bytes +0x110, +0x111 = 1 |
| 004899 (0x000b5720) | 0x000f0510; +0x5c..+0x68, +0x88 = 0; byte +0x8c = 0; +0x84 = FLT_MAX; byte +0x8d = 1 |
| 001980 (0x0004ca30) | 0x000b4fe0 (+0, +0x1c..+0x28 = 0; +4..+0xc = FLT_MAX; +0x10..+0x18 = -FLT_MAX); +0x2c, +0x30 = 0; 004147 at +0x34; 0x0002dae0 (+0x50, +0x54 = 0); +0x58..+0x60 = FLT_MAX; +0x64..+0x6c = -FLT_MAX; +0x70 = 2; +0x74 = 0; SdkContainer at +0x78 |
| 002346 (0x0005ab50) | +8..+0x10, +0x18..+0x20 = 0; +0 = this+8; +4 = this+0x18. The candidate also zeroed +0x14 and +0x24, which the oracle leaves; removed |
| 002415 (0x0005bc10) | the first three dwords of each 16-byte group +0x00..+0x98 = 0 (30 stores); +0xa4 = owner. The candidate zeroed +0x00..+0xa0 whole; now the 30 stores |

Not reproduced: the vtable word each sub-object (and each base) installs at its +0. Those point into
the oracle's .rdata and no candidate path reads them; they stay as the allocation left them.

### Other objects on the create paths

The actor (0x50), dynamic record (0x260), shape (0x228), shape group (0x110), static pruner (0x90)
and the pruner's entry arrays are `memset` to zero by the candidate before its explicit stores; a
memset writes a superset of the oracle's stores, and under the 0xcd fill they match the oracle in
every harness listed under Test. One place where the candidate initialises what the oracle does
not: the auxiliary manager's 256-slot arrays (Scene.cpp `nxSceneAuxRegisterRecord` /
`nxSceneAuxRegisterShape`). The oracle builds them through a generic resize-with-fill (the
0x0005bd00 region; the 0xd00beed0 fill is the `rep stosd` at 0x5bf18) that writes the used prefix
and leaves the rest of each retained 0x400 block, and the "active" and "vacant" arrays past their
first entry, unwritten; the candidate memsets them. Neither DLL reads a slot past an array's end.
`NxPhysicsActorLifecycleTests` samples those slots directly, so under the fill it prints
`cdcdcdcd` on the oracle and `0` on the candidate in 18 `aux_sample_*` / `aux_indices_*` lines.
Recorded, not changed.

Allocators. The oracle makes both of 000647's allocations through `[[0x101041bc]]` (Foundation).
The candidate's NpScene (0x28) already does, through `NxAllocateable::operator new` ->
`nxFoundationSDKAllocator` (built DLL 0x1002800b: `mov eax,[__imp_nxFoundationSDKAllocator];
call [eax+8]`); only the 0xa8 auxiliary manager uses `nxGetSdkAllocator()`. Candidate-wide,
the oracle's scene, actor, record, shape and group allocations (rows in 0x1000..0x28000 and 0x5a000..0x5c000) all use `[[0x101041bc]]`, while the candidate uses `nxGetSdkAllocator()` at about 117 sites. Pre-existing, tracked as a separate follow-up task. Each block is freed through the allocator that made it, so there is no crash risk; the difference is observable only when the Foundation was created with a different allocator.

### Test

- `NxPhysicsJointAllocatorTests`: the counting allocator now fills every block with 0xcd (it
  returned `calloc` blocks before). Oracle and candidate: exit 0, `stdout_delta=0`; the 12
  registered lines are unchanged. Before the fix the candidate stopped after `scene=created`
  (exit 127).
- `tests/NxPageGuardedAllocator.h` gains `NX_PAGE_GUARDED_FILL` (0xcd over each block, including
  the plain-malloc fallback). Enabled on the nine targets whose registered lines the oracle still
  prints under the fill: NxPhysicsActorNameTests, NxPhysicsActorMetadataTests,
  NxPhysicsActorBodyFlagTests, NxPhysicsActorDynamicSetterTests, NxPhysicsActorMomentumTests,
  NxPhysicsActorForceTests, NxPhysicsActorCMassTests, NxPhysicsActorShapeMutationTests and
  NxPhysicsJointStagedPairTests. NxPhysicsActorLifecycleTests (the aux arrays above) and
  NxPhysicsActorDynamicsTests (its `move_orientation` changed-word mask counts words the fill makes
  non-zero: `ff` against the registered `cf`, on both DLLs) run clean and identical on both DLLs
  under the fill but keep zeroed pages. Before the fix the candidate Lifecycle run faulted at
  Scene +0x640 (the static pruner pointer). No registered line, floor or pin changes.

## Internal-slot differential

Open items 4 and 5 (joint-open-items Task 6). Target `NxPhysicsJointSlotTests`
(`tests/PhysicsJointSlotTests.cpp`), a staged-pair differential on phases 6 and 7.

### Harness

Per case, two fresh dynamic box actors (density 2, posed with general rotations), then the joint
through the public API, then drift: actor 1 moved and turned after creation, both given linear and
angular velocities, a limit point on body 1 and two limit planes (the first accepted). The internal
joint is `*(public + 0x18)`; each slot is called as `(*(void***)internal)[slot]` with `__thiscall`,
so the same code drives both DLLs and no oracle address is named. Per case:

| Call | Slots | Printed |
|---|---|---|
| vis | 4, with a recording `NxDebugRenderable` | every renderer call as raw words, the count |
| step | 1, 7, 6 (004133's order; slot 6 with dt = 1/60) | the Scene records 0..count-1 (20 words each), the record window |
| impulse | 0 (argument unread) | changed words |
| project0/1 | 8 with each non-null body record | changed words |
| break | 2 on record 0 with 2.5 (revolute config 0 only, last) | state, record flags, changed words |

Step, impulse and projection run once under 0x027f and again under 0x0f7f (`fnstcw`/`fldcw` around
each call); visualization runs before and after. Each step prints the control word read back before it is restored
(`control_inside=027f` / `0f7f`). After every call the harness prints each changed
word of the joint (its full size), both body records (0x260) and both injected blocks (0x60).
Pointer words equal to the joint, the public joint, the Scene, a body record or an injected block
print as `JOINT`, `NPJOINT`, `SCENE`, `BODY0/1`, `SUPPORT0/1`. The SDK is created with the
page-guarded allocator and the 0xcd fill (`NX_PAGE_GUARDED_FILL`). SDK parameters: 13 = 1.5,
31 = 1, 32 = 0.75, 33 = 2; 0 and 1 are left at their defaults (0.6, -0.05) and printed.

Cases: revolute (limit + spring, projection 0.05/0.03125; motor, projection 0.5/1.0), prismatic,
cylindrical, spherical (every flag, swing axis (0, 0.6, 0.8), projection 0.05; plain, projection 2),
point-on-line, point-in-plane, distance (min/max/spring; rigid rod), pulley (body 0 the world, so
004228 writes every lever it reads; rigid. The two-dynamic-body path of 004228, which reads the
uninitialised body-0 lever, is not exercised), fixed, D6 (locked/limited/free mix, x and twist drives,
projection; all-linear-limited, locked twist/swing1, swing and slerp drives, projection). Then
`NxFindRotationMatrix` (NxFoundation export) over 13 pairs covering the common arm and every helper
axis of the parallel arm.

### State injected in place of the step

Identical on both sides, and documented in the test file:
- body record +0x204: each non-null body record gets a harness block of 0x60 bytes filled from
  that DLL's own record as 000611 fills it (+0x00..+0x08 <- +0x34..+0x3c, +0x0c <- +0xc0,
  +0x10..+0x18 <- +0x40..+0x48, +0x1c <- the record, +0x20..+0x40 <- +0x164..+0x184, +0x5c <-
  +0x110; +0x44..+0x58 zero). Reset to 0 before the actors are released.
- Scene +0x5bc = 0 before each step (000613 clears it after each island), and the joint's
  +0x160/+0x164 = -1/0 (000728 does this before 004133). The Scene array at +0x5b8 starts empty on
  both sides (000647); 000598 grows it to 4, then 8.
- Bodies: stale when the slots first run (actor 1 was moved), so 004097 runs, on both sides.

### Results

Two candidate defects, both in rows the slots call, none in the slot rows themselves:
1. **000754 was a deferred stub.** Every projection that moves a body calls 000022, whose first
   callee 000754 rebuilds the body's pose (+0x18 position, +0x24 quaternion) from its
   centre-of-mass pose. The candidate's `NX_ASSERT(0)` stub (silent in Release) left +0x18..+0x30
   unchanged; the oracle wrote c - C M^T m and the quaternion of C M^T. Written from the listing
   (core/JointSupport.cpp).
2. **NxFindRotationMatrix rounded differently.** Revolute's second projection turned body 1 by a
   few ULPs differently with the rebuilt Foundation, and matched with the oracle's (candidate
   NxPhysics + oracle NxFoundation). The vendored source rounds the cross product, 1/(1+e) and the
   products to float where the oracle keeps them on the x87 stack. The parallel arm (|e| > 1 -
   1e-6) also wrote the transpose of the oracle's matrix, i.e. the inverse rotation: the vendored
   loop assigns element (j, i) the value the oracle stores at (i, j). Rewritten to the oracle's
   stream (Foundation/src/Utilities.cpp). With the old Foundation 53 transcript lines differ:
   revolute's projection and every later line it feeds, the common-arm rotation cases 0, 1, 2
   and 4 by an ULP or a sign of zero, and all eight parallel-arm cases 10-17 (16 and 17 with
   the off-diagonal signs swapped, i.e. transposed).

After both, the transcript (1369 lines; 1477 with the review's dump read-back) is identical to the oracle's apart from the `modules` line
the runner drops. No difference appears only under 0x0f7f: every step, impulse and projection result
under 0x0f7f matches word for word, in every family. 000754's four roots now go through the X87Sqrt.h helpers
with the stored R elements as operands (Sum4, Diag, Diag, Sum3 in the listing's order), so no sum
the listing keeps on the stack is narrowed at a qword under 0x0f7f; the transcript is unchanged.

### D6JointDump.txt

The D6 solver slot (004206) opens `D6JointDump.txt` once and writes one block per call (004192,
printing through 004190 and 004188). With the pair directory as the working directory each side
writes its own; both are 3548 bytes and 77 lines. Three CRT differences, found by comparing them:
- **Rounding mode (fixed).** UCRT's printf rounds the `%f` digits at the live x87 rounding mode
  (VS2019 16.2+); the oracle's 2003 static CRT always rounds to nearest in software. The two blocks
  written under 0x0f7f (RC chop) differed in last digits (`-0.428584` against `-0.428583`).
  NxPhysics.dll now links `legacy_stdio_float_rounding.obj` (CMakeLists.txt, NxPhysics only), and
  every such difference is gone.
- **FLT_MAX (toolchain residual, not fixed).** The 2003 CRT prints at most 17 significant digits
  and pads with zeros (`340282346638528860000000000000000000000.000000`); UCRT prints the exact
  integer (`340282346638528859811704183484516925440.000000`). This is the only remaining
  difference between the two files (the 8 maxForce lines, FLT_MAX token only).
- **Flush timing (recorded).** The oracle's static CRT flushes and closes the stream at
  DLL_PROCESS_DETACH (`_endstdio`); the candidate's stream belongs to the shared UCRT, which the
  DLL unload does not touch, so it is flushed only at process exit.
The harness therefore unloads the pair after the run, calls `_flushall()` (the candidate's UCRT
instance; nothing is left to flush on the oracle side), and prints the file line by line. A number
with more than 17 integer digits is printed as its float bits (`f32:7f7fffff`). The dump text,
including the poses, angles and JwQ rows, which appear nowhere else in the transcript, is now part
of the compared transcript. It is identical on both sides.

### Not reached

004133 (Joint slot 6) is overridden by every family and called directly only by the step's 000728,
so no table call reaches it. 004087 (slot 3) is not driven.
