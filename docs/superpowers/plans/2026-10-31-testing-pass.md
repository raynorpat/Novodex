# Testing Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire, drive and falsify the 184 first-party rows that the remaining-rows pass reconstructed with a static
proof only, so each becomes `dynamically_gated` with a recorded mutation; then merge to `main`.

**Architecture:**
- Each unit (one transcription file) goes through one procedure:
  - wire its rows into the product;
  - drive them with existing or new gated families;
  - mutate each row and record the caught mutation.
- Two new tools do the mechanical parts: `tools/mutate_rows.py` sweeps mutations, and
  `close_static_rows.py --falsified` writes the records.
- Gates 3–7 pass after each unit's wiring and again after its drives.

**Tech Stack:**
- MSVC Win32 Release (CMake, `build/`) and naked `__asm`;
- Python 3 with capstone (tooling in `docs/reconstruction/novodex-physics/tools`, unittest via pytest);
- PowerShell gates (`run_phase_gate.ps1`, `run_differential.ps1`);
- the C++ test targets in `tests/`.

**Spec:** `docs/superpowers/specs/2026-10-31-testing-pass-design.md`

Paths below are relative to `docs/reconstruction/novodex-physics/` unless they start with `Physics/`, `tests/`,
`External/`, `build/` or `docs/superpowers/` (repo root).

## Global Constraints

- **Branch:** `testing-pass`, cut from `main` once this plan is committed. Merge to `main` at the end. Never push.
- **Oracle:** `D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll`, SHA-256
  `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- **The rows** are the 184 `reconstructed` rows whose notes say `awaits the testing pass`.
- **A row is done** when all of these hold:
  - it is wired (or recorded as having no candidate counterpart, and driven directly);
  - a caught mutation inside its own range is recorded;
  - it is `dynamically_gated`, and its closure entry is in `closed`.
- **Ruling against the spec:** the proof kind follows the gate.
  - A row caught by a staged-pair target (`run_differential.ps1`, detection `stdout_delta=N`) closes as
    `differential_falsified`.
  - A row caught by an oracle-differential harness (`$NxRegisteredOracleDifferentialTargets`, detection `mismatches=N`
    from the named family) closes as `oracle_differential_falsified`.
  - Both make the row `dynamically_gated`. The spec's "`oracle_differential_falsified`" for every row would mislabel
    staged-pair proofs, which the ledgers already record as `differential_falsified`.
- **A mutation counts** only when the gate detects it: `mismatches=N` (N > 0), or `stdout_delta=N` (N > 0) with the
  candidate's exit code not a crash code (`>= 0xC0000000`). A crash, timeout or build failure is logged and not counted.
- **The product follows the oracle.** A test that relied on old C++ behaviour is changed to the oracle's, with the
  reason in the commit.
- **A row that cannot be wired or driven safely, or whose mutations are not caught after its family is extended, is
  held.** It stays `reconstructed` / `reconstructed_not_falsified`, its notes say why, and it goes in
  `evidence/testing-pass-handoff.md`.
- **Every commit:**
  - the full Release build is clean;
  - `python -m pytest tests -q` from `tools/` passes (at least 891 tests);
  - `python tools/validate_inventory.py inventory.json` prints `unexplained=0`;
  - `work_units.json` and `completion-backlog.json` are regenerated.
- **Every unit task** ends with gates 3–7 passing
  (`powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_phase_gate.ps1 -Phase N`, each log ending
  `phase_gate=N status=pass`). Gates run from the repo root.
- **Re-pinning.** When a harness source changes, re-pin `evidence/oracle-only-baselines.json` and copy every retained
  proof in `evidence/oracle-only-proofs/` (the four `phase*` aliases mirror their targets' proofs).
- **New families** are registered in `tools/gate_targets.ps1`: exact families whole, with `$NxPhaseCoverageFloor` raised
  for each phase that registers the target.
- **Files and commits.** Generated `.cpp` files are CRLF. `git add` explicitly by path, never a whole directory with
  untracked files. Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Review Focus

1. **A mutation outside the row.** A split function, or a helper sharing the row's file, could be mutated and credited
   to the wrong row. Expected: candidates come only from the row's own `// 0x…` range. Test:
   `test_candidates_only_inside_the_row_range` (Task 1).
