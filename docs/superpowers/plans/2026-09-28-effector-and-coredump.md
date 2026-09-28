# Spring-and-Damper Effector and Scene Core Dump — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct the `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` block and its neighbours, about 25 KB of not-started Phase 6 rows, as real source. The block contains:
- the spring-and-damper effector (`NxScene::createSpringAndDamperEffector`, `releaseEffector`, `getNbEffectors`, the `NxSpringAndDamperEffector` API and its solver slots);
- the scene core-dump writer behind `NxPhysicsSDK::coreDump(fname, binary, addendum)` (the `.psc` writer, entry 004062, called from NpPhysicsSDK 000267).

Both are wired to the public API and checked byte-exact against the original DLL.

**Architecture:** Same method as the joint families (`docs/superpowers/plans/2026-09-25-joint-families.md`):
- recover a contract from the Capstone listing;
- write faithful source with stable-ID lines;
- wire it to the public API;
- check it with staged-pair differentials that compare the whole transcript against the original DLL.

The core dump gets an especially strong check: the test dumps a populated scene in both DLLs and prints the resulting file, text and binary, into the compared transcript.

**Tech Stack:** MSVC Win32 Release via CMake, x87 `/arch:IA32` for internal files that do x87 work, Python evidence tools, PowerShell gate runners, cdb for traces.

## Global Constraints

Every Global Constraint of `docs/superpowers/plans/2026-09-25-joint-families.md` applies. Read that section; it binds. The allocation rule in it is superseded by `docs/reconstruction/novodex-physics/units/joint-open-items-contract.md` `## Joint allocator`: allocate exactly as each oracle row does, which for joint and scene rows means the imported `nxFoundationSDKAllocator`. In addition:
- Worktree `D:\github\Novodex\.claude\worktrees\nostalgic-hamilton-a71fa5` (branch `claude/nostalgic-hamilton-a71fa5`, based on `main` at 259dc52). Evidence root `$EV` = `docs/reconstruction/novodex-physics`. Build `cmake -S . -B build -A Win32`, then `cmake --build build --config Release`. This worktree has no build directory yet, so the first task configures one.
- x87 math goes through `Physics/src/include/X87Sqrt.h` / `core/JointAcos.h`. No CRT math where the oracle uses x87 instructions. CRT stdio is used exactly where the oracle calls its static CRT (fopen/fprintf/fputs/fwrite/fclose), as D6's dump does. NxPhysics.dll already links `legacy_stdio_float_rounding.obj`.
- Gates 2, 3, 4, 6 and 7 pass; Phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables`. The gate registry is append-only for pre-existing lines. New registered lines are copied verbatim from the oracle side. Floors and Python pins are updated.
- Ledger convention: rows moved to `reconstructed` get `reconstructed_not_falsified` in their phase's closure ledger, with the standard note.
- Rows with a parameterised model in `Physics/src/ObjectModel.cpp` keep the model; the model only gains a `// Product row:` pointer comment.
- The timing table lives in `$EV/evidence/effector-and-coredump.md` (created in Task 1), with the same columns as `joint-families.md`.
- Progress ledger: `.superpowers/sdd/progress.md` in this worktree.

---

### Task 1: Contract

- [ ] Generate bundles for these units (`$EV/tools/unit_bundle.py`, supplementing missing decompiles with `DecompileSupplement.java` as the joint tasks did):
  - `gap:fluids\NpImplicitMesh.cpp..NpSpringAndDamperEffector.cpp`: only its effector rows 003922–003938; the fluid rows belong to another unit;
  - `NpSpringAndDamperEffector.cpp`;
  - `gap:NpSpringAndDamperEffector.cpp..Joint.cpp`.
- [ ] Write `$EV/units/effector-coredump-contract.md` with these sections:
  - `## Effector`: row assignment, object layouts (the public Np object and the internal effector), dispatch tables, the Scene creation row 000587 and the release/enumeration rows, solver slots and who calls them (step-only?), and the dependency closure.
  - `## Core dump`: the call chain from 000267 through 004062 into every dump row. Which object readers it uses, split into joint members (004068/004070/004072/004081/004083/004145), actors, shapes, materials and meshes, and whether each is already written in the candidate. The text and binary formats, string table and format strings. File handling: fopen mode, where the file is closed, and the addendum. Which scene contents the dump covers.
  - `## Task split`: rows per task for Tasks 2–4, each at most about 12 KB.
- [ ] Create `$EV/evidence/effector-and-coredump.md` with the timing table. Commit.

### Task 2: Effector

- [ ] Write the effector rows (internal and Np) in files named by the census owner: `Physics/src/core/SpringAndDamperEffector.cpp` / `Physics/src/core/NpSpringAndDamperEffector.cpp`, or the owner's actual path. Wire `NpScene::createSpringAndDamperEffector`, `releaseEffector` and `getNbEffectors`, plus any enumeration, through the contract's Scene rows.
- [ ] Test: add effector cases to the joint staged-pair harness, or a new staged-pair target following `NxPhysicsJointSlotTests`' registration. Each case: create over the two-actor fixture; call every public setter and getter, printed as hex words; `is*` queries; count before and after release; release/create cycle. If the solver slots can be reached through the internal object's vtable (as Task 6 of the open-items plan did), call them with identical injected state and print what they write. Register lines copied from the oracle; update floors.
- [ ] Inventory, ledger, trace for `dynamic_proof`, timing row. Commit.

### Task 3: Core dump writer rows

- [ ] Write the dump rows per the contract's split, in address order with stable-ID lines. Put them in the census owner's file; the gap has no named unit, so name it by the `__FILE__`/string evidence or `core/SceneDump.cpp`, and record the choice. Readers the candidate lacks (actor, shape, material or mesh getters used only by the dump) are written if small. Otherwise leave them as deferred stubs that write the same bytes the oracle would for the test scene's objects. Never invent data. If a reader can't be satisfied, the dump case must not include that object type; record it.
- [ ] Build; all gates pass (the writer isn't wired yet). Commit per sub-split.

### Task 4: Wire coreDump and test it

- [ ] Wire `NpPhysicsSDK::coreDump` through 000267 → 004062 as the listing does. That includes the return value, the file-name handling and the addendum.
- [ ] Test: a staged-pair target, or cases in an existing SDK-level harness, that builds a populated scene and calls `coreDump` in text mode and in binary mode, each with and without an addendum. The scene has actors of each shape kind the dump covers, materials, several joint families and an effector. The test then closes/flushes as D6's dump case did, reads the file back and prints it into the transcript (text lines verbatim; binary as hex words). Tokens that differ only through the CRT's FLT_MAX digit residual are printed as float bits, documented as in `NxPhysicsJointSlotTests`.
- [ ] Every difference is a defect in the source: debug it and fix it.
- [ ] Register oracle-sourced lines; update floors and pins. Take a cdb trace for `dynamic_proof`. Commit.

### Task 5: Results

- [ ] Inventory and ledgers for every row written. Regenerate `work_units.json` and the bundles.
- [ ] Complete `$EV/evidence/effector-and-coredump.md`:
  - `## Result`: rows and bytes per part, wired or not, lines registered, rows deferred and why;
  - `## Defects found`;
  - `## Rate`;
  - `## Verification`: fresh configure and clean build, headers, tool tests, gates 2–7, validator, stable-ID check, no CRT math in `core/` beyond the oracle's stdio.
- [ ] Commit.
