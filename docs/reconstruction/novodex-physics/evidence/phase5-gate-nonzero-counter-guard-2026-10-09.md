# Reject nonzero candidate-failure counters in staged gate transcripts

The staged-pair runner now rejects any child transcript line of the form `candidate failures=N` when `N > 0`, including when oracle and candidate print the same nonzero count and the pairwise transcript comparison would otherwise match. It checks both oracle-side and candidate-side child output. Zero counters and unrelated diagnostics are unaffected.

## Verification

The behavior is covered by `tools/tests/test_gate_transcripts.py`: a zero counter passes, a positive counter fails with a gate diagnostic, and an unrelated `candidate mismatches=0` line passes. The temporary runner fixture in `test_gate_targets.py` now copies the transcript-check helper alongside the runner, preserving its isolated registry test.

- Focused tests: 53 passed, 31 subtests passed.
- Full Python tooling suite: 819 passed, 732 subtests passed.
- Completion backlog regenerated after the Phase 7 closure evidence note changed its pinned ledger hash.
- `run_differential.ps1` ResolveOnly and temporary-registry tests passed through the full tooling suite.

The full DLL Phase 5/7 pair gates were not rerun in this slice. The last recorded passes remain Phase 5 at 2,619/2,619 and Phase 7 at 1,447/1,447; those runs predate this new transcript guard. The next full pair-gate run should happen after the committed work is integrated into the mainline, consistent with the existing test sequencing instruction. This closes only the transcript counter blind spot; the broader M1 harness split and full NxPhysics reconstruction remain open.
