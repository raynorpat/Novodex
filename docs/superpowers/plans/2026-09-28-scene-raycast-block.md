# SceneRaycast..CapsuleShape Block — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the work units `SceneRaycast.cpp` (10 rows, all not started), `gap:SceneRaycast.cpp..CapsuleShape.cpp` (80 of 141 code rows not started, about 38 KB) and `CapsuleShape.cpp` (1 row). The gap mixes scene raycast internals, body/actor math and creation helpers, island helpers and shape helpers. The joint work already implemented several of them as helpers without claiming them (000746, 000768, 000795, 000797, 000801 in `Physics/src/include/NpActorDynamicMath.h`), so the first job is an audit.

**Architecture:**
1. Audit every row in the three units. Map each oracle row to its candidate function, if any, check it against the listing, and find which existing staged-pair targets reach it.
2. Trace the existing targets to get execution evidence.
3. Write the missing rows by sub-area, including the public scene raycast API if the audit shows it is missing.
4. Add staged-pair cases for rows no target reaches.
5. Promote per the evidence and record results.

**Tech Stack:** MSVC Win32 Release via CMake, cdb, PowerShell gate runners, Python evidence tools (`unit_bundle.py`, the Capstone and Ghidra manifests, `oracle/ghidra/supplement.json` via `ghidra/DecompileSupplement.java`).

## Global Constraints

- Worktree `D:\github\Novodex\.claude\worktrees\optimistic-bhaskara-d2cc69` (branch `claude/scene-raycast-block`, from `main` at 259dc52). Evidence root `$EV` = `docs/reconstruction/novodex-physics`.
- **Other sessions are working in parallel.** Do not edit rows or files they own:
  - `NpActor.cpp` unit rows (0x2610–0xb100): worktree `nifty-meitner-27ac02`;
  - the effector/core-dump gap before `Joint.cpp`: `nostalgic-hamilton-a71fa5`;
  - the qhull gap: `reconstruction-progress-5b3127`.

  Shared files will need merging later: `Scene.cpp`, `NpScene.cpp`, `NpActorDynamicMath.h`, `inventory.json`, the ledgers and `gate_targets.ps1`. Keep edits to shared files minimal and localized, and never reformat them.
- Public headers under `Physics/include/**` and `Foundation/include/**` are immutable. Product code must not forward to, load or read the oracle DLL.
- Every product function claimed for a row carries a comment line naming its stable ID in exactly this form: `// phys_fn_NNNNNN (0x%08x, N B)` (nothing else on the line). No other comment line may begin with `// phys_fn_`. Helper functions already written for these rows get that line when claimed.
- Faithfulness: the Capstone listing (`$EV/oracle/capstone/manifest.json`) is authoritative over decompiles. x87 conventions apply:
  - register lifetimes are `double`, spills are `NxReal`, and the listing's operation grouping is kept;
  - sqrt and similar go through the naked helpers in `Physics/src/include/X87Sqrt.h`; no CRT math where the oracle uses x87 instructions;
  - files whose rows run under the simulation step's control word go on the `/arch:IA32` list with a stated reason.
- Allocation follows each oracle row's own allocator. `[0x101041bc]` means the imported `nxFoundationSDKAllocator`; row 004803 means `nxGetSdkAllocator()`. Malloc and free are always paired on one allocator.
- No row is promoted above `reconstructed`. Ledger reasons in `$EV/gates/phase{2,3,5,7}-closure.json` change only as the validator requires for rows whose state changes; `reconstructed_not_falsified` is the reason for newly reconstructed rows.
- No existing expected line in `$EV/tools/gate_targets.ps1` is edited. New registered lines are copied verbatim from the oracle side; floors and the Python pins are updated to match.
- `dynamic_proof` requires a committed cdb trace excerpt with the candidate binary's sha256 (method: `$EV/evidence/joint-open-items-trace-slots.txt`). `static_proof` records the listing review.
- Gates 2, 3, 4, 6 and 7 pass, and Phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables`, at the end of every task.
- Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- The timing table lives in `$EV/evidence/scene-raycast-block.md` (created in Task 1), with the same columns as `joint-families.md`.

---

### Task 1: Audit, contract and trace

- [ ] Generate bundles for the three units (`$EV/tools/unit_bundle.py`). Run the supplement for any row that has no `ok` decompile; pass the union of the existing `requested` RVAs, since the script overwrites the file.
- [ ] Write `$EV/units/scene-raycast-contract.md`. For every code row in the three units (all states), give:
  - stable ID, RVA and size;
  - its role: public method (from the NpScene public vtable and inventory data objects), callers and callees, and strings;
  - its sub-area (scene raycast / body-actor math / island / shape / other);
  - the candidate function and file:line, found by grepping the stable ID, helper names and behaviour;
  - status: `implemented`, `partial` (with what is missing) or `missing`;
  - the staged-pair targets that reach it;
  - a listing-review verdict for implemented rows: `faithful`, or a defect with addresses.
- [ ] Run cdb traces of the candidate over every Phase 5 and Phase 7 staged-pair target, plus the joint targets, and record hit rows. Commit the excerpt `$EV/evidence/scene-raycast-trace-audit.txt`.
- [ ] Create `$EV/evidence/scene-raycast-block.md` with an audit summary (counts by status and sub-area) and the timing table. Commit.

### Task 2: Promote the implemented, faithful, executed rows

- [ ] Fix any defect Task 1 found in an implemented row, then re-review and re-trace it.
- [ ] Claim each helper with its stable-ID line.
- [ ] Promote `implemented` + `faithful` + executed rows to `reconstructed`. Record the static proof (listing review) and the dynamic proof (trace), set implementation and source, and update the ledgers.
- [ ] Run the validator and the gates. Commit.

### Task 3: Scene raycast

- [ ] Write the missing scene-raycast rows (`SceneRaycast.cpp` and the raycast internals the contract identifies). Wire the public `NxScene` raycast methods through `NpScene` if they are placeholders.
- [ ] Add a staged-pair target covering:
  - `raycastAnyShape`, `raycastClosestShape` and `raycastAllShapes`, with an `NxUserRaycastReport` that records every callback as hex words;
  - each shape type in the scene (box, sphere, capsule, plane, and triangle mesh if a mesh can be built on both sides);
  - hit and miss cases, and rays starting inside a shape;
  - groups and masks, and max distance.
- [ ] Register oracle-sourced lines. Trace, then promote per the evidence. Commit.

### Task 4: Remaining missing rows

- [ ] Write the remaining `missing` or `partial` rows by sub-area (body/actor math, island, shape, other), following the listing.
- [ ] Add staged-pair cases for any row no target reaches, where a public path exists.
- [ ] Trace, then promote per the evidence. Rows reachable only from the simulation step stay source-only `reconstructed` with a static proof, following the joint work's precedent. State that in each proof. Commit.

### Task 5: Results

- [ ] Regenerate `work_units.json` and the three bundles.
- [ ] Complete `scene-raycast-block.md`:
  - rows and bytes moved per sub-area;
  - defects found;
  - rows left, and why;
  - the rate;
  - verification: fresh configure and clean build, headers, tool tests, validator, gates 2–7, and the stable-ID check.
- [ ] Commit.