2. **A crash counted as a detection.** A mutation that crashes the candidate also changes stdout. Expected: it is
   logged, not counted. Test: `test_staged_crash_is_not_a_detection` (Task 1).
3. **A mutated source left behind.** A failed build or killed run must not leave the mutation in the file. Expected: the
   file is restored and verified byte for byte, and the sweep stops if it is not. Test:
   `test_source_is_restored_after_a_failed_run` (Task 1). Every unit task also runs `git diff --exit-code` on its
   transcription file after the sweep.
4. **A wired forward changes behaviour nobody drives.** The dropped guards (`nxSceneMarkShapeDirty`,
   `NxContactMeshMesh`) and the reentry refusals are examples. Expected: the unit that wires them adds runs with those
   inputs (null scene or aux block, unregistered shape id, null sink, a call during a locked section), and both sides
   agree. Pinned in Task 3's drive step.
5. **A row recorded with the wrong proof kind or an unregistered gate.** Expected: the tool refuses. Tests:
   `test_falsified_refuses_an_unregistered_gate`, `test_falsified_kind_follows_the_detection` (Task 1).

## File Structure

- **Create `tools/mutate_rows.py`** and its tests in `tools/tests/test_mutate_rows.py`.
- **Modify `tools/close_static_rows.py`** (the `--falsified` mode) and `tools/tests/test_close_static_rows.py`.
- **Per unit:**
  - the product callers being re-pointed (named in the unit task);
  - new families in the test target named by the unit task;
  - `evidence/raw/testing-pass-<unit>-20261031/`, holding `manifest.json` and `mutations.txt`;
  - the unit's existing evidence doc, which gains a "Testing pass" section with the wiring table and the mutation
    record.
- **Create `evidence/testing-pass-handoff.md`** (Task 10); `evidence/remaining-rows-handoff.md` points to it.

## The unit procedure

Tasks 2 to 9 each run this procedure for the rows they list.

1. **Wiring table.** For each row, find the candidate code with its role (the evidence doc's "Not wired" notes, the
   callers in `completion-backlog.json`, the vtable slots) and classify it in the four spec forms: a C++ function, a
   table slot, an inlined copy, or no counterpart. Write the table into the evidence doc.
2. **Wire.**
   - A C++ function becomes a forward to the transcription (or its symbol is aliased), and its old body is deleted.
   - A table slot is assigned the transcription.
   - An inlined copy's call site calls the row where the oracle calls it.
   - Build, then run gates 3–7. A gate that now fails because the old C++ differed from the oracle is fixed by changing
     the test to the oracle's behaviour, with the reason in the commit. If the oracle's behaviour cannot pass, unwire
     that row and hold it.
   - Commit: `Wire <unit>'s transcriptions (<n> rows)`.
3. **Drive.**
   - For each row, pick its gate: the existing family that drives its caller, or a new family. Use a staged-pair target
     where the row is reachable through the public API, and an oracle-differential family otherwise, calling the
     oracle's row by RVA and the candidate's by symbol on identical inputs.
   - New families are exact: words compared, pointers taped as indices, and tape overflow a failure.
   - Register them, raise the floors, build, and run the target. Expected: every family exact, exit 0.
4. **Mutate.** For each row run
   `python tools/mutate_rows.py --source <transcription> --row <id> --target <test target> (--family <name> | --staged)`.
   It reads the row's range from `inventory.json`. Expected: `CAUGHT <id> …`.
   - A row reported `NOT CAUGHT` gets its family extended with inputs that reach it, then another sweep.
   - After a second `NOT CAUGHT` it is held.
   - Run `git diff --exit-code <transcription>` afterwards.
