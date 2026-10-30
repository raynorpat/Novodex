# Remaining Discovered Rows Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every one of the 270 `discovered` code rows of the pinned `NxPhysics.dll` gets a candidate implementation with
a static proof (or, for vendored rows the promotion policy holds back, a reasoned entry on the hand-off list) before
any further testing work.

**Architecture:** First-party rows become naked `__asm` listing transcriptions generated from capstone listings, proven
by a per-cluster listing-compare script and recorded in `inventory.json` and the phase closure ledger by one shared
tool. Vendored qhull/OPCODE rows go through the existing matcher and promotion policy, with source fixes in the
`External/<lib>/novodex/` overlays. Work proceeds in six stages in phase order, with gates 3–7 at each stage exit.

**Tech Stack:** MSVC Win32 Release (CMake, `build/`), naked `__asm`, Python 3 with capstone and pefile, the
reconstruction tooling in `docs/reconstruction/novodex-physics/tools/` (unittest under `tools/tests`), PowerShell phase
gates.

**Spec:** `docs/superpowers/specs/2026-10-30-remaining-rows-design.md`

## Global Constraints

- Oracle: `D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll`, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- Done means static proof only: no oracle differentials, no mutation sweeps, no new gate coverage lines.
- A first-party row is done when its listing-compare script prints `ALL EQUAL` and `MAPPINGS checked=N unexpected=0`, and
  it is `reconstructed` in `inventory.json` with `source`, `implementation`, `implementation_symbol`,
  `candidate_map_reference`, `static_proof`, an empty `dynamic_proof`, and a note that it awaits the testing pass.
- Its closure entry moves from `not_reconstructed_in_phase` to `reconstructed_not_falsified`, and the ledger's `counts` are updated to match.
- Vendored rows follow `evidence/vendored-correspondence.md` "Promotion policy for Task 5" unchanged. A row it holds back
  for execution evidence stays `discovered` and goes on the hand-off list (Task 9); it is not forced.
- Every commit: full Release build clean; `python tools/validate_inventory.py ../inventory.json` prints `unexplained=0`;
  `python tools/work_units.py …` and `python tools/report_completion.py …` regenerate `work_units.json` and
  `completion-backlog.json`; `python -m pytest tests -q` (from `tools/`) passes with at least 857 tests.
- Gates 3 to 7 (`tools/run_phase_gate.ps1 -Phase N`) pass at every stage exit.
- Generated `.cpp` files are CRLF. Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Stage `git add` explicitly by path: the working tree carries unrelated untracked files (`candidate/`, `oracle/`, `*.psc`, `canary*.txt`).

## Review Focus

1. **Two inventory ids, one function.** The inventory splits some functions at a jump target (as with 005244/005246).
   Both ids must map to the one candidate symbol, and the compare script must cover the whole function once. Test:
   `test_split_rows_share_one_symbol` (Task 1).
2. **A vendored row given to the first-party tool.** That would bypass the promotion policy. Test:
   `test_refuses_vendored_rows` (Task 1).
3. **Re-pointing a C++ stand-in** changes behaviour that an existing family already gates. Expected: the gates stay
   green, or the commit explains which side was wrong. Covered by the stage-exit gate step in each stage task.
4. **A row that jumps into the middle of another row**, or tail-jumps out of itself. Expected: the compare script's
   `expected` names the target and the mapping check passes, with no `__emit` unless it is listed with a reason.
   Covered by the per-cluster compare step.
5. **Closure count drift.** Moving an entry without updating `counts` leaves the ledger inconsistent. Test:
   `test_closure_counts_follow_the_move` (Task 1).

## File Structure

- `docs/reconstruction/novodex-physics/tools/transcribe_rows.py` (create): `gen11.py` plus `mkfile.py`, promoted from
  the raw bundles to one tool. Usage: `transcribe_rows.py <cfg.py> <dest.cpp> [--header head.txt | --append]`.
- `docs/reconstruction/novodex-physics/tools/close_static_rows.py` (create): generalises the session's `close_rows12.py`
  without the mutation evidence, writing inventory fields and closure entries from a manifest.
