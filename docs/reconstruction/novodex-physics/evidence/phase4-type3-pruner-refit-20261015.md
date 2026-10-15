# Type-3 static-pruner refit differential

The type-3 static pruner is now driven through a focused mode of
`NxPhysicsCollisionTests`. The fixture creates the shipped DLL's type-3 pruner
at RVA `0x000b5090` and a candidate `Type3StaticPruner`, adds two objects,
builds the tree with an AABB query, then marks one object's world bounds
invalid and calls the type-3 `UpdateObject` virtual. Oracle and candidate owner
callbacks refresh the object's box. The follow-up query refits the tree, and
both sides report the moved object with one hit and zero mismatches. Each
callback runs exactly once, and both prunables regain the world-AABB-valid flag.

The control was run through the registered `NxPhysicsType3PrunerTests`
oracle-differential target. Its CMake custom target depends on the existing
`NxPhysicsCollisionTests` executable; both differential runners map the
registered test name to that executable and pass `--type3-pruner-only`. The
Phase 4 runner previously mapped the target only for build/existence checks,
then tried to launch a nonexistent `NxPhysicsType3PrunerTests.exe`. PowerShell
left the prior zero exit code in `$LASTEXITCODE`, so the missing launch was
misreported as success and coverage was absent. The runner now applies the
same mapping and focused-mode flag at launch. Phase 4 now runs the fixture and
requires its oracle-side proof and coverage lines, including the `UpdateObject`
callback count.

Mutation check: replacing `Type3StaticPruner::PrepareQuery`'s
`mTree->Refit2(&builder)` return with `return true` leaves the moved leaf out of
the query. The direct differential reports `oracle_found=1 candidate_found=0`,
one hit on the oracle, zero on the candidate, and `mismatches=1`; the registered
target exits 1. Restoring the refit returns zero mismatches and a passing
focused differential. A second mutation that skips the invalid-box refresh in
`Type3StaticPruner::UpdateObject` is also rejected: the candidate owner callback
is not called, the query misses the moved object, and the fixture reports two
mismatches. Restoring the method returns both callback counts to one and the
differential to zero mismatches.

The fresh full Phase 4 gate completed with `phase_gate=4 status=pass` and
`coverage_assertions_evaluated=536 floor=536`. This establishes the moved-leaf
tree-refit and invalid-world-box refresh paths only. It does not close the
type-3 family: other query types, add/remove edge cases, extended-object
behavior, and public scene selection of pruner type 3 remain to be driven and
compared.

Raw transcripts: [focused restored differential](raw/type3-pruner-refit-20261015/focused-restored-differential.log), [refit-skipped mutation](raw/type3-pruner-refit-20261015/refit-skipped-mutation.log), [update/refit differential](raw/type3-pruner-refit-20261015/update-and-refit-differential.log), [UpdateObject mutation](raw/type3-pruner-refit-20261015/update-object-mutation.log), [restored UpdateObject control](raw/type3-pruner-refit-20261015/update-object-restored-control.log), [restored control](raw/type3-pruner-refit-20261015/restored-control.log), and [Phase 4 receipt](raw/type3-pruner-refit-20261015/phase4-gate-receipt.txt).
