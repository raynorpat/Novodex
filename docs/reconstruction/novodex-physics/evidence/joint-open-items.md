# Joint open items

Measurement note for the joint open-items plan (`docs/superpowers/plans/2026-09-25-joint-open-items.md`),
which closes the open items the joint-families plan left in `evidence/joint-families.md`
`## Open items carried forward` (items 1-12). Each task appends one row to the timing table below
and marks the items it closes under `## Open items`.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T15:53:05 | 2026-09-25T16:12:00 | 0 | 0 | Test targets, dead generic path, helper fold; no row written or moved. `NxPhysicsInternalTests` and `NxPhysicsCollisionTests` link `Physics/src/core/*.cpp` (the internal target also PhysicsSDK.cpp/NpPhysicsSDK.cpp for `PhysicsSDK::getParameter`/`instance`); both generated projects give the twelve internal joint files `NoExtensions`. Removed `nxJointConstruct`, `nxJointSizeForType`, `nxJointDestroy`, `NpJoint.cpp`/`NpJoint.h` (no test, ObjectModel or inventory `implementation` user); a type above 9 now takes the oracle's switch default (`cmp eax,9; ja 0x14529`). `core/JointX87.h` folded into `X87Sqrt.h` (`jointFsqrt*` -> `x87Fsqrt*`); all 27 helper instances in NxPhysics.dll byte-identical before and after. Gates 2, 4, 6, 7 pass; 5 red only on its vtables marker; 3 red on three registered `simulate_mismatches` counts that 0637850 moved to 0 (see item 7). |

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
  Re-registering the three lines is an edit to existing expected lines, which the plan's Global
  Constraints do not allow a task to do on its own; it is left for a decision.
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
