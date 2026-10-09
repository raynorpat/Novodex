# Phase 6 closure: ActorPairEffector body-removal notification (`phys_fn_003928`)

`phys_fn_003928` at RVA `0x0008edb0` handles the body-record teardown event
for `ActorPairEffector`. The registered `NxPhysicsEffectorTests` fixture
releases an actor while two effectors observe its record, then checks the
surviving internal body pointers.

In a throwaway archive of current mainline code, replacing
`ActorPairEffector::event` with a no-op caused the candidate to access-violate
during the lifecycle fixture (`oracle_exit=0`, `candidate_exit=-1073741819`,
`stdout_delta=9`, `stderr_exact=True`). Restoring and rebuilding the row
returned an exact differential (`both exits=0`, `stdout_delta=0`,
`stderr_exact=True`).
