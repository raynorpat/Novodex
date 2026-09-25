# Joint Open Items — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the open items the joint-families work left (`docs/reconstruction/novodex-physics/evidence/joint-families.md` `## Open items carried forward`, items 1–12).

**Architecture:** Same approach and rules as `docs/superpowers/plans/2026-09-25-joint-families.md`: recover the contract from the oracle listing, write faithful source, and check behaviour with staged-pair differentials against the original DLL. New here: a direct internal-slot differential. The test reaches each joint's internal object through the public object (`+0x18`) and calls its visualization, projection and solver virtuals in both DLLs. This exercises the rows that no simulation step reaches yet.

**Tech Stack:** MSVC Win32 Release via CMake, x87 `/arch:IA32` for internal joint files, Python evidence tools, PowerShell gate runners, cdb for traces.

## Global Constraints

Every Global Constraint of `docs/superpowers/plans/2026-09-25-joint-families.md` applies unchanged. Read that section first; it covers the stable-ID form, immutable public headers, no oracle forwarding, `reconstructed` as the ceiling, oracle-sourced registered lines only, `/arch:IA32` for internal joint files, the x87 rules, SDK-allocator allocation, links by value, and `dynamic_proof` only with execution evidence. In addition:
- Worktree `D:\github\Novodex\.claude\worktrees\reconstruction-progress-5b3127`. The branch already contains main's Foundation `NxNormalToTangents` fix (560666c) and its test-target build fix (956b6d3), merged at 7ae3022.
- Phase gates 2, 3, 4, 6 and 7 must pass at the end of every task. Phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables`.
- x87 math goes through the naked helpers (after Task 1: `Physics/src/include/X87Sqrt.h`) and `core/JointAcos.h`. No CRT math in `core/`.
- Ledger convention: rows moved to `reconstructed` get reason `reconstructed_not_falsified` in their phase's closure ledger, with the standard note. This applies to Phase 2 and Phase 7 rows too, e.g. the Scene rows.
- Every task appends a row to a new timing table in `docs/reconstruction/novodex-physics/evidence/joint-open-items.md` (Task 1 creates it). The table has the same columns as `joint-families.md`. Each task also marks the open items it closes.

---

### Task 1: Test targets, dead generic path, helper consolidation (open items 7, 11, 12)

- [ ] Add `Physics/src/core/*.cpp` (and whatever they need at link time) to `NxPhysicsInternalTests` and `NxPhysicsCollisionTests` in `CMakeLists.txt`. Keep each core file's `/arch:IA32` property: set_source_files_properties is directory-scoped, so verify that the generated vcxproj of each test target gives the internal joint files `NoExtensions`. Gates 2 and 3 must pass.
- [ ] Remove the dead generic createJoint path if nothing reaches it: `nxJointConstruct`, `nxJointSizeForType`, `nxJointDestroy`, and `NpJointObject`/`NpJointVtable` in `Physics/src/NpJoint.cpp` / `Physics/src/include/NpJoint.h`. Grep every user first, including tests and ObjectModel. Keep anything a test or model still uses, and say why. Any inventory row whose `implementation` points at removed code is repointed or noted. Validator exits 0.
- [ ] Fold `Physics/src/include/core/JointX87.h` into `Physics/src/include/X87Sqrt.h`: one set of helpers, renamed call sites, same instruction bodies. Confirm by disassembly that two representative helpers are byte-identical before and after.
- [ ] Create `evidence/joint-open-items.md` with the timing table. Commit.

### Task 2: Scene joint registration, release and the deferred Scene rows (open items 1, 2, 9)

- [ ] **Contract.** Add `units/joint-open-items-contract.md` `## Scene joint rows`, covering:
  - 000661 (Scene::addJoint, 0x13e00, 290 B), 000633 (0x12660, 370 B), 000653 (0x13760, 126 B), 000598 (0x10f50, 138 B), 000758 (0x17630, 214 B), 000022 (0x1840, 27 B) and 000571 (0x108e0, 22 B: product body; a model exists);
  - the NpScene release row behind `NxScene::releaseJoint` (find it from the NpScene public table);
  - any joint enumeration rows (`getNbJoints`, `resetJointIterator`, `getNextJoint`) that the release test needs.
  For each row give its purpose, the fields it writes (including `Joint::mScene` +0x30, the scene joint list, and the Scene+0x5b8 record array), its callers, and write/defer.
- [ ] **Write the rows** in `Physics/src/Scene.cpp`, `Physics/src/NpScene.cpp` or `core/JointSupport.cpp`, following the census owner's file. Replace the asserting stubs and the no-op `nxSceneAddJoint`.
- [ ] **Wire `NpScene::releaseJoint`** to the oracle's chain. Every family's release chain is already written: internal deleting destructor → 004095 → 000633.
- [ ] **Test.** Extend the joint test so each family case prints what the scene reports about its joints before and after release: count, enumeration, and anything else observable through the public API. Add a release-then-create cycle. Register oracle-sourced lines. The staged pair must report `stdout_delta=0`. Take a cdb trace of the release path and record `dynamic_proof` only for rows it shows executing.
- [ ] Inventory, ledgers (Phase 2/7 rows too), timing row, commit.

### Task 3: Body record +0x204 (open item 8)

- [ ] Find the oracle writer(s) of the body record's `+0x204` (`JointSupportBody*`): scan the Capstone listing for stores to `[reg+0x204]` in body/actor/scene rows, then follow the allocation and initialization of the `JointSupportBody`. Add a contract section.
- [ ] Implement it in the candidate's body-record construction and teardown (Scene.cpp, around lines 1800–1920, and the actor release path), with layout asserts.
- [ ] The existing actor and joint gates (5, 6 and 7) must stay unchanged. If a public observable exists, add a probe line. Inventory, timing, commit.

### Task 4: Rotated bodies and near-z axes (open items 3, 6)

- [ ] Add a rotated-body fixture: both actors with non-identity global orientations, including one 90° and one arbitrary quaternion. For every family, add cases that create the joint over the rotated bodies and print the same fields as the existing cases.
- [ ] Add near-z axis cases (|axis.z| > 0.707) now that Foundation is fixed.
- [ ] Any transcript difference is a defect: debug it (superpowers:systematic-debugging) and fix the joint code or the candidate's body-record conventions (+0x5c quaternion, +0xdc 3×3), whichever the oracle listing shows is wrong. Register oracle-sourced lines; update floors. Timing, commit.

### Task 5: SEH/GS frames on deleting destructors (open item 10)

- [ ] Find out why the candidate's `??_G`/`??_E` for the joint classes carry an SEH frame and a /GS cookie while the oracle's are frameless. Candidates: /EHsc with members or bases that have non-trivial destructors, `operator delete` placement, /GS on these TUs, `__declspec(novtable)`. Build small experiments in a scratch copy, not the product tree.
- [ ] Apply the narrowest faithful fix, per class or per TU. Do not disable /GS broadly without a stated reason; the pilot plan forbids broadening `/GS-` as a workaround. Verify by disassembly that the deleting destructors are frameless and the call sequence matches the oracle rows (e.g. 004729, 004382). All gates pass. Timing, commit.

### Task 6: Internal-slot differential for unexecuted rows (open items 4, 5)

- [ ] **Harness.** Add a new test (or cases in `tests/PhysicsJointTests.cpp`) that, for each family's created joint, reaches the internal object from the public object (`+0x18`) and calls its internal virtual slots directly through the object's own vtable, by slot index and never by RVA, so the same code drives both DLLs:
  - visualization (slot 4), with a recording `NxDebugRenderable` that prints every point, line, triangle and arrow as hex words;
  - projection (slot 8), where the family has one;
  - the solver slot (slot 6, NxReal argument), then printing the Scene record array (+0x5b8) entries the call wrote, as hex words.
  Set the SDK visualization parameters the rows read. Run every call under the default control word, and run the solver and projection slots again under 0x0f7f (set with `_controlfp_s` around the call and restored after).
- [ ] **D6's dump file.** `D6JointDump.txt` is written to the working directory by both sides. Compare its content between the oracle and candidate runs if the runner allows, otherwise record it.
- [ ] **Defects.** Every difference is a defect in the source: debug and fix. This includes differences that only show under 0x0f7f; that is where open item 5, the PC64 narrowing at the helpers, would appear. If a PC64 difference appears, fix the affected sites so the operand stays in st(0) (a per-site asm block). If none appears across the cases, record the measured result in the open items.
- [ ] Register oracle-sourced lines and update floors. Record `dynamic_proof` for rows a cdb trace shows executing. Update inventory and ledgers, add a timing row, commit.

### Task 7: Results

- [ ] Regenerate `work_units.json` and the joint bundles.
- [ ] Complete `evidence/joint-open-items.md`:
  - each open item's disposition (closed with evidence, or still open with the reason);
  - defects found by the new differentials;
  - the rate;
  - verification: fresh configure and clean build, headers, tool tests, gates 2–7, the stable-ID check, and no CRT math in core/.
- [ ] Update `joint-families.md` so each open item points at its disposition. Commit.
