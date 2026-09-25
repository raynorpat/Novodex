# Joint open items

Measurement note for the joint open-items plan (`docs/superpowers/plans/2026-09-25-joint-open-items.md`),
which closes the open items the joint-families plan left in `evidence/joint-families.md`
`## Open items carried forward` (items 1-12). Each task appends one row to the timing table below
and marks the items it closes under `## Open items`.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T15:53:05 | 2026-09-25T16:12:00 | 0 | 0 | Test targets, dead generic path, helper fold; no row written or moved. `NxPhysicsInternalTests` and `NxPhysicsCollisionTests` link `Physics/src/core/*.cpp` (the internal target also PhysicsSDK.cpp/NpPhysicsSDK.cpp for `PhysicsSDK::getParameter`/`instance`); both generated projects give the twelve internal joint files `NoExtensions`. Removed `nxJointConstruct`, `nxJointSizeForType`, `nxJointDestroy`, `NpJoint.cpp`/`NpJoint.h` (no test, ObjectModel or inventory `implementation` user); a type above 9 now takes the oracle's switch default (`cmp eax,9; ja 0x14529`). `core/JointX87.h` folded into `X87Sqrt.h` (`jointFsqrt*` -> `x87Fsqrt*`); all 27 helper instances in NxPhysics.dll byte-identical before and after. Gates 2, 4, 6, 7 pass; 5 red only on its vtables marker; 3 was red on three registered `simulate_mismatches` counts that 0637850 moved to 0 and passes after they were re-registered at 0, which the controller approved (see item 7). |
| 2 | 2026-09-25T16:20:39 | 2026-09-25T16:58:00 | 18 | 1,940 | Scene joint rows (units/joint-open-items-contract.md `## Scene joint rows`). Written: 000661, 000633, 000653, 000598, 000571, 000559, 000563, 000567 (Scene.cpp, `NxSceneInternal` members); 000299, 000321, 000323, 000325 (NpScene.cpp); 000022, 000712, 000758, 000760, 000778 + continuation 000780 (core/JointSupport.cpp). Seven of the 18 (000321/323/325/559/563/567/571, 187 B) were already `reconstructed` as ObjectModel models and now have product rows; eleven (1,753 B) move from `discovered`. 000754 and 004167 stay deferred `NX_ASSERT(0)` stubs (silent no-ops in Release). Scene teardown destroys still-registered joints (000606's loops in `nxSceneDelete`). Test: every family case prints getNbJoints and one iterator pass before and after release, plus a release-then-create cycle; staged pair `stdout_delta=0` on the first run; 27 oracle lines registered in each joint list, floors 6/7 = 143/67. cdb trace: evidence/joint-open-items-trace-release.txt. Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |

## Open items

Numbers are those of `joint-families.md` `## Open items carried forward`.

- **7. Phase 2/3 test targets.** Closed by Task 1 for the build: both targets compile and link, and
  Phase 2 passes. Phase 3 now runs its differential for the first time since 0637850 and fails on
  three registered coverage lines, each a candidate-vs-oracle count under the in-step word 0x0f7f
  that its registration says must fail if it moves either way:
  - `shape_raycast_sphere ... simulate_mismatches=13` (now 0);
  - `shape_raycast_capsule ... simulate_mismatches=242` (now 0);
  - `contact_capsule_capsule ... simulate_mismatches=43` (now 0).
  Every other one of the 85 registered lines matches. The cause is 0637850 (Geometry.cpp square roots
  through fsqrt at the live control word), not Task 1: with `Physics/src/Geometry.cpp` put back to
  `0637850^` on top of Task 1's tree, Phase 3 reports `phase_gate=3 status=pass` with 13/242/43.
  Re-registering the three lines edits existing expected lines, which the plan's Global
  Constraints do not let a task do on its own, so it went to the controller. The controller decided
  to re-register them. The three lines in `tools/gate_targets.ps1` now carry
  `simulate_mismatches=0`, copied verbatim from the Phase 3 run; every other field is unchanged.
  Each line's comment block now says 0637850 removed the in-step difference, gives the old count and
  keeps the line registered so a regression back to nonzero fails. The count change was a
  consequence of 0637850, not of the joint work.
- **11. The generic `createJoint` path.** Closed by Task 1. `nxJointConstruct`, `nxJointSizeForType`,
  `nxJointDestroy` (Scene.cpp) and `NpJointObject`/`NpJointVtable` (`Physics/src/NpJoint.cpp`,
  `Physics/src/include/NpJoint.h`) are removed. Users checked before removal: `tests/`,
  `Physics/src/ObjectModel.*`, every `implementation` field in `inventory.json` (none named
  NpJoint.cpp; the only inventory row at 0x000ad6e0 is phys_fn_004380, already
  `Physics/src/core/PrismaticJoint.cpp`) and CMakeLists.txt (the two test targets, updated). Kept:
  `nxSceneAddJoint` (000661's hole, still called by every family arm). `Scene::createJoint` now
  treats a type above 9 as the oracle does at 0x1438a-0x1438d: no allocation, then the shared tail
  at 0x14529 (++[Scene+0x6c8], [Scene+0x6bc] = [Scene+0x59c], re-entry flag cleared) and a return
  of 0. `desc.isValid()` rejects such a type first, so only a descriptor whose own `isValid()`
  accepts it gets there. The historical references in `phase6-joints.md`, `revolute-contract.md` and
  `joint-families-contract.md` describe the path as it was and are left unchanged.
- **12. Two sets of naked sqrt helpers.** Closed by Task 1. `Physics/src/include/X87Sqrt.h` holds the
  one set: `x87Fsqrt`, `x87FsqrtSum2/3/4`, `x87FsqrtDiag`, `x87FsqrtMulSub`, `x87FsqrtDot2/3/4`.
  The joint files include it and call the `x87Fsqrt*` names. Instruction bodies unchanged: a
  capstone dump of every `?x87Fsqrt*`/`?jointFsqrt*` symbol in `build/Release/NxPhysics.map`
  (27 instances across Geometry.obj and eight joint objects) is byte-identical before and after
  once the name is mapped; for example `x87FsqrtDiag` (was `jointFsqrtDiag`, RevoluteJoint.obj)
  `dd44240cdc442414dc6c2404d9e8dec1d9fac3` and `x87FsqrtDot3` (was `jointFsqrtDot3`, D6Joint.obj)
  `dd442404dc4c240cdd442414dc4c241cdec1dd442424dc4c242cdec1d9fac3`. Earlier documents that name
  `core/JointX87.h` or `jointFsqrt*` now mean `X87Sqrt.h` and `x87Fsqrt*`.
- **1. Release is unwired.** Closed by Task 2. `NpScene::releaseJoint` (000299) forwards to
  Scene::releaseJoint (000653), which removes the joint (000633, with the island dissolve
  000778/000780 and 000760), runs its scalar deleting destructor and decrements the count.
  The cdb trace (`evidence/joint-open-items-trace-release.txt`) shows the chain on all 25
  releases of the joint transcript, each family's and each Np class's scalar deleting
  destructor included, and the scene release destroying the two joints the cycle leaves.
  The chain the joint-families contract names (internal deleting destructor -> 004095 ->
  000633) is not the public one: 000633 runs first and clears `mScene`, so 004095 skips it.
- **2. Deferred Scene rows.** Closed by Task 2 for 000022, 000571, 000598, 000633 and 000758:
  all five have product bodies (000571/000598/000633 as `NxSceneInternal` members in
  Scene.cpp, 000022/000758 in core/JointSupport.cpp). 000022's first callee, 000754 (1,027 B,
  x87), is still a deferred `NX_ASSERT(0)` stub (a silent no-op in Release), as is 004167 behind
  000760's island-object arm. 000571, 000598 and 000758 did not run in the trace (no step, break
  or projection); 000022's breakpoint was not hit either, and its callers (004207, 004298,
  004356) have no dynamic proof.
