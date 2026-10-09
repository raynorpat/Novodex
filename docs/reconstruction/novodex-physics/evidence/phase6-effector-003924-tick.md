# Phase 6 closure: ActorPairEffector tick thunk (`phys_fn_003924`)

`phys_fn_003924` at RVA `0x0008ed50` is the virtual slot-2 thunk in
`ActorPairEffector::tick`, which dispatches slot 3 with the two body records.
The approved three-step off-center spring/damper fixture reaches this thunk
through the effector vtable; the registered `NxPhysicsSimulationTests`
staged-pair differential is its behavioral observer.

In a throwaway archive of current mainline code, replacing the thunk body with
a no-op was detected by that differential (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=4`, `stderr_exact=True`). Rebuilding after restoring the row
returned an exact control (`both exits=0`, `stdout_delta=0`,
`stderr_exact=True`).
