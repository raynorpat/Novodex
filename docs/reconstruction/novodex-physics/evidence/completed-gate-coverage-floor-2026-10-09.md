# Aggregate completed-gate coverage floor — 2026-10-09

The fresh Win32 `completed` run initially built and ran its differential targets, but the aggregate gate rejected the result: it evaluated 3,224 registered assertions against a floor of 3,406. The individual assertions and differential checks passed. `completed` executes the union of selected phase targets once, while the old floor calculation added each phase's independent floor and counted shared target/line registrations more than once.

`run_phase_gate.ps1` now counts duplicate `(category, target, line)` registrations across the selected phase set and subtracts only that overlap from the aggregate floor. The independent per-phase floors are unchanged. The regression test derives the target union and overlap from the current phase registry and checks that the runner applies the correction.

Validation on 2026-10-09:

- Fresh aggregate gate: `run_phase_gate.ps1 -Phase completed -RepoRoot <worktree> -BuildRoot D:\github\Novodex\build\m0-current-main-clean -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot <pairs-root>` — exit 0, `coverage_assertions_evaluated=3224 floor=3224`, `phase_gate=completed status=pass`. The `completed` selection used the phases currently marked passing through Phase 5 in `program.json`.
- Standalone Phase 5: same roots with `-Phase 5` — exit 0, `coverage_assertions_evaluated=2597 floor=2597`, `phase_gate=5 status=pass`.
- Focused regression: `python docs/reconstruction/novodex-physics/tools/tests/test_gate_targets.py CoverageFloor.test_completed_floor_deduplicates_shared_target_lines` — passes.

Full logs are in the local build tree at `D:\github\Novodex\build\m0-current-main-clean\completed-main-after-coverage-fix.log` and `D:\github\Novodex\build\m0-current-main-clean\phase5-standalone-main-20261009.log`. They are build outputs, not repository evidence inputs. At test time the worktree branch was `codex/nxphysics-step-scene-rows`, based on `c4de8262`, with only the runner, regression test, and this evidence/plan update intended for the change. The aggregate and Phase 5 passes establish gate consistency for this build; they do not establish full-DLL closure or certify Phases 6–8.
