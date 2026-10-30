# Vendored Rows Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the 86 vendored rows still `discovered` (71 qhull, 15 OPCODE) to `reconstructed`. Each gets a
listing-equal transcription (the static proof) and an oracle-vs-candidate execution of its group that matches in
every word (the dynamic proof, Task 5b arm (i)).

**Architecture:**
- **Transcription.** Each function's body is guarded off in its `External/<lib>/novodex/` copy and re-defined under
  its own symbol as a naked listing transcription. qhull's go in `QhullRows.cpp`, OPCODE's in
  `OpcodeVendoredRows.cpp`. Both are made by `transcribe_rows.py` and proven by a listing-compare script.
- **Execution.** It runs in `tests/PhysicsThirdPartyTests.cpp` and is measured by the `vendored_trace.py` cdb trace.
  Groups that nothing executes get new drives.
- **Recording.** `close_static_rows.py --vendored` records a row only when its group's family is exact.

**Tech Stack:**
- MSVC Win32 Release (CMake, `build/`), naked `__asm`;
- Python 3 with capstone (tooling under `docs/reconstruction/novodex-physics/tools`, unittest via pytest);
- cdb x86;
- PowerShell phase gates.

**Spec:** `docs/superpowers/specs/2026-10-30-vendored-rows-design.md`

Paths below are relative to `docs/reconstruction/novodex-physics/` unless they start with `External/`, `tests/`, `build/` or
`docs/superpowers/` (repo root).

## Global Constraints