5. **Record.** Write `evidence/raw/testing-pass-<unit>-20261031/manifest.json` from the sweep log (Task 1's format) and
   run `python tools/close_static_rows.py --falsified --manifest <it>`.
6. **Checks and commit.**
   - Run the per-commit checks, re-pin if a harness changed, and run gates 3–7.
   - Add the evidence doc's mutation record: each row with its caught mutation and detection, and held rows with their
     reasons.
   - Commit: `Drive and falsify <unit> (<n> rows)`.

---

### Task 1: Tooling

**Files:**
- Create: `tools/mutate_rows.py`, `tools/tests/test_mutate_rows.py`
- Modify: `tools/close_static_rows.py`, `tools/tests/test_close_static_rows.py`

**Interfaces:**
- **Produces** `mutate_rows.py --source <file> --row <phys_fn_id> --target <test target> (--family <name> | --staged)
  [--limit 6] [--log <file>] [--inventory inventory.json]`.
  - Output per attempt: `0x%08x  <before>  ->  <after>  :  <result>`.
  - Final line: `CAUGHT <id> <gate> <detection> <mutation text>` or `NOT CAUGHT <id>`.
  - Exit code 0 when caught, 1 when not.
- **Produces** the core functions:
  - `candidates(text: str, lo: int, hi: int) -> list[tuple[int, str, str]]`: the (address, before, after) of each
    mutable line whose `// 0x%08x` comment lies in [lo, hi). The mutation kinds are those of
    `evidence/raw/phase4-pruner-octree-20261020/mutate_gen.py`:
    - a jump inverted;
    - an `[reg + off]` offset +4, and `[reg]` → `[reg + 4]`;
    - an immediate +1, and a shift count +1;
    - an x87 operation swapped.
  - `detect_family(stdout: str, family: str) -> int | None`: N from `name=<family> … mismatches=N`.
  - `detect_staged(output: str) -> int | None`: N from `stdout_delta=N`, or None when `candidate_exit` is a crash code
    (`>= 0xC0000000`, or its negative form).
  - `sweep(source, lo, hi, run, limit) -> tuple[str, str, str] | None`. `run(mutated_text) -> (exit, output)` is
    injected; the function returns (mutation text, gate kind, detection) or None. It restores the source after every
    attempt and raises `RuntimeError` if the file's bytes differ from the original.
  - The real `run` builds `--target` with `cmake --build build --config Release --target <t>`.
    - In `--family` mode it runs `build/Release/<t>.exe <oracle dir> <sha256>`.
    - In `--staged` mode it runs
      `powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_differential.ps1 -Targets <t>`.
    - Timeout: 300 s.
- **Produces** `close_static_rows.py --falsified --manifest <m.json>`.
  - Manifest: `{"doc": path, "rows": {id: {"gate": target, "family": name, "mutation": text, "detected":
    "mismatches=N" | "stdout_delta=N"}}}`.
  - Core: `falsify(manifest, inventory, closures, oracle_targets, staged_targets) -> (inventory, closures)`.
  - It sets `state` to `dynamically_gated` and `dynamic_proof` to
    `"<gate> (<family>): a mutation of the row's own instructions (<mutation>) is detected (<detected>). See <doc>."`.
  - It moves the closure entry from `deferred` (reason `reconstructed_not_falsified`) to `closed`, as
    `{id, rva, proof, gate, falsification: {mutation, detected}, driving_phases}`.
    - `proof` is `oracle_differential_falsified` for `mismatches=`, and `differential_falsified` for `stdout_delta=`.
    - `counts`: `deferred_reconstructed_not_falsified` −1, and that proof kind +1.
  - `oracle_targets` and `staged_targets` are parsed from `tools/gate_targets.ps1`
    (`$NxRegisteredOracleDifferentialTargets` for oracle-differential gates, `$NxPhaseTestTargets` for staged-pair gates).

- [ ] **Step 1: Write the failing tests**
  - `test_mutate_rows.py`:
    - `test_candidates_only_inside_the_row_range`: a three-line file with comments 0x1000, 0x1004, 0x1008 and
      lo=0x1004, hi=0x1008 yields only the 0x1004 line.
    - `test_each_mutation_kind`: `je L1` → `jne L1`; `mov eax, dword ptr [ecx + 0x8]` → `[ecx + 0xc]`;
      `cmp eax, 5` → `cmp eax, 6`; `fadd st(0), st(1)` → `fsub …`.
    - `test_detect_family_mismatches`: `"thirdparty name=fam rva=… mismatches=12 …"` → 12; the same line for another
      name → None.
    - `test_detect_staged_delta`:
      `"differential target=T oracle_exit=0 candidate_exit=0 both_exit_zero=True stdout_delta=7 …"` → 7.
    - `test_staged_crash_is_not_a_detection`: `candidate_exit=-1073741819 … stdout_delta=40` → None.
    - `test_sweep_stops_at_the_first_caught`: a fake `run` reporting 0 then 3 mismatches returns after the second
      attempt.
    - `test_sweep_gives_up_at_the_limit`: a fake `run` that never detects, limit 2, returns None after two attempts.
    - `test_source_is_restored_after_a_failed_run`: a fake `run` that raises leaves the file byte-identical.
  - `test_close_static_rows.py`:
    - `test_falsified_closes_the_row`: the state is `dynamically_gated`; the closed entry carries
      `proof='oracle_differential_falsified'`, the gate and the falsification; the counts move by one.
    - `test_falsified_kind_follows_the_detection`: `stdout_delta=4` with a staged gate gives `differential_falsified`.
    - `test_falsified_refuses_an_unregistered_gate`.
    - `test_falsified_refuses_a_zero_detection` (`mismatches=0`).
    - `test_falsified_refuses_a_row_not_awaiting_the_testing_pass`.
- [ ] **Step 2: Run them to verify they fail**
  - Run: `cd tools && python -m pytest tests/test_mutate_rows.py tests/test_close_static_rows.py -q`
  - Expected: the new tests FAIL (the module and the mode are missing).
- [ ] **Step 3: Implement both, to the interfaces above.**
- [ ] **Step 4: Run them to verify they pass**
  - Run the same command. Expected: PASS.
  - Then `python -m pytest tests -q`. Expected: at least 904 passed.
- [ ] **Step 5: Commit:** `Add mutate_rows and a falsified mode to close_static_rows`.

### Task 2: The internal shape rows (18)

**Rows:** 001303 001305 001307 001309 001311 001313 001317 001319 001321 001325 001327 001331 001333 001335 001337 001339
001341 001345 (`Physics/src/ShapeRows.cpp`; `evidence/phase3-shape-rows.md`).

**Wiring specifics:**
- **001305:** change `tests/PhysicsShapeVtableTests.cpp` so its guard words bind the candidate's `gParameter` (elements
  44 and 48), then make `BoxShape::nxDebugRender` forward to 001305.
