# Falsifying the first-party C++ rows (sub-project 2)

Method as in `falsify-joints.md`: `mutate_cpp.py` mutates one operator inside the row's own C++ function, rebuilds
`NxPhysics.dll`, requires every changed byte to lie in the one linker-map function named like the row, and runs the row's
gate; a batch screens one row per gate family per build and the first candidate of each row is confirmed alone.
A row closes with one confirmed catch and at most 2 survivors; the closer (`close_static_rows.py --falsified`, `source_mutation`
rows) records `differential_falsified` / `oracle_differential_falsified`. Logs and manifests: `evidence/raw/falsify-first-party-20261103/`.

**Gates used.** Staged-pair targets are detected by their stdout delta; `NxPhysicsCollisionTests` rows by their family's
`mismatches=` line (the family is the one name in the row's proof text that the target prints, if exactly one);
`NxPhysicsJointTests` and the other oracle-only targets by their output differing from the unmutated run.

| group | surveyed | closed | deferred after survey | not surveyed (no gate or family could be named) |
|---|---|---|---|---|
| joints | 148 | 18 | 130 | 0 |
| collision / contact | 23 | 7 | 16 | 58 |
| mesh | 4 | 2 | 2 | 69 |
| scene | 4 | 2 | 2 | 15 |
| pruner | 51 | 21 | 30 | 16 |
| other first-party | 22 | 5 | 17 | 164 |
| total | 252 | 55 | 197 | 322 |

The 21 pruner rows had been `blocked_on_later_phase` (their gate, `NxPhysicsCollisionTests`, runs in phases 3 and 7); they
close now that Phase 7 passes, discharged by phase 7. Seventeen phase 4 rows that the earlier pass left as
`not_reconstructed_in_phase` closed from that stale entry.

**What the survivors say.** The gates that exist for most of these rows do not observe their bodies: 19 joint rows were
caught but with more than 2 survivors, 4 not at all, and in `NxPhysicsJointTests` 21 rows have no operator to mutate. The
limit is drivers, not the tool. Rows that are compiler-generated (the `Np*Joint` adjustor thunks, 36 rows) have no
source-level body: `not_independently_falsifiable`.

**Not done in this pass (792 rows still without a mutation aimed at them):** the 636 vendored rows (qhull 455, opcode 181),
114 first-party rows with no dynamic proof, 17 with no implementation mapping and 25 leftovers. See the plan's Tasks 10-13 and
the ruling in the ledger: the vendored families (`NxPhysicsThirdPartyTests`) share a target across many rows, so a batch
holds only a few rows and 636 rows at eight rounds each is far beyond this session; they need a faster per-row gate or a
decision to accept the matcher's outcome-exact evidence.