- **Branch:** `vendored-rows`, cut from `main` once this plan is committed. Never push.
- **Oracle:** `D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll`, SHA-256
  `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- **`upstream/` is never edited.** Every edit is in `External/<lib>/novodex/`. Every `novodex/` file carries a
  `NOVODEX LOCAL MODIFICATION` block that names an oracle address. `python tools/verify_vendored_sources.py --repo ../../..`
  passes.
- **The guard macro.** It is `NX_VENDORED_LISTING_ROWS`, defined on `NxQhull` and `NxOpcode` only.
- **Symbols.** Transcriptions take the original symbols: qhull as `extern "C" __declspec(naked)`, OPCODE as naked member
  definitions. No `/alternatename` for these rows.
- **Promotion is Task 5b arm (i), unchanged.** A row needs all four:
  - its group is MATCH after the re-match;
  - `family_best=exact` in the re-traced `evidence/phase4-third-party-map/vendored_coverage.csv`;
  - its proof lists and attributes every divergent family the group also ran in;
  - no open triage item.
- **The closure entry** moves from `vendored_not_falsified` to `reconstructed_not_falsified`, and
  `deferred_vendored_not_falsified` and `deferred_reconstructed_not_falsified` follow it.
- **Every commit:**
  - the full Release build is clean (`cmake --build build --config Release`);
  - `python tools/validate_inventory.py inventory.json` prints `unexplained=0`;
  - `work_units.json` and `completion-backlog.json` are regenerated (`tools/work_units.py`, `tools/report_completion.py`);
  - `python -m pytest tests -q` from `tools/` passes with at least 877 tests.
- **At each task's exit,** gates 3 and 4 pass:
  `powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_phase_gate.ps1 -Phase N`, and each log ends
  `phase_gate=N status=pass`. Task 7 runs gates 3 to 7.
- **Files and commits.**
  - Generated `.cpp` files are CRLF.
  - `git add` explicitly by path: the tree carries unrelated untracked files (`candidate/`, `oracle/`, `*.psc`, `canary*.txt`).
  - Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Undrivable groups.** A group that cannot be driven safely stays `discovered`. Its reason goes in the row notes,
  the trace line and the hand-off. It is not forced.

## Review Focus

1. **A guarded body can no longer be inlined.** Once a function is only a declaration, its upstream callers change
   codegen, and some of those callers are already-promoted rows. Expected: no promoted group's matcher class gets
   worse. Pinned by the "class drift" step of the library procedure (step 6), which stops the task on any regression.
2. **A row promoted on a family that never executed it.** `family_best=exact` must come from a family the group was
   actually hit in. Test: `test_vendored_refuses_family_not_in_group_differential` (Task 1).
3. **Split rows.** A function that spans several inventory ids must close every id against the one group's coverage
   line, and must not close an id whose group differs. Test: `test_vendored_split_rows_need_their_group` (Task 1).
4. **A header-inline member.** An OPCODE method defined inline in its header can't also be given a naked out-of-class
   definition. Expected: the probe in Task 2 step 1 finds this before any bulk transcription. Rule on each case as
   in Task 2.
5. **A family turns exact but stays registered as divergent.** Its ceiling would then hide a later regression.
   Expected: every family whose `discrete`/`beyond` drops to 0 is re-registered `verdict=exact`, and its
   `kDivergentCeilings` entry is removed. Pinned by the "ceilings" step of the library procedure (step 10).

## File Structure

- **Modify `tools/transcribe_rows.py`:** member rows (`::` in the declaration) emit no prototype.
- **Modify `tools/close_static_rows.py`:** add the `--vendored` mode.
- **Modify `tools/tests/test_transcribe_rows.py` and `tools/tests/test_close_static_rows.py`.**
- **Create `External/qhull/novodex/QhullRows.cpp` and `External/opcode/novodex/OpcodeVendoredRows.cpp`:** the
  transcriptions. Each begins with a `NOVODEX LOCAL MODIFICATION` block. Both are added to `DECLARED_ADDITIONS` in
  `tools/verify_vendored_sources.py`, to the additions list in `External/<lib>/MODIFICATIONS.md`, and to the
  `NxQhull`/`NxOpcode` source lists in `External/CMakeLists.txt`.
- **Create or modify `External/<lib>/novodex/<file>`:** the guards. A new copy is upstream byte for byte, plus the
  modification block and the guards.
- **Create per library:**
  - `evidence/raw/vendored-rows-<lib>-20261030/`, holding `cfg.py`, `head.txt`, `manifest.json` and `listing-compare.txt`;
  - `evidence/vendored-rows-<lib>-listing-compare.py`.
- **Modify `tests/PhysicsThirdPartyTests.cpp`:**
  - a new `nxDriveVendoredRows` section (Task 6);
  - the isolation calls (Tasks 4 and 5);
  - `kDivergentCeilings`.
- **Modify `tools/gate_targets.ps1`:** family registrations and `$NxPhaseCoverageFloor`.
- **Regenerate:**
  - `evidence/phase4-third-party-map/{qhull,opcode}_match.csv` and `vendored_coverage.csv`;
  - `evidence/vendored-trace-{qhull,opcode}.txt`.
- **Modify the docs (Task 7):** `evidence/vendored-correspondence.md` and `evidence/remaining-rows-handoff.md`.
- **Modify the registry:** `inventory.json` and `gates/phase4-closure.json`.

## The library procedure

Tasks 2 to 5 each run this procedure for the groups they list. It is written out once here.

1. **Guard.** For each function, in its file's `novodex/` copy (create the copy from `upstream/` if there is none),
   wrap the body in `#ifndef NX_VENDORED_LISTING_ROWS` … `#endif`, followed by
   `/* NX_VENDORED_LISTING_ROWS: transcribed in <RowsFile> (phys_fn_NNNNNN) */`. Add or extend the file's
   modification block with the function's oracle RVA.
2. **Config.** Extend `evidence/raw/vendored-rows-<lib>-20261030/cfg.py`:
   - `ROWS` takes each function's whole range from the inventory, callees first, one entry per function. `decl` is
     the original signature with `__declspec(naked)`: `extern "C"` for qhull, `Class::method` for OPCODE.
   - `CALLS` names callees by their candidate symbol from `{lib}_match.csv`.
   - `DATA` names `qh_qh`, `qhmem`, `qhstat` (qhull) and OPCODE's statics.
   - `TABLES` lists the jump tables.
   - `PREFIX` is `nxQhK` or `nxOpK`.
3. **Generate and build.** Run `python tools/transcribe_rows.py <cfg> <RowsFile> --append` (or `--header head.txt`
   the first time), then the Release build. Fix an unresolved callee by naming it in `CALLS`, never by editing the
   instructions.
