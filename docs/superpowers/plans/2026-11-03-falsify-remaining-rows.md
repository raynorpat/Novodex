# Falsifying the Remaining Reconstructed Rows Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give every code row now at `reconstructed`, `discovered` or `statically_reviewed` (1,365 rows) a recorded mutation inside its own compiled range that a gate caught, or a validator-accepted deferral.

**Architecture:** Two new tools under `docs/reconstruction/novodex-physics/tools/`: `falsify_queue.py` (which rows, which gate, which source function) and `mutate_cpp.py` (source-level mutation of one C++ function, a byte-range check on the rebuilt DLL, detection through the existing `mutate_rows.py` primitives, and batching across rows with disjoint gate families). Rows are then worked group by group with one shared batch procedure, closed with `close_static_rows.py --falsified`.

**Tech Stack:** Python 3 (tooling and its unittest/pytest suite in `tools/tests`), MSVC/CMake build of `NxPhysics.dll` and the gated test targets, PowerShell gate scripts, the existing `mutate_rows.py` / `close_static_rows.py` / `report_completion.py`.

**Spec:** `docs/superpowers/specs/2026-11-03-falsify-remaining-rows-design.md`

## Global Constraints

* Never push. Fast-forward merge to `main` locally. Explicit-path `git add` only. Leave unrelated untracked files and `stash@{0}` untouched.
* Commit trailer: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
* A mutation counts only if (1) the unmutated control run is exact, (2) every changed byte of the rebuilt DLL lies inside the row's compiled range, and (3) a gate family reports the difference. Close a row with at most 2 survivors out of 16 mutants tried; otherwise defer it.
* Time-box every family; a spike that does not reach a row ends in a recorded deferral (reason from `DEFERRED_REASONS` in `validate_inventory.py`: `blocked_on_later_phase`, `unreachable_in_phase_2`, `not_independently_falsifiable`, `homeless_shared_code`, `not_reconstructed_in_phase`, `reconstructed_not_falsified`, `data_object_not_dispositioned`).
* Full gate cycle after each closed batch: `run_phase_gate.ps1 -Phase 3` through `7` all exit 0; update `fixture_source_sha256` in `evidence/oracle-only-baselines.json` when `tests/PhysicsCollisionTests.cpp` changes; copy only the ten named `build/oracle-proofs/NxPhysics*.json` into `evidence/oracle-only-proofs/` plus the four duplicates (phase5-shape-vtable-boxsweep, phase6-joint-desc, phase6-joint-matrix, phase7-joint-support); regenerate `work_units.json` and `completion-backlog.json`; tooling tests pass.
* Python edits to C source that contain a backslash use `chr(92)`; never `\\n` in a heredoc.
* Fixtures and logs must contain no text from the capturing process (see `evidence/phase7-closure-audit.md`, Review follow-up).
* Oracle: `D:/FlamingEnt__/Unreal_3/Binaries`, sha256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` (`NX_ORACLE_DIR` overrides the path).

## Review Focus

1. A mutation that changed an inlined callee or nothing at all being counted for the row -> Task 3's range check, with a test where the changed bytes lie outside the range.
2. A batch attributing one row's detection to another -> Task 5, with a test where two rows share a family and must not batch.
3. A mutated source left on disk after a crash or a failed restore -> Task 4's restore test; every procedure ends with `git diff --quiet` on the mutated file.
4. A flaky digest producing a false "caught" -> the control run before every batch (Task 5) and a repeat of every recorded catch once on its own (Procedure B step 6).
5. Equivalent mutants hiding a weak gate -> the survivor list is kept per row in the evidence directory and a row with more than 2 survivors of 16 is deferred, not closed (Procedure B step 7).

---

## Procedure B (batch procedure, used by Tasks 7-13)

For a group of rows from `falsify_queue.py`:

1. `python tools/falsify_queue.py --group <name> --out .superpowers/sdd/<plan>/queue-<name>.json` lists id, rva, size, source, symbol, gate target, family and the batch plan.
2. For each batch: `python tools/mutate_cpp.py --queue <file> --batch <k> --survey --limit 16 --log <log>` mutates one function per row (disjoint families together), builds once, runs once, attributes detections by family, bisects a failing family. Survivors are listed per row.
3. A row with a catch and at most 2 survivors out of 16 gets a recorded mutation: `python tools/mutate_cpp.py --queue <file> --row <id> --log <log>` (single row, non-survey).
4. `python tools/mutate_cpp.py --manifest <log> --out <evidence dir>/manifest.json` writes the `close_static_rows.py --falsified` manifest (gate `NxPhysicsPhase6RowsTests` or the row's own target, family, mutation, detected, `wired` true).
5. `python tools/close_static_rows.py --falsified --manifest <manifest>`.
6. Repeat each recorded catch once alone; run the full gate cycle; regenerate; tooling tests.
7. Rows over the survivor limit, with no standalone range, or whose family never reaches them are deferred with the reason and the survivor list in `evidence/falsify-<group>.md`.
8. Commit explicit paths; fast-forward merge to `main`; ledger line.

---

### Task 1: Row queue

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/falsify_queue.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_falsify_queue.py`

