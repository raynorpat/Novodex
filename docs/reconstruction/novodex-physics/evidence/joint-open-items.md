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
| 3 | 2026-09-25T17:05:00 | 2026-09-25T17:21:01 | 0 | 0 | Body record +0x204 (units/joint-open-items-contract.md `## Body record +0x204`). Capstone scan of `[reg+0x204]`: the two body-record writers are 000797 (body constructor, stores 0 at 0x1b713) and 000611 (0x11305, points it at the body's 0x60-byte element of the Scene's +0x5ac array). 000611 is reachable only from the simulation thread's step (002400 -> 000659 -> 000655). No allocation at actor creation, so the Phase 5 allocation sequences are unchanged. Candidate: explicit `mUnknown204 = 0` in `nxActorComputeMass` (the memset already gave 0), `nxSceneDelete` frees `[+0x5ac]-4` when non-null (000663 0x14019-0x14034; always null in the candidate), `JointSupportBody` declared to its full 0x60 bytes with offset and size asserts. 000600/000611/000613/000708/004174/004176 not written (step only). No public observable and no probe line: both DLLs hold +0x204 = 0 until a step. Slot 0 (004358/004374) dereferences +0x204 unconditionally, so on an unsimulated body joint it faults in both DLLs (Task 4 note). No row changes state; inventory unchanged. Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |
| 4 | 2026-09-25T17:23:00 | 2026-09-25T17:56:16 | 0 | 0 | Rotated bodies and near-z axes (units/joint-open-items-contract.md `## Rotated bodies and near-z axes`). Test: every family over two near-z axes on the identity fixture (indices 4, 5) and over a rotated-body fixture in a second scene (actor a 90 degrees about y, actor b at (1, 2, 3, 4)/sqrt(30); indices 10-13: a general, a diagonal and the two near-z axes); revolute prints its saved frames; the Task 4 cases also print the rotated actors' body-record words and each internal joint's +0x4c..+0x14b block and family tail. Four transcript differences, four candidate defects, all on the candidate side: (1) the actor quaternion at +0x24/+0x5c came from the public NxQuat(NxMat33) conversion, not 000801's x87 one (one bit on actor b); (2) NxJointDesc_SetGlobalAnchor/SetGlobalAxis composed the body rotation with a formula that was right only for the identity quaternion, so every rotated case failed desc.isValid(); their normalisation and transposed products are now kept in the register as in the listing; (3) the creation path's +0x134/+0x158/+0x124 did not follow 000768 (one bit on actor a's +0x134 and +0x124, which moved 004378's and 004244's relative rotations); (4) +0x164 did not follow 000746. No joint row was wrong: 004097/004101/004121/004125/004129/004244/004378 match word for word once their inputs match. Near-z axes matched on the first run. 36 oracle lines registered in each joint list, floors 6/7 = 215/103. No inventory row changes state (000768 and 000746 are reproduced whole as helpers in NpActorDynamicMath.h, 000801 in part; none is claimed). Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |
| 5 | 2026-09-25T18:30:40 | 2026-09-25T18:55:00 | 0 | 0 | SEH/GS frames on the joint deleting destructors (item 10). Cause: CMake's default `/EHsc`; the oracle has no C++ exception handling (no `__CxxFrameHandler`, and its only `fs:[0]` uses are CRT rows (0xf4000+)). Scratch reproduction (cl 14.51, `/O2 /EHsc`, a class with an inline class `operator delete` calling a non-noexcept allocator): the `??_G` gets `push -1; push handler; fs:[0]` plus the cookie; with `/EHs-c-` it is frameless; `/GS-` removes only the cookie, not the frame; `noexcept` on the operator delete alone, `throw()`, `/Zc:implicitNoexcept-` and `noexcept(false)` destructors do not remove it. Fix: `/EHs-c-` appended to the 22 joint class files in CMakeLists.txt (core/Joint.cpp, the ten family files, NpJointShared.cpp, the ten Np files); `/GS` unchanged. Before: 53 functions in those objects carried an EH frame and cookie (every `??_G`/`??_E`, the family constructors, `??1Joint`, the class `operator delete`s). After: none; the four cookie-only rows (D6Joint row_slot6/row_slot8, Joint::setGlobalAnchor/setGlobalAxis) keep their cookies. Joint staged-pair transcript byte-identical before/after (paths and DLL hashes aside); NxPhysicsJointTests output identical. No row changes state. Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |
| 5 (follow-up) | 2026-09-25T19:00:08 | 2026-09-25T19:15:00 | 0 | 0 | Joint allocator (item 10, allocator subsection; units/joint-open-items-contract.md `## Joint allocator`). Every joint allocation and free now goes through `nxFoundationSDKAllocator` as the oracle rows do (`[[0x101041bc]]` slots +8/+0x14; no joint row calls 004803): 36 sites in core/*.cpp, Joint.h, NpJointShared.h, JointSupport.h and Scene.cpp (createJoint, 000598, the three joint-array frees in `nxSceneDelete`). New staged-pair target `NxPhysicsJointAllocatorTests` (phases 6 and 7): Foundation created with allocator A, then the SDK with B; a revolute and a distance joint created and released. Oracle and candidate: A 0x204,0x1c,8 then 2 frees; 0x184,0x1c then 2 frees; B none. Before: all in B. 12 oracle lines registered; floors 6/7 = 257/130. Found: the candidate faults in createActor when the SDK allocator does not return zeroed memory (Scene block array header left unset), before and after this change; recorded as open. No row changes state. Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |
| Scene init | 2026-09-25T19:14:00 | 2026-09-25T19:45:00 | 2 | 1,032 | Scene initialisation (units/joint-open-items-contract.md `## Scene initialisation`). 000647 (998 B) rewritten at byte offsets from the listing (it wrote dword indices as byte offsets since 98f2625) with its sub-object helpers (004938, 004899, 005109, 005071, 005029, 004996, 001980 and their bases; no state change, all `discovered`); 002346 (34 B) and 002415 reduced to the oracle's stores. `NxPhysicsJointAllocatorTests` fills blocks with 0xcd: both DLLs exit 0, stdout_delta=0 (candidate faulted before). `NX_PAGE_GUARDED_FILL` on nine page-guarded targets. No registered line, floor or pin changes. Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker (same failure set as HEAD). |
| 6 | 2026-09-25T19:50:00 | 2026-09-25T20:25:00 | 1 | 1,027 | Internal-slot differential (items 4, 5; units/joint-open-items-contract.md `## Internal-slot differential`). New staged-pair target `NxPhysicsJointSlotTests` (phases 6 and 7): 15 joints over all ten families; each internal joint (public +0x18) has slots 4, 1/7/6, 0 and 8 called through its own table by index, under 0x027f and again (step, impulse, projection) under 0x0f7f, revolute's break test (2) last; body +0x204 gets a harness block filled from the body record as 000611 fills it; Scene +0x5bc and the joint's record window reset as 000613/000728 do. Every renderer call, Scene record and changed word of joint, bodies and blocks printed, pointers named. Two defects, both below the slot rows: 000754 (1027 B, the body pose from the centre-of-mass pose, called by 000022 on every projection that moves a body) was a stub, now written; the Foundation's NxFindRotationMatrix rounded to float where the oracle keeps x87 registers and wrote the transpose in its parallel arm, now follows the oracle's stream. Then 1369 transcript lines identical, nothing differs only under 0x0f7f. D6JointDump.txt: same values; UCRT's rounding-mode-dependent `%f` fixed by linking `legacy_stdio_float_rounding.obj` into NxPhysics (review), FLT_MAX's digits a toolchain residual; the dump is read back into the transcript after unload + `_flushall()`. growJointRecords, addJointBreakEvent, 004091 and 004188 made noinline (the oracle calls them). 146 oracle lines registered (134, then 12 dump/control-word lines in review), floors 6/7 = 403/276. 000754's roots through the X87Sqrt.h helpers (review). cdb trace evidence/joint-open-items-trace-slots.txt: 115 rows hit; 74 rows gain dynamic_proof, 000754 moves to reconstructed (Phase 7 ledger reconstructed_not_falsified). Gates 2, 3, 4, 6, 7 pass; 5 red only on its vtables marker. |

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
- **8. Body +0x204 is unbuilt.** Closed by Task 3 as far as the candidate's paths reach. The
  oracle writes body+0x204 in two places. The body constructor 000797 stores 0 there, and the
  candidate now does the same explicitly. The simulation step's 000611 is the only non-zero writer:
  once per step it points +0x204 at the body's element of the Scene's 0x60-byte array at +0x5ac
  (grown by 000600, freed by 000663). That array is what `JointSupportBody` is. The step is not in
  the candidate, so 000600, 000611, 000613, 000708 and 004174/004176 go with it, and until then
  +0x204 stays 0 in both DLLs. Consequence: slot 0's 004358 and 004374 dereference +0x204 with no
  null test, so they cannot run on an unsimulated body in either DLL.
- **3. Rotated-body conventions are untested.** Closed by Task 4. Every family now runs over a
  rotated-body fixture (a 90-degree turn and a general unit quaternion) and the transcript is
  byte-identical to the oracle's, including each internal joint's orientation-dependent words. The
  joint rows' conventions were right: body +0x5c is (x, y, z, w) and +0xdc/+0x134 are row-major,
  as the candidate assumed. What was wrong sat on the candidate's side of the body record and in
  the two joint-descriptor exports; see the Task 4 timing row and
  `units/joint-open-items-contract.md` `## Rotated bodies and near-z axes`.
- **6. Foundation `NxNormalToTangents` defect.** Closed by Task 4 for the joints. With main's
  Foundation fix (560666c, merged at 7ae3022) every family was run over two near-z axes,
  (0.1, 0.2, 0.97) normalised and exactly (0, 0, 1), over the identity fixture and the rotated one.
  All matched on the first run: the saved local normals (from `NxJointDesc_SetGlobalAxis`),
  004101's frame quaternions and prismatic's 004378 words agree with the oracle.
- **10. SEH/GS frames on the deleting destructors.** Closed by Task 5. The cause is the exception
  model, not /GS: the candidate built every file with CMake's default `/EHsc`, and the oracle was
  built without C++ exception handling (its image has no `__CxxFrameHandler`; the only `fs:[0]`
  uses are CRT rows). Under `/EHsc` a destructor is `noexcept`, so each deleting destructor put a
  terminate state around the allocator calls its inlined class `operator delete` makes, and each
  family constructor got unwind states for its constructed base. `/GS` puts a cookie on every
  function with an EH registration frame, which is where the cookie came from. The joint class
  files now take `/EHs-c-` (CMakeLists.txt, after the `/arch:IA32` list); `/GS` is unchanged, and
  the four joint rows with a genuine cookie (a local buffer) keep it. Every `??_G`/`??_E` of the ten
  internal classes, their Np classes, `NpJointShared<>` and `Joint` is now frameless. The internal
  family ones (e.g. against 004368, 004382) reinstall the vptr, delete the public object through
  slot 0 with 1, call the Joint destructor body and free on flag bit 0; the Np ones (e.g. against
  004729) reinstall both vptrs and free on flag bit 0; the `sub ecx,0Ch; jmp` thunks are unchanged. Remaining differences, not EH: (a) an `ebp` frame
  pointer (the build does not omit it); (b) the compiler's `test al,4` branch to
  `__global_delete` (the `::delete` form, which the 2003 compiler did not have); (c) `??_GJoint`
  inlines its destructor body where 004119 calls out (0x95d20); (d) the Np ones inline the
  EmbeddedHookBase destructor where 004729 calls 0x5ba90, and omit the oracle's first vptr store
  (the most-derived table, 0x1011b328) while keeping the `NxJoint` one (0x1011a680 in the oracle);
  both stores are dead. See the Task 5 timing row.
  - **Allocator (Task 5 follow-up).** The review's residual: every joint row the candidate had
    written through `nxGetSdkAllocator()` allocates and frees through `nxFoundationSDKAllocator`
    (`[[0x101041bc]]`, `NxUserAllocator` slots +8 malloc and +0x14 free) in the oracle. Checked
    row by row with a scan of each row's listing for `[0x101041bc]` against calls to 004803
    (`nxGetSdkAllocator`, 0xb4000): the ten family constructors' 0x1c Np allocations (004366,
    004380, 004320, 004300, 004276, 004262, 004234, 004222, 004250, 004210), 000665's ten internal
    allocations, the break events (004111, 004374, 004308), the limit planes (004143 malloc and
    free, 004089 free), 000780's and 000661's pointer-array grow, 000598's record-array grow,
    000760's island free, 000600/000663's body-array and joint-array frees, and every joint
    deleting destructor: all `[0x101041bc]`, none 004803. The candidate now does the same at
    each site (core/*.cpp, Joint.h, NpJointShared.h, JointSupport.h, and in Scene.cpp createJoint,
    000598 and `nxSceneDelete`'s +0x5b8/+0x5ac/+0x58c frees). The two allocators are the same
    object only when NxCreatePhysicsSDK creates the Foundation; they differ when the Foundation
    was created first with another allocator, or when none is passed (the Foundation's default
    against the Physics built-in). `NxPhysicsJointAllocatorTests` (phases 6 and 7) creates the
    Foundation with allocator A and then the SDK with B and creates and releases a revolute and a
    distance joint. Oracle: A gets 0x204, 0x1c, 8 (revolute internal, Np object, the +0x58c array)
    and two frees; 0x184, 0x1c and two frees for distance; B gets nothing. The candidate before the
    follow-up put all of them in B. After: identical to the oracle, 12 lines registered. The
    joint-families plan's rule "allocation through `nxGetSdkAllocator()->malloc`" is superseded
    for joint rows; see `units/joint-open-items-contract.md` `## Joint allocator`.
  - **Scene construction relies on zeroed memory (found by the allocator test; closed by the
    Scene initialisation item below).** With an
    allocator that returns plain `malloc` blocks, the candidate faults in createActor:
    `nxSceneAddActorObject` -> `nxSceneArrayReserve` frees an uninitialised array-header pointer
    of the 0x710-byte Scene block (0xbaadf00d under cdb; heap corruption c0000374), before and
    after this task. The oracle runs clean. Every other harness passes the page-guarded allocator,
    whose fresh pages are zero, which hid it. The allocator test's counting allocator returns
    zeroed blocks so it measures only the joint allocator; the Scene constructor's missing
    initialisation belongs with a faithful 000647.
- **Scene initialisation (closes the allocator test's zeroed-memory finding).** The Scene
  constructor 000647 and its sub-object helpers had been converted from dword indices to byte
  offsets without scaling (98f2625: `p[0x43]` became `nxDword(p, 0x43)`), so they wrote bytes
  0x00..0x1c7 and left every field above that, and every sub-object, to the allocation. Rewritten
  from the listing at byte offsets in the listing's order (units/joint-open-items-contract.md
  `## Scene initialisation`), including the sub-object bases 0x000f0510/0x000f0660/0x000b4fe0/
  0x0002dae0, and the four fields that take Scene+0x50 rather than the Scene's address. 002346
  and 002415 now make exactly the oracle's stores (the candidate over-zeroed). The vtable words the
  sub-objects install are not written. `NxPhysicsJointAllocatorTests` now fills every block with
  0xcd: oracle and candidate identical (before: candidate exit 127 after `scene=created`). Nine
  page-guarded targets run with a 0xcd fill (`NX_PAGE_GUARDED_FILL`); Lifecycle and Dynamics run
  clean under it on both DLLs but differ from their zero-page registrations (aux-array samples the
  oracle leaves unwritten; a changed-word mask), so they keep zero pages. Allocators: 000647's
  NpScene (0x28) already goes through `NxAllocateable::operator new` -> `nxFoundationSDKAllocator`
  (built DLL 0x1002800b: `mov eax,[__imp_nxFoundationSDKAllocator]; call [eax+8]`), as in the
  oracle; only the 0xa8 auxiliary manager (Scene.cpp, the constructor's last allocation) uses
  `nxGetSdkAllocator()`. Candidate-wide, the oracle's scene, actor, record, shape and group allocations (rows in 0x1000..0x28000 and 0x5a000..0x5c000) all use `[[0x101041bc]]`, while the candidate uses `nxGetSdkAllocator()` at about 117 sites. Pre-existing, tracked as a separate follow-up task. Each block is freed through the allocator that made it, so there is no crash risk; the difference is observable only when the Foundation was created with a different allocator.
- **Task 4 review follow-ups.**
  - The two joint-descriptor closures were re-measured with mutations in each row's own code (see
    `evidence/phase6-joints.md` at the end and `gates/phase6-closure.json`).
  - The pose and CMass-offset setters now end in the 000768 reproduction. setGlobalPose and
    setGlobalOrientation use 000196/000200's own conversion.
  - The three global-offset setters form R^T products in their listing order.
  - NxPhysicsActorCMassTests pins all of this over seven rotated bodies (+42 lines, floor 5 = 871).
  - Posed joint fixtures (180 degrees about x and y; largest diagonal x and y) take the pivot arms
    of 000801 and 000768 (+15 lines per joint list, floors 6/7 = 245/118).
- **Precision of the new double/CRT-sqrt code** (Task 4 review). JointDesc.cpp, the Scene.cpp
  creation path and the NpActorDynamicMath.h helpers (000801, 000768, 000746, 000196/000200) keep
  the listing's register values as `double` and call the CRT `sqrt`, compiled for SSE2. These rows
  run only from the public API outside the simulation step, where the control word is the
  process default 0x027f (53-bit precision). There, a double operation and its square root round
  exactly as the x87 register does, so SSE2 reproduces the oracle bit for bit. Under the step's
  0x0f7f (64-bit precision) they would differ, but no step calls them in the candidate.
- **Deferred stubs are silent in Release** (joint-open-items Task 2 review). `NX_ASSERT` is
  `assert` (`Foundation/include/NxAssert.h`) and the Release build defines `NDEBUG`, so every
  `NX_ASSERT(0)` deferred stub in `Physics/src/core` compiles to an empty body: reaching one skips
  the oracle's work without any report. Earlier documents that call these "asserting stubs" mean
  this. The ones remaining after Task 2 are both in `core/JointSupport.cpp`:
  `Row000754Fixture::row000754` (phys_fn_000754, called by 000022) and
  `Row004167Fixture::row004167` (phys_fn_004167, called by 000760's island-object arm). Task 6 wrote 000754; 004167 is the one left.
- **Scene teardown free order** (open; found in the Task 2 review, behaviour unchanged). The
  candidate's `nxSceneDelete` frees the joint record array (+0x5b8) and the joint pointer array
  (+0x58c) directly after its joint-list loops. The oracle's destructor 000663 frees +0x5b8 at
  0x13ffb, after the +0x08/+0x0c arrays and the +0x48 object, and +0x58c at 0x14195, near its
  end. No gate observes the order today; aligning it belongs with a faithful 000663.
- **4. No simulation-path execution.** Closed by Task 6 for every slot a table call reaches.
  `NxPhysicsJointSlotTests` calls each family's internal visualization (4), step (1, 7, 6),
  impulse (0) and projection (8) slots through the object's own table by slot index, in both DLLs,
  with the step's inputs (body +0x204, the Scene record count, the joint's record window) injected
  identically (units/joint-open-items-contract.md `## Internal-slot differential`). Executed and
  matching word for word: the solver slots of all ten families, the projection slots (004356,
  004298, 004207), the visualization slots (004364, 004318, 004312/004314, 004274, 004260, 004232,
  004221, 004200; fixed has none), the impulse slots (004374, 004308, 004219), D6's dump rows
  (004188, 004190, 004192) and the shared 004064, 004093, 004111 (with 004091 and 000571), 004123,
  004135, 004389, 004391, 004393; also 000022, 000598, 000754 and 000758. The differential found two
  defects below the slot rows: 000754 was an `NX_ASSERT(0)` stub (now written), and the
  Foundation's `NxFindRotationMatrix` did not follow the oracle's rounding and wrote the transpose
  in its parallel arm (now follows the oracle's stream). No slot row itself was wrong. Pulley's
  body 0 is the world, so 004228 reads no uninitialised lever; its two-dynamic-body path (which
  reads that lever) is not exercised. D6JointDump.txt: both sides write 77 lines. UCRT's printf
  rounded the 0x0f7f blocks' last digits at the chop mode where the 2003 CRT rounds to nearest;
  NxPhysics now links `legacy_stdio_float_rounding.obj` and that difference is gone. What remains
  is a toolchain residual: FLT_MAX prints as 17 digits and zeros against UCRT's exact integer. The
  oracle's CRT flushes the stream at DLL detach, UCRT only at process exit; the harness unloads
  the pair, calls `_flushall()` and prints the dump into the compared transcript (FLT_MAX as float
  bits), where it matches. Not reached: 004133, which every family
  overrides and only the step's 000728 calls directly, and 004087 (slot 3), which the harness
  does not drive. The step itself (000600, 000611, 000613, 000708, 004174, 004176, 000728) is still
  not in the candidate. cdb trace: `evidence/joint-open-items-trace-slots.txt`.
- **5. PC64 narrowing at the naked x87 helpers.** Measured by Task 6; no difference found. Every
  step, impulse and projection call ran again under the in-step word 0x0f7f (PC 64, RC chop) in
  both DLLs, over the 15 joints (every family; the rows holding every operand the item lists ran:
  distance 004240, spherical 004306/004308/004310/004314, cylindrical 004326, prismatic 004386,
  revolute 004356/004374, D6 004206/004207). All 0x0f7f results match the oracle word for word,
  so no per-site asm was added. After review, 000754's four roots (written by this task) take
  their operands through the X87Sqrt.h helpers in the listing's order, so none is a C-formed
  double narrowed at the qword; each step also prints the control word read back inside it. The narrowing is real in principle (a `double` argument to an
  `x87Fsqrt*` helper is rounded from 64 to 53 bits), but in these cases it never reached a
  stored float. The item stays a known risk for inputs that land on a rounding boundary; it is
  not observable in any registered case.
