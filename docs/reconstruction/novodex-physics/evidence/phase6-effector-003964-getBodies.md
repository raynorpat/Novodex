# Phase 6 closure: SpringAndDamperEffector::getBodies (`phys_fn_003964`)

`phys_fn_003964` at RVA `0x0008f390` converts each effector-local anchor back
to world space and returns the owning actor pointers. The registered
`NxPhysicsCoreDumpTests` fixture serializes this state for effectors whose two
ends are dynamic bodies.

In a throwaway archive of mainline `4124a43e`, adding `1.0` to the first
returned world-space anchor X coordinate was detected by the registered
Phase 6 staged-pair differential (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=10`, `stderr_exact=True`). Restoring and rebuilding the row
returned the exact control (`both exits=0`, `stdout_delta=0`,
`stderr_exact=True`).
