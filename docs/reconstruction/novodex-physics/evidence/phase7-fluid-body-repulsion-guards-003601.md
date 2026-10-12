# Fluid emitter body-repulsion guard paths (`phys_fn_003601`)

IDA at RVA `0x000881c0` shows three behaviorally distinct guard paths before
body application: a null actor returns zero; a present actor with emitter flag
bit 4 clear returns the actor pointer; and a set flag calls
`EmitterGetEnabledFlag`, continuing whenever that result is nonzero. The
enabled path fetches force and torque, gets particle mass in x87 `ST(0)`, and
passes the scaled vectors to the reconstructed impulse/wake path. The
force-application callee is `void`, so its post-call EAX value is not a stable
return contract.

`NxPhysicsFluidEmitterBodyRepulsionTests` now exercises those paths against
the pinned oracle and map-resolved candidate in isolated pairs. It verifies
the actor-pointer return with bit 4 clear, the null-actor return, callback
suppression when the enabled result is zero, and continued callback flow for
a negative nonzero enabled result. The negative case checks all three
callbacks and deliberately does not assert the helper's EAX. The existing live
body case still compares callback arguments and vector/mass words, applied
body-state words, and stack balance.

The new negative-result regression was red before the production fix:
oracle exit 0, candidate exit 1, and `stdout_delta=2`; the candidate's signed
`<= 0` check incorrectly rejected the negative nonzero result. The flag-disabled
regression also caught the prior zero return where the oracle returned the
actor pointer. The candidate now uses the oracle's zero-only enabled check and
returns the actor pointer on the flag-disabled branch. The restored focused
oracle/candidate differential passes with `stdout_delta=0` and exact stderr.

Fresh Phase 5 passes at 2,670/2,670 coverage assertions and Phase 7 passes at
1,697/1,697. The reconstruction tooling suite passes all 810 tests; inventory
validation reports 6,338 functions, 5,138 data objects, and zero unexplained
rows. Both the worktree and pinned Unreal public-header roots verify all 80
files. Full gate logs and the tooling transcript are in
`evidence/raw/phase7-fluid-body-repulsion-guards-003601-20261012/`. This
row-specific mutation falsification changes the enabled callback guard from
`== 0` to `<= 0`. The candidate then stops after its first callback while the
oracle continues through all three (`oracle_exit=0`, `candidate_exit=1`,
`stdout_delta=2`, exact stderr). The restored control is exact (`both exits=0`,
`stdout_delta=0`, exact stderr). Captured runs are
`evidence/raw/phase7-fluid-body-repulsion-guards-003601-20261012/phys_fn_003601-negative-mutant.log`
and `.../phys_fn_003601-restored-control.log`. This mutation closes the full
233-byte row in the Phase 7 ledger.