- **001309:** add the scene globals' sources to the ObjectModel-only targets in `CMakeLists.txt`, then make
  `ShapeBase::nxShapeGlobalPose` forward to it.
- **001303** (CCD path) and **001345** (BASE vtable): no counterpart, so they are driven directly in
  `NxPhysicsCollisionTests`.
- The others are wired already.

**Gates:** the existing families first: `NxPhysicsActorDynamicSetterTests` (001309, 001313, 001321),
`NxPhysicsActorShapeMutationTests` (001307, 001331–001341), `NxPhysicsObjectLayoutTests` (001305, 001325) and
`NxPhysicsSimulationTests` (001303). New families for the rest.

- [ ] Run the unit procedure.

### Task 3: The shape-handle rows (28) and 001876

**Rows:** 001099 001101 001103 001105 001131 001133 001135 001137 001139 001141 001143 001145 001147 001151 001153 001157
001167 001181 001211 001213 001215 001217 001225 001227 001229 001231 001233 001235 (`NpShapeHandleRows.cpp`) and 001876
(`ContactMeshMeshRow.cpp`). All are wired.

**Drives:**
- A staged-pair family in `NxPhysicsActorShapeMutationTests` calls every handle method, through each of the five shape
  families' public tables, on live scenes on both DLLs.
- It includes calls made while the API reentry flag is held, which must be refused with the reentry report on both
  sides.
- 001876 is driven by the mesh-mesh contact path in `NxPhysicsCollisionTests`, with a null sink and a null geometry
  image among its inputs (the dropped guards).
