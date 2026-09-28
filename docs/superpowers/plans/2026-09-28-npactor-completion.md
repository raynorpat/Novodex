# NpActor.cpp Completion — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the `NpActor.cpp` work unit, the public actor API. It has 87 code rows, of which 53 (about 31 KB) are still `discovered`. Earlier Phase 5 packets already implemented most of those and tested them against the original DLL, but never promoted the rows. Three rows are explicitly unimplemented (`updateMassFromShapes` 000164, `setDynamic`, `setGlobalPose`) and four are noted as partial (000196, 000198, 000200, 000202).

**Architecture:**
1. Audit each not-started row: map its oracle row to the candidate function, check it against the listing, and find which existing Phase 5 staged-pair targets exercise it.
2. Prove execution with cdb traces of the candidate over those targets.
3. Finish the partial and missing rows from the listing.
4. Add staged-pair cases for rows no target reaches.
5. Promote per the evidence and record the results.

**Tech Stack:** MSVC Win32 Release via CMake, cdb, PowerShell gate runners, Python evidence tools.

## Global Constraints

- Worktree `D:\github\Novodex\.claude\worktrees\nifty-meitner-27ac02` (branch `claude/nifty-meitner-27ac02`). Evidence root `$EV` = `docs/reconstruction/novodex-physics`. Other sessions may be working on other units (the qhull gap), so touch only NpActor-related code, tests, docs and the rows of this unit in the shared JSON files.
- Public headers under `Physics/include/**` and `Foundation/include/**` are immutable. Product code must not forward to, load or read the oracle DLL.
- The stable-ID comment convention used in this file must be kept. Every product function gets a comment naming its stable ID and RVA. New comments use the exact form `// phys_fn_NNNNNN (0x%08x, N B)` on its own line, and existing prose comments may stay.
- Faithfulness: the Capstone listing (`$EV/oracle/capstone/manifest.json`) is authoritative over decompiles. Follow the project's x87 conventions: register lifetimes are `double`, spills are `NxReal`, and the listing's operation order is kept. Use `Physics/src/include/X87Sqrt.h` for square roots and never CRT math where the oracle uses x87 instructions. Allocation goes through the allocator the oracle row uses: `[0x101041bc]` means `nxFoundationSDKAllocator`, and 004803 means `nxGetSdkAllocator`.
- No row is promoted above `reconstructed`. Ledger reasons in `$EV/gates/phase5-closure.json` change only as the validator requires for rows whose state changes. `reconstructed_not_falsified` is the reason for newly reconstructed rows.
- No existing expected line in `$EV/tools/gate_targets.ps1` is edited. New registered lines are copied verbatim from the oracle side. Floors and the Python pins are updated to match.
- `dynamic_proof` requires a committed cdb trace excerpt with the candidate DLL's sha256. `static_proof` records the listing review.
- Gates 2, 3, 4, 6 and 7 must pass, and Phase 5 must fail only on `candidate CANDIDATE-MISSING family=vtables`, at the end of every task.
- Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- A timing table in `$EV/evidence/npactor-completion.md` (created in Task 1) has the same columns as `joint-families.md`.

---

### Task 1: Audit, contract and trace

- [ ] For each of the unit's 87 code rows (53 `discovered`), produce `$EV/units/npactor-contract.md` with a table:
  - stable ID, RVA and size;
  - the public method it implements, from the NpActor public vtable in the PE and inventory data objects, or its role for non-vtable helpers;
  - the candidate function and file:line;
  - status: `implemented`, `partial` (and what is missing), or `missing`;
  - the Phase 5 staged-pair targets whose cases call it;
  - a listing-review verdict: walk each implemented row's oracle listing against the candidate source and record `faithful`, or a defect with its addresses.
- [ ] Run cdb breakpoint traces of the candidate DLL over every Phase 5 actor staged-pair target, following the method in `$EV/evidence/joint-open-items-trace-slots.txt`. Record the hit rows and commit the excerpt `$EV/evidence/npactor-trace.txt` with the sha pin.
- [ ] Create `$EV/evidence/npactor-completion.md`. Commit.