- `docs/reconstruction/novodex-physics/tools/tests/test_transcribe_rows.py` and `test_close_static_rows.py` (create).
- Per cluster:
  - the subsystem's existing naked-row file, or a new `Physics/src/<area>/<Topic>Rows.cpp` added to the four CMake source lists next to `PrunerOctree.cpp`;
  - `evidence/phase<N>-<topic>-listing-compare.py`;
  - `evidence/phase<N>-<topic>.md`;
  - `evidence/raw/phase<N>-<topic>-<date>/` (the cfg, the header, `listing-compare.txt`).
- Stage 2: `External/qhull/novodex/*.c` and `External/opcode/novodex/*.cpp` overlays; `evidence/phase4-third-party-map/*`; `evidence/vendored-correspondence.md`.
- `docs/reconstruction/novodex-physics/evidence/remaining-rows-handoff.md` (create in Task 9).

## The cluster procedure

Tasks 3 to 8 run this for every cluster they list. It is written once here.

1. **Place.**
   - List the cluster's rows and their direct callers from `completion-backlog.json`.
   - If the callers' `implementation.path` is a naked-row file of one subsystem, add the rows there.
   - Otherwise create `Physics/src/<area>/<Topic>Rows.cpp`, with `<area>` taken from the callers' paths and `<Topic>` from what the rows do. Add it to the four CMake lists, including the `COMPILE_OPTIONS` `set_property`.
2. **Config.** Write `cfg_<topic>.py` for `transcribe_rows.py`:
   - `ROWS` uses each row's inventory range, callees first, with one entry for a split function;
   - `CALLS` maps targets to candidate aliases (`/alternatename`) or in-file names;
   - `DATA` and `STRINGS` name oracle globals and constants;
   - `TABLES` lists jump tables;
   - `PREFIX` is a constant prefix unique to the file.