**Interfaces:**
- Produces: `build_queue(inventory: dict) -> list[dict]` (one dict per row whose `kind` is `code` and `state` is `reconstructed`, `discovered` or `statically_reviewed`, with keys `id`, `rva` (int), `size`, `phase`, `state`, `source`, `symbol` (the inventory `implementation_symbol`, or `None`), `third_party` (or `None`), `target` (the `Nx...Tests` name parsed from `dynamic_proof`, or `None`), `group`); `group_of(row: dict) -> str` returning one of `joints`, `collision`, `mesh`, `pruner`, `scene`, `vendored_opcode`, `vendored_qhull`, `no_driver`, `no_mapping`, `unclassified`, `other_first_party`; `batches(rows: list[dict], size: int) -> list[list[dict]]` putting rows with the same `target` in different batches.
- CLI: `falsify_queue.py [--inventory PATH] [--group NAME] [--out PATH] [--summary]`.

- [ ] **Step 1: Write the failing tests** in `test_falsify_queue.py`: `test_queue_keeps_only_unfinished_code_rows` (rows of kind `code` with state `dynamically_gated`, `closed`, `classified` and kind `data` are absent; the three unfinished states are present); `test_group_of_joints` (`source` `Physics/src/D6Joint.cpp` -> `joints`; `Physics/src/NpFixedJoint.cpp` -> `joints`); `test_group_of_vendored` (`third_party` `qhull` -> `vendored_qhull`, `opcode` -> `vendored_opcode`); `test_group_of_no_driver_and_no_mapping` (an `implementation`-less row with no `dynamic_proof` -> `no_mapping`; with `implementation` but no `dynamic_proof` -> `no_driver`); `test_batches_never_share_a_target` (six rows over two targets, batch size 4 -> no batch holds two rows with the same target).
- [ ] **Step 2: Run** `python -m pytest docs/reconstruction/novodex-physics/tools/tests/test_falsify_queue.py -q`. Expected: FAIL (module missing).
- [ ] **Step 3: Implement** the functions above in `falsify_queue.py`; the target is the first `Nx[A-Za-z0-9]+Tests` token in `dynamic_proof`; group order of precedence: vendored, no_mapping/no_driver, then by source file name (`*Joint*` -> joints; `Contact*`, `Distance*`, `Gjk*` -> collision; `*Mesh*`, `Quantizer*`, `PMap*` -> mesh; `Pruner*`, `IcePruner*`, `Ice*` -> pruner; `Scene*` -> scene), else `other_first_party`.
- [ ] **Step 4: Run** the tests. Expected: PASS.
- [ ] **Step 5: Run** `python docs/reconstruction/novodex-physics/tools/falsify_queue.py --summary`. Expected: counts per group summing to 1,365 (the spec's figure at the time of writing; record the actual number in the ledger).
- [ ] **Step 6: Commit** `git add docs/reconstruction/novodex-physics/tools/falsify_queue.py docs/reconstruction/novodex-physics/tools/tests/test_falsify_queue.py`.

### Task 2: C++ function span and mutation operators

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/mutate_cpp.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_mutate_cpp.py`

**Interfaces:**
- Produces: `find_function_span(text: str, symbol: str) -> tuple[int, int] | None` (offsets of the body from its opening `{` to the matching `}`; `symbol` is the inventory form such as `OBB::ComputePoints(Point*) const` or a bare `Class::name`; returns `None` when the definition is absent or ambiguous); `cpp_mutations(text: str, lo: int, hi: int) -> list[Mutation]`, `Mutation = (offset, old, new, kind)` with kinds `cmp` (`<` with `<=`, `>` with `>=`, `==` with `!=`), `arith` (`+` with `-`, `*` with `+`), `const` (an integer literal plus one), `neg` (`if(x)` with `if(!(x))`), skipping comments, string literals and preprocessor lines; `apply_mutation(text: str, m: Mutation) -> str`; `spread(muts: list[Mutation], limit: int) -> list[Mutation]` (evenly spread over the list, deterministic).

- [ ] **Step 1: Write the failing tests:** `test_span_finds_member_definition` (a two-function source, asking for `A::f(int)` returns the body of `A::f` only); `test_span_ambiguous_overload_returns_none` (two overloads, symbol without parameter list -> `None`); `test_mutations_skip_comments_and_strings` (a `<` inside `// a < b` and inside `"a < b"` is not offered); `test_apply_changes_exactly_one_site` (the result differs from the input at one place); `test_spread_is_even_and_deterministic` (20 mutations, limit 5 -> 5 items, same twice, first and last included).
- [ ] **Step 2: Run** them. Expected: FAIL.
- [ ] **Step 3: Implement** the four functions with a small tokenizer (no regex over the whole file for the brace match).
- [ ] **Step 4: Run** them. Expected: PASS.
- [ ] **Step 5: Commit** both files.

### Task 3: Compiled-range check

**Files:**
- Modify: `docs/reconstruction/novodex-physics/tools/mutate_cpp.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_mutate_cpp.py`

**Interfaces:**
- Consumes: `read_map(path)` from `close_static_rows.py` (`{symbol: (address, object)}`).
- Produces: `function_range(mapsyms: dict, symbol: str, text_end: int) -> tuple[int, int]` (rva and size: size is the distance to the next higher symbol address, capped at `text_end`); `pe_changed_rvas(before: bytes, after: bytes) -> list[tuple[int, int]]` (merged `[lo, hi)` RVA ranges where the two PE images differ, via the section table; differences in the headers or checksum/timestamp are ignored); `inside(changed: list[tuple[int, int]], rva: int, size: int) -> bool` (true only if `changed` is non-empty and every range lies within `[rva, rva+size)`).

- [ ] **Step 1: Write the failing tests:** `test_inside_requires_all_changes_in_range`; `test_inside_false_when_nothing_changed` (empty -> False); `test_pe_diff_ignores_timestamp_and_checksum` (two synthetic PE byte strings that differ only in the header timestamp -> `[]`); `test_pe_diff_maps_file_offset_to_rva` (one differing byte in a section's raw data -> the RVA range `[va + delta, va + delta + 1)`); `test_function_range_uses_next_symbol`.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement** the three functions (parse the DOS/NT headers and section table by hand; no new dependency).
- [ ] **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit.**

### Task 4: Single-row sweep and survey

**Files:**
- Modify: `docs/reconstruction/novodex-physics/tools/mutate_cpp.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_mutate_cpp.py`

**Interfaces:**
- Consumes: from `mutate_rows.py` `detect_family(stdout, family)`, `detect_harness(stdout, returncode, family, count_crash)`, `detect_staged(output, count_crash)`, `write_atomic(path, data)`, `harness_command(target, extra_args)`, `_build(target, staged)`; from Tasks 2-3 the span, mutation and range functions.
- Produces: `sweep_row(path: str, symbol: str, run, build_dll, limit: int, survey: bool = False, restore=None) -> dict` returning `{'caught': [...], 'survived': [...], 'rejected': [...]}` where `run(mutated_path) -> int | None` returns the detection count or `None`, `build_dll() -> bytes` returns the rebuilt DLL image, and each mutation is rejected (not tried) when `inside(...)` is false. The source is restored in a `finally`.

- [ ] **Step 1: Write the failing tests** with fake `run`/`build_dll` callables: `test_sweep_restores_source_when_run_raises`; `test_sweep_rejects_mutation_outside_range` (the fake DLL builder changes bytes outside the range -> the mutation lands in `rejected`, `run` is not called for it); `test_sweep_stops_at_first_catch_unless_survey`; `test_survey_lists_survivors`; `test_control_run_must_be_exact` (a control that reports a difference raises `ControlNotExact`).
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement** `sweep_row` and a `main()` with `--row ID --inventory PATH --limit N --survey --log PATH` that wires the real `run`/`build_dll` for the row's target (harness targets through `harness_command`, staged targets through the staged path of `mutate_rows.py`); log format identical to `mutate_rows.py` (`CAUGHT phys_fn_N <target> <kind>=<n> <addr> <old> -> <new>` and `SURVEY ...`).
- [ ] **Step 4: Run** the tests. Expected: PASS.
- [ ] **Step 5: Real smoke test:** `python tools/mutate_cpp.py --row phys_fn_<one joint wrapper row> --limit 6 --survey --log .superpowers/sdd/<plan>/smoke.log`; Expected: a SURVEY line, `git diff --quiet` on the mutated source afterwards, no stale mutated file.
- [ ] **Step 6: Commit.**

### Task 5: Batching with attribution

**Files:**
- Modify: `docs/reconstruction/novodex-physics/tools/mutate_cpp.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_mutate_cpp.py`

**Interfaces:**
- Consumes: `batches(rows, size)` from Task 1, `sweep_row` pieces from Task 4.
- Produces: `sweep_batch(rows: list[dict], run_family, build_dll, limit: int) -> dict[str, dict]` keyed by row id: one mutation per row applied together, one build, one run, detections attributed by `row['family']`; a row whose family reports a difference is `caught`, the others `survived`; when two rows' mutations touch the same file the file is edited once per batch round; a row in a failing batch whose family cannot be told apart is re-run alone (bisect). Rows with the same family must not be in one batch (`ValueError`).

- [ ] **Step 1: Write the failing tests:** `test_batch_attributes_by_family` (three rows, fake run reporting a difference only for family B -> only that row is caught); `test_batch_refuses_shared_family`; `test_batch_bisects_when_control_differs_in_one_family`; `test_batch_restores_every_file`.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement** `sweep_batch` and the `--queue FILE --batch K` options of `main()`.
- [ ] **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit.**

### Task 6: Manifest from a log, and closing C++ rows

**Files:**
- Modify: `docs/reconstruction/novodex-physics/tools/mutate_cpp.py` (`--manifest LOG --out FILE`), `docs/reconstruction/novodex-physics/tools/close_static_rows.py` (only if a C++ row is refused)
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_mutate_cpp.py`, `tests/test_close_static_rows.py`

**Interfaces:**
- Produces: `write_manifest(log_path: str, doc: str, gate_for: dict[str, str]) -> dict` in the shape `close_static_rows.py --falsified` reads: `{"doc": ..., "rows": {"phys_fn_N": {"gate", "family", "mutation", "detected", "wired": true}}}`.

- [ ] **Step 1: Write the failing test** `test_manifest_from_log` (a two-line log -> two manifest rows, `wired` true) and, in `test_close_static_rows.py`, `test_falsified_closes_a_cpp_row_that_is_already_driven` (a `reconstructed` row with `implementation` and `dynamic_proof` becomes `dynamically_gated` and its ledger entry moves to `closed`).
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement** `write_manifest`; adjust `close_static_rows.py` only where the test shows it refuses (for example it demands a missing `static_proof` that these rows already have, or demands `wired` false).
- [ ] **Step 4: Run** the new tests and the whole tooling suite (`python -m pytest docs/reconstruction/novodex-physics/tools/tests -q`). Expected: all pass (940 plus the new ones).
- [ ] **Step 5: Commit.**

### Task 7: Pilot, the joint families

**Files:**
- Create: `docs/reconstruction/novodex-physics/evidence/falsify-joints.md`, `docs/reconstruction/novodex-physics/evidence/raw/falsify-joints-<date>/{manifest.json,survey.log}`
- Modify: inventory, closure ledgers, work_units, completion-backlog (by the tools), `docs/reconstruction/novodex-physics/evidence/testing-pass-handoff.md`

- [ ] **Step 1: Queue and plan.** Run Procedure B step 1 for group `joints` (about 200 rows). Measure one build and one gate run; choose the batch size N so that the whole group takes no more than 20 gate runs; record N and the timings in `falsify-joints.md`.
- [ ] **Step 2: First batch as a pilot** (N rows). Expected: a table of caught/survived/rejected; every rejection has a reason (no standalone range, inlined, no change). Fix `mutate_cpp.py` defects found, each with a test first, and commit them separately.
- [ ] **Step 3: Remaining batches** by Procedure B steps 2-8, one commit per two or three batches.
- [ ] **Step 4: Record** closed count, deferred count and reasons in `falsify-joints.md` and the handoff; merge.

Expected: every joint row closed or deferred with a reason; phases 3-7 gates exit 0.

### Task 8: Collision, contact and mesh rows

**Files:** as Task 7 with `falsify-collision.md` and `falsify-mesh.md`.

- [ ] **Step 1-4:** Procedure B for groups `collision` then `mesh`. Rows with no dynamic proof go to Task 12, not here.

Expected: each row closed or deferred; gates green.

### Task 9: Pruner, scene and remaining first-party rows

**Files:** as Task 7 with `falsify-pruner.md`, `falsify-scene.md`, `falsify-other.md`.

- [ ] **Step 1-4:** Procedure B for groups `pruner`, `scene`, `other_first_party`.

Expected: each row closed or deferred; gates green.

### Task 10: Vendored opcode

**Files:** as Task 7 with `falsify-opcode.md`.

- [ ] **Step 1:** Run Procedure B for group `vendored_opcode`, mutating the upstream source under `External/opcode/upstream` (the overlay under `External/opcode/novodex` where the row lives there), gate = the row's vendored family (`NxPhysicsThirdPartyTests` / `NxPhysicsAssetTests`).
- [ ] **Step 2:** A row whose function is inlined or a template instance with no standalone range is deferred `homeless_shared_code` or `not_independently_falsifiable` with the byte-range evidence.
- [ ] **Step 3:** Update `phase4-third-party-map/vendored_coverage.csv` only through the existing vendored tools; never edit it by hand.

Expected: each row closed or deferred; gates green.

### Task 11: Vendored qhull

**Files:** as Task 10 with `falsify-qhull.md`.

- [ ] **Step 1-3:** Same as Task 10 for group `vendored_qhull` (455 rows), upstream under `External/qhull`; split into at least four commits.

Expected: each row closed or deferred; gates green.

### Task 12: Rows without a driver or a mapping, and the unfinished states

**Files:** `docs/reconstruction/novodex-physics/evidence/falsify-undriven.md`, plus whatever drivers are added to `tests/`.

- [ ] **Step 1:** For each `no_driver` row (about 150) take a cdb hit count over the existing gated targets (breakpoint on the row's oracle rva, as in `phase7-closure-audit.md`); a row that is hit is added to the family that runs it.
- [ ] **Step 2:** A row that no target reaches gets the smallest driver that reaches it, or is deferred `unreachable_in_phase_2` / `not_independently_falsifiable` with the hit-count evidence.
- [ ] **Step 3:** For each `no_mapping` row (about 160) find the candidate function by the map and the call graph, record `implementation`/`implementation_symbol`, then Procedure B; otherwise defer.
- [ ] **Step 4:** The 19 `discovered` and 7 `statically_reviewed` rows: classify each (reconstruct, or `classified`/deferred with the reason).

Expected: every row from these groups closed or deferred.

### Task 13: Carried-over rows

**Files:** `docs/reconstruction/novodex-physics/evidence/falsify-carried.md`.

- [ ] **Step 1:** 003649 (FluidManager create): finish the branch audit named in its inventory note, then Procedure B on `FluidManager.cpp`'s create path with the public fluid fixture as gate.
- [ ] **Step 2:** 005386 (model stream load): try the Phase 7 recorder (`NxPhysicsRowCapture`) with a hook at 0x000e94c0 in a scene that loads a mesh from a stream; otherwise keep it deferred with the result.
- [ ] **Step 3:** Revisit the Phase 7 deferrals with whatever scene improvements Tasks 7-12 made available.

Expected: each carried row closed or re-deferred with a dated reason.

### Task 14: Close the sub-project

**Files:** `docs/reconstruction/novodex-physics/evidence/testing-pass-handoff.md`, `inventory.json`, ledgers, `work_units.json`, `completion-backlog.json`.

- [ ] **Step 1:** `python tools/falsify_queue.py --summary`. Expected: zero rows in `reconstructed`, `discovered` and `statically_reviewed` that are not deferred in a closure ledger.
- [ ] **Step 2:** Full gate cycle phases 3-7; `validate_inventory.py` passes; tooling tests pass.
- [ ] **Step 3:** A fresh whole-branch review on the most capable model, with the Review Focus above; fix Critical and Important findings in one pass with a failing test first.
- [ ] **Step 4:** Report counts (closed, deferred by reason), rulings and deferred minors; merge to `main` locally; do not push.