4. **Compare.**
   - Run `evidence/vendored-rows-<lib>-listing-compare.py`, written on
     `listing_compare_lib.run(rows, expected, tables, data, static_ranges=…)` over `build/Release/NxPhysics.dll` and `.map`.
   - Expected output: `ALL EQUAL` and `MAPPINGS checked=N unexpected=0`.
   - Save the output as `listing-compare.txt` in the bundle.
5. **Re-match.** Run `python tools/vendored_match.py`. Expected: every transcribed group is MATCH.
6. **Class drift.** Diff the new `{lib}_match.csv` against `HEAD`'s. If any group that is not transcribed and has
   promoted rows gets worse (MATCH→SHAPE→REVIEW→DIFF), stop. Either transcribe that caller too (if it is in this
   plan's 86) or ledger a ruling and leave the inlined callee unguarded.
7. **Run the harness.** Run `build\Release\NxPhysicsThirdPartyTests.exe > <workspace>\tp.txt`. Every family that
   used to be exact stays exact, and no ceiling is exceeded.
8. **Trace.** Re-run the trace pipeline with the same two exes and options as the `Traced:` and `Command:` lines of
   `evidence/vendored-trace-<lib>.txt`, writing `vendored_coverage.csv` and `vendored-trace-<lib>.txt`:
   - `vendored_trace.py identity`;
   - `vendored_trace.py script`;
   - `cdb -cf <script> build\Release\NxPhysicsThirdPartyTests.exe`;
   - `vendored_trace.py parse`;
   - `vendored_trace.py coverage`;
   - `vendored_trace.py report`.
9. **Isolation fallback.** If a transcribed group is hit but its `family_best` is not `exact`, find the upstream
   callee responsible (the first differing tape word). Add a direct call by RVA to the row, on identical inputs, as
   its own `vendored_rows_<group>` family in the Task 6 section style, then repeat steps 7 and 8.
10. **Ceilings.** For every family that is now exact, change its `tools/gate_targets.ps1` line to the
    `mismatches=0 worst_ulp=0 verdict=exact` form and delete its `kDivergentCeilings` entry. Lower any remaining
    divergent entry to its new figures. Register new families, and raise `$NxPhaseCoverageFloor` for phase 4 by the
    number of lines added.
11. **Record.**
    - Write `manifest.json` in the `close_static_rows.py` format, adding `"group"` and `"family"` per row (Task 1).
    - Run `python tools/close_static_rows.py --vendored --manifest <manifest>`.
    - Re-pin `oracle-only-baselines.json` and its proofs if a fixture hash changed.
12. **Checks and commit.** Run the per-commit checks from Global Constraints, then gates 3 and 4. The commit subject
    is `Transcribe and execute <what> (<n> <lib> rows)`.

---

### Task 1: Tooling

**Files:**
- Modify: `tools/transcribe_rows.py` (the `protos +=` line in `transcribe()`)
- Modify: `tools/close_static_rows.py` (`close()`, `main()`)
- Test: `tools/tests/test_transcribe_rows.py`, `tools/tests/test_close_static_rows.py`

**Interfaces:**
- Produces: `close(manifest, inventory, closures, mapsyms, coverage=None)`.
  - With `coverage`, it runs vendored mode. `coverage` is `{(library, group_rva): {'family_best': str, 'differential': str}}`,
    read from `vendored_coverage.csv`.
  - Each manifest row also carries `"group": "0x…"` (the group's function RVA) and `"family": "<name>"` (an exact family).
- Produces: `close_static_rows.py --vendored [--coverage evidence/phase4-third-party-map/vendored_coverage.csv]`.
- What vendored mode writes:
  - `static_proof`, as now, plus `Vendored <lib> function, upstream body guarded by NX_VENDORED_LISTING_ROWS`;
  - `dynamic_proof`: `cdb trace, evidence/vendored-trace-<lib>.txt: family <family> exact (every word compared with the oracle); vendored_coverage.csv group <group> family_best=exact`;
  - `notes`: `Vendored row transcribed and executed (vendored-rows pass).`, followed by the old note when it said why
    the row was held;
  - the closure entry's reason `vendored_not_falsified` → `reconstructed_not_falsified`, with both counts updated.

- [ ] **Step 1: Write the failing tests**
  - `test_transcribe_rows.py`:
    - `test_member_rows_emit_no_prototype`: rows with decls `__declspec(naked) float Triangle::Area() const` and
      `extern "C" __declspec(naked) void qh_x()` give `protos == 'extern "C" void qh_x();\n'`.
  - `test_close_static_rows.py`, with an in-memory inventory row `third_party: "qhull"`, a closure entry reason
    `vendored_not_falsified`, and a coverage dict:
    - `test_vendored_records_static_and_dynamic_proof`: the state is `reconstructed`; `dynamic_proof` contains the
      family and `family_best=exact`; `deferred_vendored_not_falsified` goes down by 1 and
      `deferred_reconstructed_not_falsified` up by 1.
    - `test_vendored_refuses_inexact_group`: `family_best: 'discrete'` raises `ValueError`.
    - `test_vendored_refuses_family_not_in_group_differential`: `family_best: 'exact'` but the manifest's family
      missing from `differential` (or present as `=discrete`) raises `ValueError`.
    - `test_vendored_split_rows_need_their_group`: two ids with one `group` both close. A third id whose `group` has
      no coverage line raises `ValueError`.
    - `test_vendored_refuses_first_party_row`: a row with no `third_party` in vendored mode raises `ValueError`.
    - The existing `test_refuses_vendored_rows` stays as it is (no `coverage`, so it still refuses).
- [ ] **Step 2: Run them to verify they fail**
  - Run: `cd tools && python -m pytest tests/test_transcribe_rows.py tests/test_close_static_rows.py -q`
  - Expected: the six new tests FAIL (prototype emitted; `close()` takes no `coverage`).
- [ ] **Step 3: Implement**
  - In `transcribe()`, skip the prototype when the decl text before `(` contains `::`.
  - In `close()`, branch on `coverage`:
    - vendored mode requires `third_party`, the `vendored_not_falsified` reason, and a coverage line with
      `family_best == 'exact'` whose `differential` contains `<family>=exact`;
    - first-party mode is unchanged.
  - `main()` reads the CSV with `csv.DictReader`, keyed by `(library, rva)`.
- [ ] **Step 4: Run them to verify they pass**
  - Run the same command. Expected: PASS.
  - Then run `python -m pytest tests -q`. Expected: at least 883 passed.
- [ ] **Step 5: Commit** the four files: `Add a vendored mode to close_static_rows; no prototypes for member rows`.

### Task 2: OPCODE (15 rows)

**Interfaces:**
- Consumes: Task 1's `--vendored` mode and member-row decls.
- Produces:
  - `External/opcode/novodex/OpcodeVendoredRows.cpp`, with its markers;
  - the `NX_VENDORED_LISTING_ROWS` define on `NxOpcode`;
  - the bundle `evidence/raw/vendored-rows-opcode-20261030/`.

Groups:
- 004844 `Container::Container(const Container&)`;
- 005069 `SphereCollider::SphereTriOverlap`;
- 005111 `OBBCollider::ValidateSettings`, which is ICF-folded with `PlanesCollider::ValidateSettings`, so both
  symbols must resolve to the one body: the second is a naked `jmp` to the first, and the comment says why;
- 005204 `OBB::ComputeWorldEdgeNormal`;
- 005366 `Model::~Model`;
- 005368 `Model::Build`;
- 005281 `SAP_PairData::Init`;
- 005155 `Plane::Set`;
- 005179, 005181, 005183, 005185 `Triangle::Area/Normal/Center/Inflate`;
- 005193, 005195, 005197 `Matrix4x4::CoFactor/Determinant/Invert`.

- [ ] **Step 1: Probe one member row.** Run procedure steps 1 to 4 for `Triangle::Area` (005179) alone, after:
  - adding `OpcodeVendoredRows.cpp` to the `NxOpcode` sources;
  - adding `target_compile_definitions(NxOpcode PRIVATE NX_VENDORED_LISTING_ROWS)`;
  - adding the file to `DECLARED_ADDITIONS["opcode"]` and to `MODIFICATIONS.md`.

  Expected: the build is clean, the compare prints `ALL EQUAL`, and `verify_vendored_sources.py` passes. If the
  method is defined inline in its header, ledger `Ruling:` that the header's inline body is guarded in a
  `novodex/` copy of the header (the same macro), and continue.
- [ ] **Step 2:** Run procedure steps 1 to 6 for the remaining 14.
- [ ] **Step 3:** Run procedure steps 7 to 10. Expected: `ice_plane_triangle` and `ice_matrix4x4` are exact, and
  their ceilings are removed.
  - The groups already hit (13) should read `family_best=exact`.
  - The two groups that nothing hits are left for Task 6.
- [ ] **Step 4:** Run procedure steps 11 and 12 for the rows whose groups are exact. Expected: `report_completion.py`
  shows `discovered` down by that number.

### Task 3: qhull float-divergence sources and x87 rows (16 rows)

**Interfaces:**
- Consumes: Task 2's pattern.
- Produces:
  - `External/qhull/novodex/QhullRows.cpp`;
  - the define on `NxQhull`;
  - the `DECLARED_ADDITIONS["qhull"]` entry;
  - the bundle `evidence/raw/vendored-rows-qhull-20261030/`.

Groups, callee-first: 002425 `qh_distplane`, 002487 `qh_gram_schmidt`, 002444 `qh_getdistance`, 002438 `qh_getangle`,
002485 `qh_findgooddist`, 002518 `qh_randommatrix`, 002550 `qh_joggleinput`, 002558 `qh_rotateinput`, 002571/002573
`qh_initqhull_buffers`, 002970 `qh_tracemerging`, 003101 `qh_furthestout`, 003107/003109 `qh_matchduplicates`, 003184
`qh_initialvertices`, 003204 `qh_nextfurthest`.

- [ ] **Step 1:** Run procedure steps 1 to 6.
- [ ] **Step 2:** Run procedure steps 7 to 10. Expected: `qhull_hull_x87` and the other `qhull_*_x87` families
  have `discrete=0` at least. Any that are now word-exact are re-registered as exact. Groups with no hits wait for
  Task 6.
- [ ] **Step 3:** Run procedure steps 11 and 12 for the exact groups.

### Task 4: The remaining qhull rows existing drives reach

Groups (the hit ones among):
- 002697 `qh_detvridge`;
- 002884 `qh_appendmergeset`;
- 002900/002902 `qh_degen_redundant_facet`;
- 002921–002925 `qh_maydropneighbor`;
- 002927–002933 `qh_mergecycle_neighbors/ridges/vneighbors`;
- 002937 `qh_mergeneighbors`;
- 002939 `qh_mergeridges`;
- 002943/002945 `qh_mergevertex_neighbors`;
- 002947/002949 `qh_mergevertices`;
- 002951/002953 `qh_neighbor_intersections`;
- 002972 `qh_updatetested`;
- 002987/002989 `qh_vertexridges`;
- 002991/002993 `qh_find_newvertex`;
- 002995 `qh_mergecycle`;
- 003051 `qh_matchvertices`;
- 003093 `qh_checkvertex`;
- 003146 `qh_triangulate_mirror`;
- 003164/003166 `qh_createsimplex`.

Before step 1, read `hits` in `vendored_coverage.csv` for each group, and move any with no hits to Task 6.

- [ ] **Step 1:** Run procedure steps 1 to 6, then 7 to 10.
- [ ] **Step 2:** Run procedure steps 11 and 12 for the exact groups.

### Task 5: Transcribe the qhull rows nothing executes yet

Groups:
- 002522 `qh_scalelast`;
- 002528 `qh_setdelaunay`;
- 002532–002538 `qh_voronoi_center`;
- 002566–002570 `qh_init_qhull_command`;
- 002575 `qh_initqhull_mem`;
- 002577 `qh_initqhull_start`;
- 002670/002672 `qh_initqhull_globals`;
- 002699/002701 `qh_detvridge3`;
- 002703/002705 `qh_eachvoronoi`;
- 002743 `qh_printvdiagram2`;
- 002813/002815 `qh_printvoronoi`;
- 003133/003135 `qh_printhashtable`;
- 003304 `qh_setprint`;
- 003412 `qh_printallstatistics`;
- 003415 `qh_errprint`;
- plus any moved here from Task 4.

- [ ] **Step 1:** Run procedure steps 1 to 6.
- [ ] **Step 2:** Run step 7 (the harness: nothing that was exact regresses). Commit the transcriptions:
  `Transcribe the qhull rows awaiting drives (<n> rows)`.
  - Nothing is recorded yet.
  - The rows stay `discovered` until Task 6.

### Task 6: New drives for the unexecuted groups

**Files:** Modify `tests/PhysicsThirdPartyTests.cpp`, adding a section after `nxDriveQhullGap`, its call in `main`,
and `tools/gate_targets.ps1`.

**Interfaces:**
- Produces: `static void nxDriveVendoredRows(const NxOracleRows& o, bool selfOnly)`, which reports families
  `vendored_rows_<group>` through `nxReport`.
- Consumes the harness's existing pieces: the oracle-row table, tapes, `nxReport`, and the qhull drive's setup for
  each side (`nxDriveQhullHull`).

The drives, each one a family:
- **`vendored_rows_qhull_output_<opt>`.** A whole qhull run on each side with the option strings `"v Fv"`, `"v o"`,
  `"d Fv"`, `"FN Fi"`, `"Qbb d"`, and `"Ts Tv"` for statistics and the hash table, over the `qhull_hull` inputs.
  Each side writes to its own `tmpfile()`, and the tape is the file's bytes as words. These reach the output,
  Voronoi, Delaunay, scaling, statistics and set-print groups.
- **`vendored_rows_qhull_init`.** `qh_init_qhull_command`, `qh_initqhull_start`, `qh_initqhull_mem`,
  `qh_initqhull_globals` and `qh_initqhull_buffers` through the NovodeX call sequence with an option string covering
  their branches. The tape is the `qh` block's scalar fields, with pointers skipped as the existing qhull families
  skip them.
- **`vendored_rows_qhull_merge`.** Coplanar-cluster inputs (points on 3 parallel planes, 1e-13 jitter, fixed
  seed) with `"C-0 Qc"`, which force the merge-cycle, vertex-rename and new-vertex groups. The tape is as
  `qhull_hull`'s.
- **`vendored_rows_qhull_errprint`.** `qh_errprint("ERR", facet, NULL, ridge, vertex)` on a facet from a finished
  hull on each side, writing to `qh ferr`, which is redirected to `tmpfile()`. The tape is the bytes. `qh_errexit` is
  never called.
- **`vendored_rows_opcode_<group>`.** The two OPCODE groups Task 2 left unhit, each called directly by RVA on
  objects built by the oracle's own constructors, as `nxDriveIceMaths` does.

- [ ] **Step 1: Write the drives.** Run the harness:
  `build\Release\NxPhysicsThirdPartyTests.exe > <workspace>\tp.txt`. Expected: every new family prints a line, and
  each is `mismatches=0`. A family that mismatches is a finding: debug the transcription or the drive, never the
  tape.
- [ ] **Step 2: Undrivable groups.** For a group that no safe drive reaches, ledger a `Ruling:` with the reason and
  add it to Task 7's held list.
- [ ] **Step 3:** Run procedure steps 8, 10, 11 and 12 for every newly exact group, both qhull and OPCODE.
  - Register each new family as exact in `gate_targets.ps1`.
  - Raise the phase 4 floor by the number of lines added.
  - Commit: `Drive and record the vendored rows nothing executed (<n> rows)`.

### Task 7: Docs, registry and hand-off

**Files:**
- `evidence/vendored-correspondence.md`: a new "Vendored-rows pass (2026-10-30)" section;
- `evidence/remaining-rows-handoff.md`;
- the inventory notes of the 8 ICE rows (written by Task 1's notes rule; check them).

- [ ] **Step 1:** Write the section:
  - a table of every group with its rows, its library, its proving family and the exe SHA-256 from the trace;
  - the families re-registered as exact, and the ceilings removed;
  - the held groups with their reasons (or "none");
  - every class-drift ruling.
- [ ] **Step 2:** In `remaining-rows-handoff.md`, replace the vendored hold section with a pointer to the new section,
  and list any held groups.
- [ ] **Step 3:** Run `python tools/report_completion.py`. Expected: the `discovered` count equals the held groups'
  rows, which is zero if none are held.
- [ ] **Step 4:** Run gates 3 to 7, the tooling tests, the inventory validator and `verify_vendored_sources.py`. All
  must pass.
- [ ] **Step 5: Commit:** `Record the vendored-rows pass`.
