# Phase 7 closure audit (2026-11-02)

Read-only audit for the plan `docs/superpowers/plans/2026-11-02-phase7-closure.md`.

## What the validator needs to pass Phase 7

Tried by setting Phase 7 to `pass` in `program.json` and `closed` in `inventory.json` and running `validate_inventory.py`
(reverted; nothing kept). The only errors were three missing record fields in `program.json` phase 7:
`gate_artifact`, `evidence_commit`, `implementation_commit`. Phase 6 names `gates/phase6-closure.json` as its gate artifact (Phase 5 names
`gates/phase5.json`; Phases 2-4 name `gates/phase<N>.json`). The validator asks only for a named `gate_artifact`, so Phase 7 names
the existing `gates/phase7-closure.json`, following Phase 6; no `gates/phase7.json` was written (plan Task 9 step 1 amended; Ruling in the
ledger). If a `phase7.json` is wanted later it is a pure record. Phases 2-6 passed with 39/175/972/244 rows
deferred, so deferral with a ledger reason does not block the flip.

## Rows

| item | state | work |
|---|---|---|
| 002296, 002298 | replay matches, every live call is count-zero, 11/16 mutants survive | Task 3 |
| 002300-002310 | not reached by any scene | Task 4 |
| 002350 | replays, 26/40 mutants survive | Task 5 |
| 002332, 002336 | no entry point found | Task 6 |
| 002400 | endless loop | Task 7 |
| 003649 | partial reconstruction (`not_reconstructed_in_phase`); `evidence/phase7-fluid-enabled-create-003649.md` | Task 2 |
| 000604 | `statically_reviewed` | Task 2 |

## Carried obligations

None is discharged; each appears only as an escalation in `gates/phase2.json`, `gates/phase3.json` or `gates/phase4.json`
(and, for the first, `evidence/phase3-leaf-kernels.md`). The validator does not enforce them.

| obligation | status | evidence found |
|---|---|---|
| static_proof_closures_have_no_oracle_side (lock rows 002362/2364/2366) | missing | `tryLock` appears in `tests/PhysicsInternalTests.cpp` only |
| simulation_control_word_scope (000659) | partial | row transcribed in `Scene.cpp`; harness has `kControlSimulate` (0x0f7f) and 86 uses |
| borrowed_phase_5_and_7_layout (contact sink offsets) | missing | none |
| contact_sink_warm_start_axis_survives_reset | missing | none |
| phase4_never_driven_under_simulate_word | partial | `kControlSimulate` is used by the phase 6/7 row families and pruner octree; Phase 4 OPCODE rows not re-measured as a set |
| box_box_contact_frame_overflow | missing | no box-stack scenario |

## Other Phase 7 items

* Public-surface disposition matrix: missing.
* Trajectory scenarios / `NxPhysicsTrajectoryTests` / `NxPhysicsSceneTests`: not in `CMakeLists.txt`.
* Data objects: 502 owned, 0 closed; Phase 5 and 6 carry theirs as `data_object_not_dispositioned` in the closure ledger, so
  Phase 7 does the same (no validator demand beyond that).

## Task 2 findings

* **000604** is already closed in `gates/phase7-closure.json` as `static_proof_falsified` (NxPhysicsInternalTests; the no-op
  mutant of `nxSceneResetBodyRecords` fails the check). Its inventory state `statically_reviewed` is that proof kind. It needs
  nothing further for Phase 7; the destructor-phase effect is not observable from outside the DLL, which is why the oracle
  differential route was not taken.
* **003649** stays deferred as `not_reconstructed_in_phase`. Its public behaviours are already pinned by differentials
  (48-case descriptor matrix, callback order, allocation-failure interleavings, growth, teardown), but the row's own record
  asks for a full 2,452-byte instruction-to-source audit, which is reconstruction work rather than closure work, and the row's
  implementation is C++ in `Scene.cpp`, so `mutate_rows.py` (which mutates listing ranges) does not apply. It moves to the
  next sub-project (falsifying the remaining `reconstructed` rows).

## Final dispositions (Tasks 3-9)

**Finding that changes the earlier record.** A hit count with `cdb` (a breakpoint on each row during the controller scene of
`NxPhysicsSweepCapture`) shows that rows 002300, 002302, 002304, 002306, 002308, 002310, 002332, 002336 and 002400 are all
reached live (35, 38, 35, 11, 11, 35, 2, 4 and 1 hits). The earlier statement that "no scene reaches 002300-002310" and that
002332/002336 had "no entry point" was wrong: those rows had only never been hooked.

**Machinery.** `NxPhysicsRowCapture` (`tests/PhysicsRowCaptureTool.cpp`) records live calls of any row by rva (registers, sixteen
stack words, the sweep-cache global, everything reachable) into `tests/fixtures/row<id>.bin`; `nxReplayOneRow` replays them
against the oracle and the transcription (families `phase7_row_<id>`, registered under `NxPhysicsPhase6RowsTests`, floors
1905/3279). What it took to make the replay deterministic: one arena that keeps the recorded regions at their relative
addresses (pointer order and distance matter to the sweeps), word-aligned windows, the oracle's and the transcription's own
tables canonicalised to the oracle's offsets, heap blocks the row takes from its own heap compared as "a pointer", a recording
stub for 002308's callback (its real target needs the process registries), and the `nxP7CallInitialise` alias corrected to
`NxSceneInternal::createActor` (0x00011730 is `phys_fn_000626`, not a scene initialiser).