### Task 2: Cross-cutting pass (contract `## Conventions` G1, E1, H1, SSE)

- [ ] **Error reports (G1/E1).** Reproduce the oracle's lock-failure (G1) and invalid-argument/state (E1) reports on every NpActor row that has them, following 000128. Match the code, file string, line, message and order, including the inline `FoundationSDK::instance` null check (`int3`) before each report. The report and lock order must follow each row's listing (for example 000204 checks E1 before taking the lock).
- [ ] **H1.** Rewrite `nxNpActorMarkRecordDirty` from the oracle's inline growth sequence: no null or id bounds tests, 2n+2 growth, `nxFoundationSDKAllocator`, and no `if(!grown)`. Fix `nxNpActorTransitionKinematic` (000785/000787) to do its dirty marks and list growth before the malloc, and to use the Foundation allocator for the 0x20 state.
- [ ] **x87 order.** Put `Physics/src/NpActor.cpp` on the `/arch:IA32` list, then fix the float expressions the contract names (000150 and the others) to the listing's order with `double` register lifetimes.
- [ ] Add staged-pair cases to the Phase 5 actor targets for the error paths (invalid args, static/kinematic misuse, lock-held calls if the harness can hold a lock) and for H1's growth past the initial capacity. Register oracle-sourced lines and update floors and pins. Commit.

### Task 3: Row-level defects

- [ ] Fix every substantive row defect in the contract, grouped as the contract groups them:
  - 000204-000208 (000756/000789 models, the 000004 shape update with the group's 001018/001315);
  - 000124/000126/000090 (CMass-frame composition, 000784 wake);
  - the wake blocks in 000174/000176/000180/000182;
  - 000168 (`_fpclass` gating);
  - the summation orders (000182, 000134, 000060/000742, …);
  - 000746 (the world tensor);
  - the rest.
  Write the helper rows the fixes need where the contract assigns them (000789, 000746, 000784, 000756, 000785/000787, 000782). Follow the listing, and add staged-pair cases that reach each fixed branch (wake thresholds, NaN/denormal masses, grouped actors). Commit per group.

### Task 4: Shape add/remove (000070/000072)

- [ ] Write the shape add and release chain from the listing: 000036, 000024, 000032, 001041, 001033, 001028, 000006, 001941, 000503, 003628, 000012, 001957 and 001960, as the contract's dependency chains require. Reconcile with the candidate's existing partial implementation and the Phase 5 shape-mutation differential. Add cases for every shape type and for grouped actors. Commit.

### Task 5: Mass from shapes and setDynamic (000164, 000122)

- [ ] Write 000008 and 001397, and audit the candidate shapes' slot-4 dispatch against the mass wrappers 000947/001008/001371/001024/001249. Then write 000164.
- [ ] Write the record lifecycle rows: 000026 (000797, 000630), 000776 (000722), 000632 (004103) and 000531/000533/001943/001279. Decide whether they replace Scene.cpp's createActor density approximation; they should, if the listing shows the same path. Then write 000122 (`setDynamic`).
- [ ] Add cases: updateMassFromShapes over each shape type, with density and mass descriptors, and setDynamic on a static actor with and without shapes. Commit.

### Task 6: Coverage for untraced rows

- [ ] Re-run the NpActor trace over all Phase 5 targets. Add cases for any row still unexecuted. Commit.

### Task 7: Promotion and results

- [ ] Promote rows with a `faithful` review and trace hits to `reconstructed`, with `static_proof` (the review) and `dynamic_proof` (the trace). `implementation`/`source` name `Physics/src/NpActor.cpp`, which must contain the stable ID. Update the Phase 5 ledger and run the validator.
- [ ] Regenerate `work_units.json` and the NpActor bundle. Complete `npactor-completion.md` with:
  - rows and bytes promoted;
  - rows left and why;
  - defects found and fixed;
  - the rate;
  - verification: fresh build, headers, tool tests, validator, gates 2–7.
- [ ] Commit.
