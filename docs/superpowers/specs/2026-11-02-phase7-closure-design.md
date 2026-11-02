# Phase 7 closure design

Sub-project 1 of the four-part completion program (Phase 7 closure; falsifying the remaining `reconstructed` rows; wiring the
verified rows into the shipping DLL; the Phase 8 audit and sign-off). The program's success criterion, as the user stated it:
**the shipping `NxPhysics.dll` is the oracle** -- it behaves like the original using the verified code, and every code row
reaches the terminal state in the Phase 8 audit. This spec covers only the first sub-project.

## Goal

Write `gates/phase7.json`, flip Phase 7 to `pass` in `program.json` / `inventory.json`, with every Phase 7 code row either
`dynamically_gated` (driven, and a recorded mutation inside the row's range was caught) or deferred in
`gates/phase7-closure.json` with a reason the validator accepts. No row is forced.

## Scope

In scope:
* The 12 held rows: 002296, 002298, 002300, 002302, 002304, 002306, 002308, 002310, 002332, 002336, 002350, 002400.
* 003649 (`FluidManager` create, `not_reconstructed_in_phase`: a partial reconstruction whose constructor sub-rows, failure
  ordering and teardown still need a branch-by-branch audit) and 000604 (`statically_reviewed`).
* The obligations the Phase 7 plan carries in from earlier gates: `static_proof_closures_have_no_oracle_side` (phase 2),
  `simulation_control_word_scope`, `borrowed_phase_5_and_7_layout`, `contact_sink_warm_start_axis_survives_reset`,
  `box_box_contact_frame_overflow` (phase 3), `phase4_never_driven_under_simulate_word` (phase 4).
* The public-surface disposition matrix and the trajectory scenarios the Phase 7 plan names.
* The 502 Phase 7 data objects only as far as the validator requires them for the gate.

Out of scope: 005386 (phase 4; belongs to sub-project 2), wiring (sub-project 3), granting `closed` (sub-project 4).

## Outcome rule

Each row ends in exactly one state: `dynamically_gated` with a caught mutation, or deferred with a validator-accepted reason
(for example `not_independently_falsifiable`, or an unreachable reason). Phases 2-6 closed with deferrals, so a deferral is not a
failure; forcing a row with a vacuous drive is. A row that is driven but whose mutants mostly survive is deferred, not closed.

## Approach

1. **Audit (read-only).** Check each carried obligation, the matrix and the trajectory scenarios against the repository and the
   gate files; list what exists, what is partial, what is missing. The size of the remaining plan depends on this result, so it
   is the first task and produces a written table.
2. **Rows, time-boxed per row, spike before build:**
   * 002296 / 002298: a sweep scene whose live calls reach the non-zero-count path (today every capture is count-zero, 11 of 16
     mutants survive). Reuse the live-capture and replay machinery (`PhysicsSweepCaptureTool`, `nxDrivePhase7Replay`).
   * 002300-002310: a shape-versus-box sweep scene. No public API reaches them directly; find the scene (CCD pair kinds) that
     does, or rule them unreachable.
   * 002350: a trigger scene in which triggers enter, leave and re-enter across steps (26 of 40 mutants survive today).
   * 002332 / 002336: spike whether any controller entry point is reachable from the public SDK; otherwise rule.
   * 002400: ruling (endless loop; cannot be driven to completion).
   * 003649: finish the branch audit, then drive through the public fluid API with a recorded mutation.
   * 000604: promote with a driven check.
3. **Gate artifacts.** Discharge or re-escalate each carried obligation, write the disposition matrix and trajectory evidence,
   write `gates/phase7.json`, update the closure ledger, program and inventory, regenerate `work_units.json` and
   `completion-backlog.json`.
4. **Verification.** Phases 1-7 gate cycle green, tooling tests green, inventory validation clean.

## Constraints

* Never push; fast-forward merge to `main` locally; explicit-path `git add`; leave unrelated untracked files and
  `stash@{0}` untouched.
* Commit trailer: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
* Each row gets a spike with a stated stopping point; a spike that does not reach the row ends in a ruling, recorded.
* Final report lists rulings, deferrals and plain status.

## Risks

* The audit may show the carried obligations are larger than the rows (the box-stack overflow scenario and the simulate-word
  re-measurement of Phase 4 rows are real work). The plan is sized from the audit, not from this spec.
* Several rows may end deferred; the gate must still pass, and each deferral needs a reason that survives review.