| row | replay | mutants caught (spread) | disposition |
|---|---|---|---|
| 002300 | equal, 35 calls | 15/16 | **closed** (`phase7_row_002300`, recorded mutation at 0x58877) |
| 002302 | equal (same function) | 10/16 | deferred `reconstructed_not_falsified`: the later half of the function is reached by few calls |
| 002298 | equal, 34 calls | 5/9 | deferred: the wrapper forwards values that are mostly zero |
| 002304 | equal | 2/16 | deferred: the recorded calls leave the body early |
| 002306 | equal | 2/16 | deferred: same |
| 002308 | equal (callback stubbed) | 10/16 | deferred |
| 002310 | equal, 35 calls | 0/16 | deferred: its first step is an `overlapAABBShapes` query that returns 0 for every recorded call; a 536-call terrain scene did not change that |
| 002332 | equal, 2 calls | 7/16 | deferred: twenty more controller variants made the oracle side corrupt its heap on call 3, so the 2-call fixture stays |
| 002336 | equal, 2 calls | 8/13 | deferred: the scene-level list head it links to is empty in both calls |
| 002296 | (earlier family) | 5/16 | deferred as before |
| 002350 | (earlier family) | 14/40 | deferred as before |
| 002400 | replays without a fault | not applicable | **ruling: deferred, endless by design.** It is the simulation thread's loop (`jmp L5b9e3` unconditional, 0x5ba38); it cannot return, so no differential can complete and a mutation can only be caught as a hang. Kept under its static proof |

The replay families for the deferred rows are kept as regression guards (the transcription equals the oracle on every recorded
call); they are not claimed as falsification.

**Carried obligations.** None of the six is discharged; each is re-escalated, not dropped, and none gates the Phase 7 flip
(the validator does not read them):
* `static_proof_closures_have_no_oracle_side` (lock rows 002362/2364/2366): needs a two-thread contention scenario against the
  oracle; open.
* `simulation_control_word_scope`: 000659 is transcribed and the harness exercises 0x0f7f in the phase 6/7 row families; the
  scene-level assertion that the word is installed across `simulate` is open.
* `borrowed_phase_5_and_7_layout` and `contact_sink_warm_start_axis_survives_reset`: sink offsets still borrowed; open.
* `phase4_never_driven_under_simulate_word`: the Phase 4 OPCODE rows are not re-measured as a set under 0x0f7f; open.
* `box_box_contact_frame_overflow`: no box-stack scenario; open.
All six move to the Phase 8 audit as named items.

**Public-surface disposition matrix and trajectory scenarios:** not written; carried to the Phase 8 audit. The 502 Phase 7 data
objects are carried as `data_object_not_dispositioned`, as Phases 5 and 6 carry theirs.

**Gate record.** `program.json` phase 7 is `pass` with `gate_artifact` `gates/phase7-closure.json` (as phases 5 and 6), and
`inventory.json` marks the phase closed; `validate_inventory.py` passes (548 closed, 12 deferred).

## Review follow-up

* **Fixtures scrubbed.** The first row fixtures held the capture process's environment block (a session token, account and
  organisation ids, user and machine names), because the snapshot follows every private pointer. `NxPhysicsRowCapture` now zeroes
  every run of ten or more printable characters (narrow or wide) in what it stores, and the fixtures were captured again; the
  branch history was rewritten before anything was shared. The two older fixtures already in `main` (`sweep2296.bin`,
  `step2350.bin`) were made without this filter: they carry user and machine names and paths (`C:\Users\...`, the machine's
  domain name) but no token; whether to scrub them is a decision for whoever owns `origin`.
* **Capture parameters** (bytes displaced, optional first-level window, in hex): 002298 `58830 8`; 002300 `58870 6`;
  002304 `58ea0 6`; 002306 `59320 6`; 002308 `59710 6`; 002310 `597c0 5`; 002332 `5a520 6 800`; 002336 `5a880 6 800`.
  Run `NxPhysicsRowCapture <pair dir> tests/fixtures/row<id>.bin <rva> <bytes> [window]`.
* A fault on either side now fails a `phase7_row_<id>` family.
* The listing-compare tool's expected call table named the wrong callee for 0x00011730 (the same mistake as the alias, so the
  compare could not see it); it was corrected and rerun (111 call mappings, 0 unexpected).
* `002304` and `002298` return values are not compared (`useEax` off): 002304's `al` and 002298's pass-through of 002296 are
  boolean results that a later pass should compare byte-wise.
* Known blind spots of the replay digest, which is why the eight families are guards and only 002300 is claimed as falsified:
  bytes between recorded regions, writes through pointers that were not captured (they land in a shared zero buffer), the
  contents of blocks the row allocates, and stored words in the address band that are not floats in the 1e-6..1e6 range.
* Escalations: the six obligations stay recorded in the gate files that raised them and in the Phase 7 plan, and here. The
  Phase 8 plan names five of them already; `borrowed_phase_5_and_7_layout` is not in it, and the validator ties a plan's
  escalations to the gate files, so naming it there needs a gate-file change that this closure does not make.