- `nxSceneMarkShapeDirty` (001325, Task 2's unit) gets runs with a null aux block and an unregistered shape id in this
  task's family.

- [ ] Run the unit procedure.

### Task 4: NpTriangleMeshRows (24)

**Rows:** 002095 002097 002098 002100 002101 002103 002104 002106 002108 002110 002112 002114 002116 002118 002120 002122
002124 002126 002128 002130 002132 002134 002136 002138 (`evidence/phase4-np-triangle-mesh-rows.md`).

**Gate:** the public triangle-mesh API through `NxPhysicsTriangleMeshApiTests` (staged pair); the wiring replaces the
candidate's `NpTriangleMesh` methods.

- [ ] Run the unit procedure.

### Task 5: TriangleMeshRows (16)

**Rows:** 001524 001528 002077 002178 002184 002190 002192 002194 002200 002205 002239 002243 002245 002247 002255 002256
(`evidence/phase4-triangle-mesh-rows.md`).

**Gate:** `NxPhysicsTriangleMeshApiTests` (002239's caller is driven there); an oracle-differential family in
`NxPhysicsCollisionTests` for the internal ones.

- [ ] Run the unit procedure.

### Task 6: PMapRows (13)

**Rows:** 001988 001992 001994 001996 001998 002000 002002 002004 002006 002010 002012 002014 002031
(`evidence/phase4-pmap-rows.md`).

**Gate:** `NxPhysicsAssetTests` (001988's caller) and `NxPhysicsPMapResolution80Tests`.

- [ ] Run the unit procedure.

### Task 7: PrunableRows (2) and PrunerSupportRows (46)

**Rows:**
- 001935 001947 (`PrunableRows.cpp`);
- 004790 004793 004799 004801 004820 004822 004824 004828 004830 004832 004834 004852 004870 004872 005231 005262 005263
  005279 005380 005386 005404 005448 005452 005454 005456 005458 005460 005462 005466 005470 005471 005473 005479 005539
  005566 005568 005586 005588 005590 005606 005608 005610 005624 005642 005644 005646 (`PrunerSupportRows.cpp`;
  `evidence/phase4-pruner-support-rows.md`).

**Wiring specifics:**
- **001935:** the visualisation walk passes the oracle's callback shape.
- **001947:** link the scene globals into the ObjectModel-only targets (as for 001309).
- **The pruners, the pool and box pruning** move from their C++ to the transcriptions, one subsystem per commit:
  - the stream read/write and the memory stream;
  - the sorters;
  - the process pool;
  - the factory and the owner adapters;
  - the pruners' slots;
  - the pool save/load.

**Gates:** `NxPhysicsAssetTests` (004790–004801), `NxPhysicsPopulatedSceneTeardownTests` (004828, 004834),
`NxPhysicsSceneRaycastTests` (004830, 004832, 005404), `NxPhysicsSceneVisualizeTests` (005279),
`NxPhysicsSimulationTests` (004820, 004824), `NxPhysicsActorShapeMutationTests` (004852, 005448) and
`NxPhysicsCollisionTests` (005479). Oracle-differential families in `NxPhysicsCollisionTests` cover the rest.

- [ ] Run the unit procedure, one subsystem per wiring commit.

### Task 8: Phase6Rows (10)

**Rows:** 004159 004161 004169 004171 004401 004405 004413 004537 004759 004761 (`evidence/phase6-rows.md`).

**Gate:** `NxPhysicsSimulationTests` (004169); the Phase 6 targets for the rest.

- [ ] Run the unit procedure.

### Task 9: Phase7Rows (26)

**Rows:** 002268 002270 002274 002276 002278 002280 002282 002284 002286 002288 002290 002292 002294 002296 002298 002300
002302 002304 002306 002308 002310 002316 002332 002336 002350 002400 (`evidence/phase7-rows.md`).

**Gate:** `NxPhysicsSimulationTests` (002350); the Phase 7 targets for the rest. The block from 002268 to 002316 is wired
in commits of at most about 3 KB of listing each, callee first, with gate 7 after each.

- [ ] Run the unit procedure.

### Task 10: Hand-off and records

**Files:** create `evidence/testing-pass-handoff.md`; modify `evidence/remaining-rows-handoff.md`.

- [ ] **Step 1:** Generate the hand-off from `inventory.json`:
  - the closed rows by unit, with their gates and proof kinds;
  - the held rows with their reasons;
  - every wiring change that altered product behaviour, with the test that pins it.
- [ ] **Step 2:** Point `remaining-rows-handoff.md` at it.
- [ ] **Step 3:** Run `report_completion.py`. Expected: no row's notes still say `awaits the testing pass` except held
  rows.
- [ ] **Step 4:** Run gates 3–7, the tooling tests and the validator. All pass.
- [ ] **Step 5: Commit:** `Hand off the testing pass`.
