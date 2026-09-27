# Vendored qhull/OPCODE Correspondence — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove, row by row, that the candidate's vendored qhull 2003.1 and OPCODE 1.3 code (plus NovodeX overlays) corresponds to the oracle's. Fix real differences, write the NovodeX-only rows, and move proven rows out of `vendored_not_falsified`. This covers about 375 KB of the remaining not-started census bytes: the `gap:opcode\IcePrunable.cpp..opcode\OPC_MeshInterface.cpp` and `gap:Controller.cpp..fluids\Fluid.cpp` units.

**Architecture:** Automated structural matching, not hand transcription. The existing maps `docs/reconstruction/novodex-physics/evidence/phase4-third-party-map/{qhull,opcode}_map.csv` already pair each oracle row with an upstream source function (`grade` = mapped / probable / unmapped). A new tool disassembles each mapped row in the oracle and its counterpart in the candidate DLL, and compares normalized features:
- the call sequence, resolved to function identities;
- string and float constants by value;
- the integer immediate multiset;
- branch structure.

Rows that differ are triaged by hand. Execution evidence comes from cdb traces of the existing third-party and asset differentials.

**Tech Stack:** Python (capstone, pefile, unittest), MSVC Win32 Release via CMake, cdb, PowerShell gate runners.

## Global Constraints

- Worktree `D:\github\Novodex\.claude\worktrees\reconstruction-progress-5b3127` (branch `claude/reconstruction-progress-5b3127`, currently equal to `main` at b210042). Evidence root `$EV` = `docs/reconstruction/novodex-physics`.
- Public headers are immutable. Upstream trees under `External/*/upstream` are immutable. NovodeX changes go in `External/*/novodex/` overlays, recorded in `External/*/MODIFICATIONS.md` (see `External/README.md` and `$EV/tools/verify_vendored_sources.py`).
- Product code must not forward to, load or read the oracle DLL.
- No row is promoted above `reconstructed`. Ledger reasons change only as the validator requires for rows whose state changes.
- No existing expected transcript line in `$EV/tools/gate_targets.ps1` is edited to make a gate pass. New registered lines are copied verbatim from the oracle side.
- `dynamic_proof` requires committed execution evidence (a cdb trace excerpt with the candidate DLL's sha256). Static structural matching goes in `static_proof`.
- Gates 2, 3, 4, 6 and 7 pass, and Phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables`, at the end of every task.
- Python tools follow `$EV/tools` style: 4 spaces, a module docstring, and `main()` returning 0, 1 or 2. Tests live in `$EV/tools/tests/`.
- The timing table lives in `$EV/evidence/vendored-correspondence.md` (created in Task 1), with the same columns as `joint-families.md`.
- Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

---

### Task 1: The structural matcher

- [ ] Write `$EV/tools/vendored_match.py` with unit tests. For every map row graded `mapped` or `probable`:
  - Locate the candidate function in `build/Release/NxPhysics.map` by `source_function`. Handle C names (`_qh_*`) and C++ mangled names (OPCODE and ICE), using demangling or a normalized-name index. Take the function extent from the map ordering and the PE.
  - Disassemble the oracle row (row extent from `$EV/inventory.json`, bytes from the PE) and the candidate function.
  - Extract features:
    - direct call targets, in order, resolved to identities (oracle: row → map `source_function`; candidate: symbol name). Imports and indirect calls are recorded as such.
    - referenced strings, by content;
    - float and double constants, by value;
    - the integer immediate multiset, excluding stack offsets;
    - global data references, as relocation targets classified by what they point to;
    - conditional branch count;
    - x87 versus SSE use.
  - Classify each row:
    - `MATCH`: every feature equal.
    - `SHAPE`: same calls, constants and data refs, but different branch or op counts (compiler shape only).
    - `DIFF`: calls, constants or data refs differ (list them).
    - `MISSING`: no candidate symbol, e.g. inlined in the candidate; name the candidate caller that inlines it, if found.
  - Output: `$EV/evidence/phase4-third-party-map/{qhull,opcode}_match.csv` and a summary.
- [ ] Unit tests use synthetic byte strings and a tiny fake map. Cover call resolution, constant extraction, each classification, and MISSING.
- [ ] Run it on both libraries. Commit the tool, tests, CSVs and a first summary in `$EV/evidence/vendored-correspondence.md`.

### Task 2: Triage qhull

- [ ] For every qhull `DIFF` and `MISSING` row, decide the cause:
  - a NovodeX modification not yet in the overlays (e.g. the allocator redirect through the global at `0x10125080`);
  - an upstream-version difference;
  - a compiler artifact such as inlining, folding or CRT differences;
  - a real candidate defect.
  Fix real differences in `External/qhull/novodex/` overlays and record them in `MODIFICATIONS.md`. Spot-check 10 `SHAPE` rows by hand against the listing.
- [ ] Unmapped qhull rows (37, including the NovodeX driver 003279 and the entries 003255/003413): write any the candidate lacks, reconstruct-style (bundle, listing, faithful source), in the overlay or in `Physics/src`. The census owner decides which.
- [ ] Re-run the matcher. The third-party differential (Phase 4) and all gates must stay green. Commit.

### Task 3: Triage OPCODE

- [ ] Same as Task 2 for OPCODE. The NovodeX modifications are listed in `$EV/evidence/phase4-third-party.md` §2.3: the extended OPCODECREATE/BuildSettings, the build-from-blob path, tree serialization, the single-triangle short circuit, IcePrunable.cpp and IceAdjacencies.cpp. The 149 unmapped rows are NovodeX or ICE code. Many were reconstructed in Phase 4 Task 2b, so check inventory states before writing anything. Commit.

### Task 4: Execution coverage

- [ ] Run cdb breakpoint traces of the candidate over the existing Phase 4 differentials (`NxPhysicsThirdPartyTests`, `NxPhysicsAssetTests`) with a breakpoint on every matched vendored function. Record hit rows.
- [ ] If important families have no hits, add oracle-sourced cases to the existing third-party differential: convex hull cooking through qhull, and mesh build, deserialize and query through OPCODE, at several sizes including degenerate inputs. The transcript must stay byte-identical. Commit trace excerpts under `$EV/evidence/`.

### Task 5: Inventory, ledger, results

- [ ] For each vendored row:
  - `MATCH`/`SHAPE` rows with no unresolved triage item → `reconstructed`. `implementation` and `source` point to the upstream or overlay source file that defines the function. `static_proof` records the matcher result and features compared, plus the triage note for SHAPE rows. `dynamic_proof` is set only for traced rows.
  - `DIFF` rows fixed in Tasks 2 and 3 → the same, citing the fix commit.
  - Anything unresolved stays `discovered`, with the reason recorded.
- [ ] Update the Phase 4 ledger per the validator (`vendored_not_falsified` → the reason matching the new state). Validator exits 0.
- [ ] Regenerate `work_units.json` and bundles for the two units. Complete `vendored-correspondence.md`: counts by class, fixes, rows left and why, the rate, and verification (fresh build, gates 2–7, headers, tool tests, `verify_vendored_sources.py`). Commit.
