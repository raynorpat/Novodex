# Box shape identity slot: `phys_fn_001391`

The Box vtable's slots 14, 15, and 16 share the three-byte identity method at RVA `0x00027f00`. The candidate binds those slots to `BoxShape::nxBoxSelf`, which returns the receiver unchanged.

`NxPhysicsShapeVtableTests` calls Box slot 14 on both the pinned oracle and candidate and checks pointer identity. The clean target reports `shape vtable oracle_digest=ed1294b6 cases=626 failures=0` (`build/shape-self-001391-green.log`).

For falsification, a throwaway `git archive` of commit `e3c27e10` was extracted to the system temp directory. The private `ObjectModel.h` copy changed `nxBoxSelf` to return null. Rebuilding the oracle/candidate test target then reported `shape vtable oracle_digest=ed1294b6 cases=626 failures=1` and exited 1 (`build/shape-self-001391-mutation.log`). That single failed identity comparison is recorded in the closure ledger as `mismatches=1`. The generated project was restored and the target was clean-built from the unchanged mainline header; the baseline again reported zero failures.

The row is closed by the Phase 5 oracle/candidate differential. No public Physics header changed.