- **9. `Joint::mScene` (+0x30) is never written.** Closed by Task 2. 000661 writes it last on
  every registration (27 in the trace), 000633 and the scene teardown clear it.
- **Deferred stubs are silent in Release** (joint-open-items Task 2 review). `NX_ASSERT` is
  `assert` (`Foundation/include/NxAssert.h`) and the Release build defines `NDEBUG`, so every
  `NX_ASSERT(0)` deferred stub in `Physics/src/core` compiles to an empty body: reaching one skips
  the oracle's work without any report. Earlier documents that call these "asserting stubs" mean
  this. The ones remaining after Task 2 are both in `core/JointSupport.cpp`:
  `Row000754Fixture::row000754` (phys_fn_000754, called by 000022) and
  `Row004167Fixture::row004167` (phys_fn_004167, called by 000760's island-object arm).
- **Scene teardown free order** (open; found in the Task 2 review, behaviour unchanged). The
  candidate's `nxSceneDelete` frees the joint record array (+0x5b8) and the joint pointer array
  (+0x58c) directly after its joint-list loops. The oracle's destructor 000663 frees +0x5b8 at
  0x13ffb, after the +0x08/+0x0c arrays and the +0x48 object, and +0x58c at 0x14195, near its
  end. No gate observes the order today; aligning it belongs with a faithful 000663.
