# Phase 7 Closure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Write `gates/phase7.json` and flip Phase 7 to `pass`, with every Phase 7 code row either `dynamically_gated` (driven, recorded mutation caught) or deferred in `gates/phase7-closure.json` with a validator-accepted reason.

**Architecture:** An audit sizes the work; each of the 14 rows then gets a time-boxed spike that ends in a driven differential (new family in `tests/PhysicsCollisionTests.cpp`, registered in `gate_targets.ps1`, falsified with `mutate_rows.py`, closed with `close_static_rows.py --falsified`) or a recorded ruling. The gate artifacts and status flip come last.

**Tech Stack:** C++ (MSVC, x86 naked `__asm` transcriptions in `Physics/src/Phase7Rows.cpp`), CMake/MSBuild, PowerShell gate scripts, Python tooling under `docs/reconstruction/novodex-physics/tools/`, cdb for oracle inspection.

**Spec:** `docs/superpowers/specs/2026-11-02-phase7-closure-design.md`

## Global Constraints

* Never push. Fast-forward merge to `main` locally. Explicit-path `git add` only.
* Leave unrelated untracked files and `stash@{0}` untouched; never stash/pop without checking `git stash list`.
* Commit trailer: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
* No row is forced. A row driven but with most mutants surviving is deferred, not closed. A spike that does not reach its row ends in a ruling.
* Every gate cycle: register expected lines and floors in `gate_targets.ps1` (`$NxPhaseCoverageFloor`, and the `MINIMUM` pin in `tools/tests/test_gate_targets.py`); update `fixture_source_sha256` in `evidence/oracle-only-baselines.json` (CRLF-normalised sha256 of `tests/PhysicsCollisionTests.cpp`); run `run_phase_gate.ps1 -Phase 3` through `7`, all exit 0; copy only the named `build/oracle-proofs/NxPhysics*.json` into `evidence/oracle-only-proofs/` (plus the four duplicates: phase5-shape-vtable-boxsweep, phase6-joint-desc, phase6-joint-matrix, phase7-joint-support); regenerate `work_units.json` and `completion-backlog.json`; tooling tests (940) pass.
* Python edits to C source with backslashes use `chr(92)`; no heredoc `\\n`.
* Oracle: `D:/FlamingEnt__/Unreal_3/Binaries`, sha256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` (override with `NX_ORACLE_DIR`).

## Review Focus

1. A driven row whose mutants mostly survive being closed anyway -> Task 3-7 each require a survey with no more than 2 survivors out of 16, else defer.
2. A deferral reason the validator rejects, leaving the gate unreachable -> Task 9 runs `validate_inventory.py` after every ledger edit.
3. A carried obligation silently dropped -> Task 8 produces one ledger line per obligation: discharged (with evidence path) or re-escalated.
4. Drives that only reach count-zero / no-pair paths (as 002296 and 002350 did) -> each scene task states the label or counter that must be non-zero (below).
5. Oracle-address-dependent digests making a family flaky -> each new family is run 3 times in each mode and the digest must not change.

---

### Task 1: Audit the Phase 7 closure obligations

**Files:**
- Create: `docs/reconstruction/novodex-physics/evidence/phase7-closure-audit.md`

**Interfaces:**
- Produces: a table with one row per item (14 rows; 6 carried obligations; the disposition matrix; the trajectory scenarios; the 502 data objects) and columns `status` (done / partial / missing), `evidence`, `work`. Tasks 2-9 read it.

- [ ] **Step 1: For each carried obligation in `docs/superpowers/plans/2026-08-09-novodex-physics-phase7-scenes-simulation.md` (lines 44-49: `static_proof_closures_have_no_oracle_side`, `simulation_control_word_scope`, `borrowed_phase_5_and_7_layout`, `contact_sink_warm_start_axis_survives_reset`, `phase4_never_driven_under_simulate_word`, `box_box_contact_frame_overflow`) grep the repo, `gates/*.json` and `evidence/` for work that discharges it; record status and evidence path.**
- [ ] **Step 2: Check whether a public-surface disposition matrix and trajectory scenarios exist (`grep -ril "disposition" docs/reconstruction/novodex-physics/evidence`, `NxPhysicsTrajectoryTests`, `NxPhysicsSceneTests` in `CMakeLists.txt`).**
- [ ] **Step 3: Read what the validator requires for a phase to close** (`tools/validate_inventory.py` around the closure-ledger checks, lines ~1233-1400, and `gates/phase6-closure.json` + `gates/phase5-closure.json` as the model) and record the required `gates/phase7.json` shape and the accepted deferral reasons.
- [ ] **Step 4: For the 502 Phase 7 data objects, record what `validate_inventory.py` needs of them for the gate (state `classified`, or listed in the closure ledger).**
- [ ] **Step 5: Commit** `git add docs/reconstruction/novodex-physics/evidence/phase7-closure-audit.md` -- "Audit the Phase 7 closure obligations".

Expected: the audit lists every item with a status; no code or gate files change.

### Task 2: Promote 000604 (statically reviewed) and finish 003649 (FluidManager create)

**Files:**
- Modify: `Physics/src/Scene.cpp` (000604 at 0x000110b0), `Physics/src/fluids/FluidManager.cpp` (003649 at 0x0008a000), `tests/PhysicsCollisionTests.cpp` or the existing fluid lifecycle fixture named in `evidence/phase7-fluid-enabled-create-003649.md`
- Modify: `docs/reconstruction/novodex-physics/tools/gate_targets.ps1`

**Interfaces:**
- Consumes: Task 1 audit rows for 000604 and 003649.
- Produces: manifest `evidence/raw/testing-pass-phase7-fluid-<date>/manifest.json` for `close_static_rows.py --falsified`.

- [ ] **Step 1: Read `evidence/phase7-fluid-enabled-create-003649.md` and the 003649 listing; list the branches (constructor sub-rows, failure ordering, teardown) not yet exercised.**
- [ ] **Step 2: Write one differential case per unexercised branch in the existing public fluid fixture; run it against oracle and candidate and watch the cases that expose a gap fail (RED).**
- [ ] **Step 3: Fix `FluidManager.cpp` until the cases agree with the oracle (GREEN); run the fixture.**
- [ ] **Step 4: For 000604 drive the row through the nearest public call and compare with the oracle.**
- [ ] **Step 5: Survey: `python docs/reconstruction/novodex-physics/tools/mutate_rows.py --source <file> --row <NNNNNN> --target <fixture target> --survey --skip 2 --limit 16 --log <log>`; accept only <= 2 survivors, else defer with a ruling in the audit.**
- [ ] **Step 6: Record a mutation (non-survey, `--count-crash`) and write the manifest; commit.**

Expected: both rows closed or deferred with a recorded reason.

### Task 3: Sweep scene reaching the non-zero path of 002296 / 002298

**Files:**
- Modify: `tests/PhysicsSweepCaptureTool.cpp` (scene), `tests/PhysicsCollisionTests.cpp` (`nxDrivePhase7Replay`), `tests/fixtures/sweep2296.bin`

**Interfaces:**
- Consumes: existing capture hook at 0x583e0 (displaced `sub esp,0x104`), replay machinery `nxDrivePhase7Replay`.
- Produces: a refreshed fixture whose recorded calls include count > 0 (check by decoding the count argument per call in the capture log).

- [ ] **Step 1: Extend the capture scene so at least 4 recorded calls have a non-zero count (print the counts from the capture tool; this is the check that must be non-zero).**
- [ ] **Step 2: Re-capture; run `phase7_replay`; the transcription must match on every call (digest equal).**
- [ ] **Step 3: Survey 002296 and 002298; accept only <= 2 survivors of 16.**
- [ ] **Step 4: If the survey passes, record the mutation, register the changed expected line (`checks`, `oracle`, `words`), floors, baseline sha, run the gate cycle, close the rows; otherwise write the ruling.**
- [ ] **Step 5: Commit.**

Expected: 002296 and 002298 closed, or deferred with the survey table in the audit.

### Task 4: Shape-versus-box sweep scene for 002300-002310

**Files:**
- Modify: `tests/PhysicsSweepCaptureTool.cpp`, `tests/PhysicsCollisionTests.cpp`, `tests/fixtures/`

**Interfaces:**
- Consumes: Task 3's capture scene and replay.
- Produces: per-row call counts proving each of 002300, 002302, 002304, 002306, 002308, 002310 was reached (count hits with a cdb breakpoint at each RVA during the capture scene, as was done for 005566: `bp NxPhysics+0x<rva>` and count).

- [ ] **Step 1: Spike (time-box: one session). Using cdb hit counts, find which CCD pair kinds (sphere/capsule/box/mesh versus box) make each row run; if none do, write the unreachable ruling and stop this task for that row.**
- [ ] **Step 2: Add those shapes to the capture scene; capture; replay; require transcription equals oracle.**
- [ ] **Step 3: Survey each reached row; <= 2 survivors of 16 to close, else defer.**
- [ ] **Step 4: Gate cycle for the rows closed; commit.**

### Task 5: Trigger enter/leave/re-enter scene for 002350

**Files:**
- Modify: `tests/PhysicsStepCaptureTool.cpp`, `tests/fixtures/step2350.bin`, `tests/PhysicsCollisionTests.cpp` (`nxDrivePhase7StepReplay`)

**Interfaces:**
- Consumes: the existing step capture (hook at 0x5ae50, `NxReplayAllocator`).
- Produces: a label-hit report (breakpoint on each of the row's labels, as in `hits2.cmd`) showing the second half of the row (removal of ended pairs, the `+0x5fc..0x604` event vector) executes.

- [ ] **Step 1: Make triggers enter, leave and re-enter across steps in different orders and with multiple bodies; require that the label-hit report shows the previously unreached labels hit.**
- [ ] **Step 2: Re-capture, replay, require equality.**
- [ ] **Step 3: Survey 40 spread mutants; require <= 4 survivors (today 26) to close; else keep deferred.**
- [ ] **Step 4: Gate cycle if closed; commit.**

### Task 6: Controller reachability spike for 002332 / 002336

**Files:**
- Modify (if reachable): `tests/PhysicsCollisionTests.cpp`; evidence in `evidence/phase7-closure-audit.md`

- [ ] **Step 1: Find callers of 0x5a520 / 0x5a880 in the oracle (xrefs) and the public SDK entry (exports, headers) that reach them; try to call it on the oracle under cdb with a breakpoint at each row.**
- [ ] **Step 2: If reached, add a replay/drive family, survey (<= 2 of 16 survivors), close, gate cycle. If not reached after the spike, write the unreachable ruling.**
- [ ] **Step 3: Commit.**

### Task 7: 002400 ruling

**Files:**
- Modify: `evidence/phase7-closure-audit.md`

- [ ] **Step 1: Re-read the 002400 listing (0x5b9e0, 90 bytes) and the testing-pass handoff entry; confirm it cannot terminate when driven.**
- [ ] **Step 2: Record the deferral with the reason chosen from the validator's accepted vocabulary (Task 1 step 3) and the evidence; commit.**

### Task 8: Carried obligations, disposition matrix and trajectory scenarios

**Files:**
- Create/Modify: per the Task 1 audit (expected: `evidence/phase7-*.md`, a trajectory fixture registered in `CMakeLists.txt`, `gates/phase7-closure.json` notes)

**Interfaces:**
- Consumes: Task 1 table.
- Produces: one ledger line per obligation: discharged (evidence path) or re-escalated (to Phase 8, with the open question).

- [ ] **Step 1: For each audit row marked partial/missing, write the smallest test that discharges it (RED), then the evidence (GREEN). Box stack for `box_box_contact_frame_overflow`; re-measure the Phase 4 rows named in `phase4_never_driven_under_simulate_word` under control word 0x0f7f; contended `tryLock` for the lock rows.**
- [ ] **Step 2: Anything that cannot be discharged inside the time-box is re-escalated in writing, not dropped.**
- [ ] **Step 3: Commit each obligation separately.**

### Task 9: Write the gate, flip the status, merge

**Files:**
- Create: `docs/reconstruction/novodex-physics/gates/phase7.json`
- Modify: `gates/phase7-closure.json`, `program.json`, `inventory.json`, `work_units.json`, `completion-backlog.json`, `evidence/testing-pass-handoff.md`

- [ ] **Step 1: Write `gates/phase7.json` in the shape found in Task 1 step 3; update the closure ledger so every Phase 7 row is closed or deferred with an accepted reason.**
- [ ] **Step 2: `python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json` -- Expected: pass.**
- [ ] **Step 3: Full gate cycle (Global Constraints) -- Expected: phases 3-7 exit 0; tooling tests pass.**
- [ ] **Step 4: Commit explicit paths; `git checkout main && git merge --ff-only <branch>`. Do not push.**
- [ ] **Step 5: Final report listing rulings, deferrals and plain status.**
