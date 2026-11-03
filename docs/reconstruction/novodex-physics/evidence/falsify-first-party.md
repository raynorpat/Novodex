# Falsifying the first-party C++ rows (sub-project 2)

Method as in `falsify-joints.md`: `mutate_cpp.py` mutates one operator inside the row's own C++ function, rebuilds
`NxPhysics.dll`, requires every changed byte to lie in the one linker-map function named like the row, and runs the row's
gate. A batch screens one row per gate family per build; the first two candidates of each row are then run alone (a row's other
candidates are taken as caught unless one of those fails). A row closes with one confirmed catch and at most 2 survivors;
`close_static_rows.py --falsified` records `differential_falsified` / `oracle_differential_falsified`.
Logs and manifests: `evidence/raw/falsify-first-party-20261103/rerun/` (`survey-<group>.log`, `manifest.json`, `deferrals*.json`).

**Gates.** Staged-pair targets are detected by their stdout delta; `NxPhysicsCollisionTests` rows by their family's own
`mismatches=` line (the family is the one name in the row's proof text that the target prints, if exactly one); oracle-only
targets without families (`NxPhysicsJointTests`) by their output differing from the unmutated run, with the exit code of
the unmutated run as the baseline.

**Checks a closure now has to pass (added after the independent review of the first run).** A control run of the unmutated build
must be exact; for a listing transcription (lines carrying `// 0x........`) the mutated line's oracle address must lie in the
row's own range; a row whose candidate function is more than twice its size plus 64 bytes is shared with other rows and is
not surveyed (`homeless_shared_code`); a mutation must name the row's own source file; a failed build is not a missing change.
The first run of this group had keyed the gate family as `target:family`, so only crashes were ever counted as catches; its 37
closures were reopened (`reopen_rows.py`) and the group re-run.

| group | surveyed | closed | deferred after a survey | not surveyed: control not exact, shared function or not located | no gate or family could be named |
|---|---|---|---|---|---|
| joints | 148 | 18 | 130 | 0 | 0 |
| collision / contact | 19 | 16 | 3 | 5 | 58 |
| mesh | 4 | 2 | 2 | 10 | 69 |
| scene | 2 | 0 | 2 | 9 | 15 |
| pruner | 51 | 27 | 24 | 3 | 16 |
| other first-party | 20 | 8 | 12 | 5 | 164 |
| total | 244 | 71 | 173 | 32 | 322 (321 carry a note: one of them is no longer at reconstructed) |

The pruner closures that had been `blocked_on_later_phase` on `NxPhysicsCollisionTests` (registered to phases 3 and 7) close
now that Phase 7 passes, discharged by phase 7; some rows left as `not_reconstructed_in_phase` by the earlier pass close from
that stale entry.

**What the survivors say.** The gates that exist for most of these rows do not observe their bodies: in the joints 19 rows were
caught with more than 2 survivors, 4 not at all, and in `NxPhysicsJointTests` 21 rows have no operator to mutate. The limit
is drivers, not the tool. Rows that are compiler-generated (the `Np*Joint` adjustor thunks) have no source-level body:
`not_independently_falsifiable`.

**Not done in this pass (792 rows still without a mutation aimed at them):** 636 vendored rows (qhull 455, opcode 181), 114
first-party rows with no dynamic proof, 17 with no implementation mapping and 25 others. The vendored families
(`NxPhysicsThirdPartyTests`) share one target across many rows, so a batch holds only a few rows and 636 rows at eight rounds
each is far beyond this session: they need a faster per-row gate or a decision to accept the matcher's outcome-exact evidence.