3. **Generate and build.** Run `transcribe_rows.py`, then build the full Release solution. Fix unresolved aliases by adding the candidate symbol, never by editing generated instructions.
4. **Re-point.** For each finished caller that reached a row through a C++ stand-in, make the stand-in a forward to the naked row (or switch its alias) and delete the old body.
5. **Compare.** Write `evidence/phase<N>-<topic>-listing-compare.py` on `listing_compare_lib.run(rows, expected, tables, data, static_ranges=…)`. Run it against `build/Release/NxPhysics.dll` and `.map`. Expected: `ALL EQUAL` and `MAPPINGS checked=N unexpected=0`. Save the output to the raw bundle as `listing-compare.txt`.
6. **Evidence doc.** `phase<N>-<topic>.md` says:
   - for each row, its id, RVA and what it does;
   - the places that differ from a plain copy;
   - the rows with indirect call sites (from the backlog's `indirect_call_sites`);
   - that the rows await the testing pass.
7. **Record.** Run `close_static_rows.py --manifest manifest_<topic>.json` (format in Task 1).
8. **Checks and commit.** Run the per-commit checks from Global Constraints, then commit the cluster with an explicit `git add`. The subject is `Transcribe <what> (<ids>)`.

A row that resists the procedure after about an hour, despite compare chains, aliases, and `__emit` with a listed
reason, goes into `evidence/remaining-rows-handoff.md` under "Hard rows" with the blocker, and the cluster commits without it.

---

### Task 1: Tooling — `transcribe_rows.py` and `close_static_rows.py`

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/transcribe_rows.py`
- Create: `docs/reconstruction/novodex-physics/tools/close_static_rows.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_transcribe_rows.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_close_static_rows.py`

**Interfaces:**
- Produces: `transcribe_rows.py <cfg.py> <dest.cpp> (--header <head.txt> | --append)`.
  - The config keys are those of `gen11.py`: `ROWS, CALLS, SLOTS, DATA, STRINGS, PUSHES, TABLES, PREFIX`.
  - `--header` writes a new file from a header containing `@@PROTOS@@` and `@@BODY@@`.
  - `--append` adds the prototypes and bodies before a `// @@ROWS-END@@` marker in an existing file.
  - The output is CRLF.
  - The core is `transcribe(cfg, oracle_bytes) -> (protos: str, body: str)`.
- Produces: `close_static_rows.py --manifest <m.json> [--inventory …] [--closure-dir …] [--map build/Release/NxPhysics.map]`.
  - Manifest: `{"source": path, "compare": path, "doc": path, "instructions": int, "mappings": int, "rows": {id: {"symbol": name-prefix, "what": text}}}`.
  - Core: `close(manifest, inventory, closures, mapsyms) -> (inventory, closures)`.

- [ ] **Step 1: Write the failing tests**
  - `test_transcribe_rows.py` (unittest, with a tiny synthetic image):
    - `test_emits_one_naked_body_per_row`: two `ROWS` give two `__declspec(naked)` bodies whose `// 0x%08x` comments cover each range exactly.
    - `test_jump_table_becomes_compare_chain`: a `TABLES` entry gives `cmp reg, 0; je …` up to N-1 and a final `jmp`.
    - `test_append_inserts_before_marker`: `--append` leaves the text outside the marker byte-identical.
    - `test_output_is_crlf`.
  - `test_close_static_rows.py` (in-memory inventory and closure dicts):
    - `test_marks_reconstructed_with_static_proof_only`: state `reconstructed`; `static_proof` names the compare script, the instruction and mapping counts and the RVA; `dynamic_proof` is empty; the note contains `awaits the testing pass`.
    - `test_closure_counts_follow_the_move`: `deferred_not_reconstructed_in_phase` goes down by 1 and `deferred_reconstructed_not_falsified` goes up by 1 per row; the entry reason is `reconstructed_not_falsified`.
    - `test_split_rows_share_one_symbol`: two ids with the same `symbol` both get it, along with the same `candidate_map_reference`.
    - `test_refuses_vendored_rows`: a row with `third_party` set raises `ValueError`.
    - `test_refuses_rows_not_discovered`: a row already `reconstructed` raises `ValueError`.
    - `test_missing_map_symbol_raises`: `ValueError` naming the symbol.

- [ ] **Step 2: Run them to verify they fail**

  Run: `cd docs/reconstruction/novodex-physics/tools && python -m pytest tests/test_transcribe_rows.py tests/test_close_static_rows.py -q`
  Expected: FAIL (modules missing).

- [ ] **Step 3: Implement both tools**
  - `transcribe_rows.py` is `evidence/raw/phase4-pruner-cluster-20261020/gen11.py` plus `mkfile.py`, refactored into `transcribe()` plus `main()`; its behaviour is otherwise unchanged.
  - `close_static_rows.py` takes the field writes of the session's `close_rows12.py` (inventory: `state`, `source`, `implementation`, `implementation_symbol`, `candidate_map_reference` with `object` from the map line, `static_proof`, `notes`) and drops the mutation text.
  - Both preserve each JSON file's line endings, the same way `close_rows12.py`'s `rd()` and `wr()` do.

- [ ] **Step 4: Run them to verify they pass**

  Run: the same command. Expected: PASS. Then run the full `python -m pytest tests -q`; expected: at least 867 passed.

- [ ] **Step 5: Regenerate the pruner file as a check**

  Run: `python tools/transcribe_rows.py evidence/raw/phase4-pruner-cluster-20261020/cfg_ABC.py %TEMP%/po.cpp --header evidence/raw/phase4-pruner-cluster-20261020/headABC.txt`,
  then `fc /b %TEMP%\po.cpp D:\github\Novodex\Physics\src\opcode\PrunerOctree.cpp`.
  Expected: no differences, or only differences the commit message explains.

- [ ] **Step 6: Commit** the four files: `Add transcribe_rows and close_static_rows for the remaining-rows pass`.

### Task 2: Amend the spec for the vendored promotion policy

**Files:** Modify: `docs/superpowers/specs/2026-10-30-remaining-rows-design.md` (section 1, Success, End state).

- [ ] **Step 1:** In section 1, state that the promotion policy applies unchanged:
  - groups with x87 compares or arithmetic, or still REVIEW without a recorded hand review, cannot be promoted statically;
  - they stay `discovered` and are listed in the hand-off for the testing pass;
  - the bucket counts are filled in by Task 4's triage.
- [ ] **Step 2:** In Success and End state, change "zero `discovered`" to "zero `discovered` except the hard rows and the vendored rows the promotion policy holds for execution evidence, each listed with its reason".
- [ ] **Step 3: Commit**: `Spec: vendored rows keep the Task 5 promotion policy`.

### Task 3: Stage 1 — Phase 3 first-party (9 clusters, 49 rows)

**Interfaces:** Consumes Task 1's tools, and the cluster procedure.

- [ ] `0x23940-0x23a54` (001099–001105, 4 rows)
- [ ] `0x23da0-0x24399` (001131–001167, 13 rows)
- [ ] `0x245e0-0x24634` (001181) and `0x24990-0x24da4` (001211–001235, 10 rows): one commit if they share a subsystem
- [ ] `0x258f0-0x26699` (001303–001313, 6 rows, 3,467 bytes)
- [ ] `0x26ad0-0x27732` (001317–001345, 12 rows)
- [ ] `0x46ab0-0x470ed` (001876, 1,597 bytes)
- [ ] `0x4b9c0-0x4b9ee` (001935) and `0x4bc20-0x4bc79` (001947)
- [ ] **Stage exit:** gates 3 to 7 pass (`powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_phase_gate.ps1 -Phase N`, each log ending `phase_gate=N status=pass`). `report_completion.py` shows `discovered` down by 49 (to 221, less any hard rows).

### Task 4: Stage 2 — Vendored pass (86 rows: 71 qhull, 15 OPCODE)

**Files:** `External/qhull/novodex/*.c`, `External/opcode/novodex/*.cpp` (overlays); `evidence/phase4-third-party-map/{qhull,opcode}_{match,review}.csv`; `evidence/vendored-correspondence.md` (a new "Remaining-rows pass" section); `inventory.json`; `gates/phase4-closure.json`.

The matcher classes today, for the discovered rows:
- qhull: 32 REVIEW, 27 DIFF, 12 SHAPE;
- OPCODE: 6 DIFF, 5 SHAPE, 4 REVIEW.

- [ ] **Step 1: Triage.**
  - Run `python tools/vendored_match.py` (default arguments) against the current build. For each of the 86 rows record its class, its notes (x87, inlining, imms, integer branches) and its first `diff_*` token.
  - Then bucket each row:
    - promotable statically: SHAPE or MATCH with no x87, no inlining note and no imms note;
    - needs a hand review: REVIEW, or SHAPE/MATCH with an inlining, imms or integer-branch note;
    - needs a source fix: DIFF or MISSING;
    - needs execution evidence: any x87.
  - Write the table into the new section of `vendored-correspondence.md`.
- [ ] **Step 2: Source fixes.**
  - For each DIFF or MISSING group, edit the overlay file named by `source_function` until the matcher stops reporting it as DIFF (MISSING: make the function compile and link in).
  - A group that only a transcription can satisfy follows the cluster procedure into `Physics/src/vendored/<Lib>Rows.cpp` instead.
  - Re-run the matcher after each file; commit per overlay file: `Fix the <lib> overlay of <function> to match the oracle (<ids>)`.
- [ ] **Step 3: Hand reviews.** Record each review in `{lib}_review.csv` with the addresses read and a verdict, in the existing format.
- [ ] **Step 4: Promote** the rows the policy now allows.
  - Use the field format of the Task 5b promotions: the static proof text names the matcher class, the compared features and the uncompared ones.
  - The closure entries stay `vendored_not_falsified`.
  - Rows held for execution evidence stay `discovered`; collect their ids for Task 9.
  - Commit per library: `Promote the remaining <lib> rows the policy allows (<n> rows)`.
- [ ] **Stage exit:** gates 3 to 7 pass, and the inventory validator prints `unexplained=0`.

### Task 5: Stage 3 — Phase 4 first-party, first half (11 clusters, 53 rows)

- [ ] `0x2d940-0x2d9a8` (001524–001528)
- [ ] `0x4cba0-0x4cc55` (001988) and `0x4cee0-0x4db9e` (001992–002006, 8 rows, 3,226 bytes)
- [ ] `0x4df30-0x4e112` (002010–002014) and `0x4ff20-0x4ff71` (002031)
- [ ] `0x52060-0x521ab` (002077): its neighbours are in `InternalTriangleMesh.cpp`
- [ ] `0x52a60-0x5328c` (002095–002138, 24 rows)
- [ ] `0x53f40-0x54793` (002178–002205, 7 rows)
- [ ] `0x54a80-0x54ba2` (002239), `0x54e70-0x552b5` (002243–002247) and `0x555b0-0x5580a` (002255–002256): the mesh-mass neighbourhood, so check `ObjectModel.cpp`
- [ ] Regenerate the backlog: `discovered` is down by 53. No gates at this exit (spec, stage 3).

### Task 6: Stage 4 — Phase 4 first-party, second half (17 clusters, 46 rows)

- [ ] `0xb3d20-0xb3fff` (004790–004801, mixed 4/6) and `0xb4790-0xb4d6b` (004820–004834, mixed 4/7): SdkContainer and allocator neighbourhood
- [ ] `0xb5090-0xb5123` (004852) and `0xb5460-0xb5497` (004870–004872)
- [ ] `0xe5590-0xe56d0` (005231), `0xe6230-0xe647e` (005262–005263, the bounded pruner's vtable slots 7 and 0) and `0xe6b50-0xe6c0a` (005279): into `PrunerOctree.cpp`, appended with `--append`
- [ ] `0xe9420-0xe9545` (005380–005386) and `0xe9b20-0xe9b7e` (005404)
- [ ] `0xef670-0xefa88` (005448–005466, 8 rows) and `0xefc90-0xeffb4` (005470–005479): the pruning pool
- [ ] `0xf15c0-0xf15f3` (005539), `0xf2090-0xf2199` (005566–005568), `0xf2580-0xf2684` (005586–005590), `0xf2ec0-0xf2fd4` (005606–005610), `0xf3630-0xf36a5` (005624) and `0xf3d40-0xf3ee4` (005642–005646): grouped by subsystem after the Place step
- [ ] **Stage exit:** gates 3 to 7 pass.

### Task 7: Stage 5 — Phase 6 (5 clusters, 10 rows)

- [ ] `0x9ab00-0x9b0d0` (004159–004171)
- [ ] `0xafcc0-0xaffda` (004401) and `0xb02e0-0xb0548` (004405–004413)
- [ ] `0xb15e0-0xb15ff` (004537) and `0xb38a0-0xb3917` (004759–004761)
- [ ] **Stage exit:** gates 3 to 7 pass.

### Task 8: Stage 6 — Phase 7 (4 clusters, 26 rows)

- [ ] `0x56690-0x5a16a` (002268–002316, 22 rows, 14,739 bytes)
  - Split it into commits of at most about 3 KB of listing each, in callee-first order.
  - Run gate 7 after each commit, because this block is the riskiest.
- [ ] `0x5a520-0x5a8d3` (002332–002336)
- [ ] `0x5ae50-0x5b60e` (002350, 1,982 bytes)
- [ ] `0x5b9e0-0x5ba3a` (002400)
- [ ] **Stage exit:** gates 3 to 7 pass.

### Task 9: Hand-off to the testing pass

**Files:** Create `docs/reconstruction/novodex-physics/evidence/remaining-rows-handoff.md`.

- [ ] **Step 1:** Generate the row lists from `inventory.json` and `completion-backlog.json`:
  - every row this plan moved to `reconstructed`, grouped by the test family that drives its nearest gated caller (that caller's `dynamic_proof` names the family), with a "no driven caller" group;
  - the rows with indirect call sites, marked;
  - the hard rows, each with its blocker;
  - the vendored rows held for execution evidence, each with its class and notes.
- [ ] **Step 2:** Run `report_completion.py`. Expected: `discovered` equals the hard rows plus the held vendored rows, and no more.
- [ ] **Step 3:** Run the final checks: gates 3 to 7, the tooling tests and the inventory validator, all passing.
- [ ] **Step 4: Commit**: `Hand off the remaining-rows pass to testing`.
