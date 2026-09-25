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
| 000633 | 0x12660 | 370 | 7 | Scene::removeJoint | bit 0 clear: unlink from +0x5a0 or report (code 0xce, line 0x77c), nothing else; bit 0 set: 000778 on the first non-null body with &Scene+0x58c, swap-remove every occurrence from +0x58c, unlink from +0x59c (or report, code 2, line 0x7a6, and stop), then Joint +0x10 = 0, bit 0 cleared, Joint +0x30 = 0 | 000653, 004095 ~Joint (when +0x30 is set), 004107 (false branch) | write, `NxSceneInternal::removeJoint` |
| 000653 | 0x13760 | 126 | 7 | Scene::releaseJoint | the createJoint re-entry flag (.data 0x10123c10; report code 2, line 0x4e2, message via .data 0x10122050); 000633; slot 5 with 1 (scalar deleting destructor) when non-null; --[+0x6c8]; +0x6bc = +0x59c | 000299 | write, `NxSceneInternal::releaseJoint` |
| 000598 | 0x10f50 | 138 | 7 | grow the record array | +0x5c0 = 2 * capacity, or 4; new block of capacity * 0x50; copy +0x5bc records; free the old, zero, store the new | 004093 | write, `NxSceneInternal::growJointRecords` |
| 000571 | 0x108e0 | 22 | 7 | post a break event | event +4 = [+0x620]; [+0x620] = event (no null test) | 004111, revolute 004374, spherical 004308 | write, `NxSceneInternal::addJointBreakEvent` (the ObjectModel model stays) |
| 000559 | 0x10860 | 7 | 7 | Scene::getNbJoints | none; returns [+0x6c8] | 000321 | write, `NxSceneInternal::getNbJoints` |
| 000563 | 0x10880 | 13 | 7 | Scene::resetJointIterator | +0x6bc = +0x59c | 000323 | write |
| 000567 | 0x108a0 | 23 | 7 | Scene::getNextJoint | +0x6bc = cursor's +0x10; returns the cursor, or 0 | 000325 | write |
| 000299 | 0xc5d0 | 87 | 7 | NxScene::releaseJoint (NpScene slot 6) | write lock +0xc (002364; report `PhysicsSDK: WriteLock is still aquired. ...`, code 2, NpScene.cpp line 0x7f); 000653 on NxJoint +0x08 (appData = the internal joint); unlock the link loaded before the call | public | write, `NpScene::releaseJoint` (NpScene.cpp) |
| 000321 | 0xc910 | 36 | 7 | NxScene::getNbJoints (slot 19) | read lock +0x10 (002362/002366) around 000559 | public | write |
| 000323 | 0xc940 | 31 | 7 | NxScene::resetJointIterator (slot 20) | the same lock around 000563; tail-jump unlock | public | write |
| 000325 | 0xc960 | 55 | 7 | NxScene::getNextJoint (slot 21) | the same lock around 000567; returns [joint+0x48] or 0 | public | write |
| 000778 (+000780) | 0x185f0 (+0x18630) | 58 (+243) | 7 | dissolve a body's island | root refresh through 000712 on the parent; per island body (+0x1d0 chain): every joint on its +0x1d8 list (linked through Joint +0x34) except the removed one appended to the array, every +0x34 cleared, then 000760 on the body; `ret 8` | 000633 | write, `Row000778Fixture::row000778` (JointSupport.cpp) |
| 000712 | 0x15d30 | 32 | 2 | island root with path compression | body +0x1bc | 000778, 000780 | write, `Row000712Fixture::row000712` |
| 000760 | 0x17710 | 168 | 2 | reset a body's island fields | when its own root: 004167 on +0x1e0 and free it; +0x1bc = self, +0x1c0/+0x1c4/+0x1d0/+0x1d8/+0x1dc = 0, +0x1c8 = 1, +0x1cc = 0x4b7afafa, +0x1d4 = self, +0x1e4 &= ~2; +0x4c raised to 0.39999998f unless +0x114 bit 8 | 000780, 000604 | write, `Row000760Fixture::row000760` |
| 000758 | 0x17630 | 214 | 2 | body +0x124 quaternion (x, y, z, w) to the +0x134 3x3 | +0x134..+0x154 | revolute 004356 | write, `Row000758Fixture::row000758` (x87: spills 2yy, 2xz, 2yw, 2yz, 1 - 2xx) |
| 000022 | 0x1840 | 27 | 2 | refresh an actor body after a pose change | 000754 on +0x08; then the +0x10 object's slot 6 with the argument (tail jump) when set | revolute 004356, spherical 004298, D6 004207 | write, `Row000022Fixture::row000022` |
| 000754 | 0x17010 | 1027 | 7 | body pose from the mass pose (x87) | - | 000022 | defer: asserting stub `Row000754Fixture::row000754` |
| 004167 | 0x9ad10 | 156 | 6 | island-object teardown | - | 000760 | defer: asserting stub `Row004167Fixture::row004167` |
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
