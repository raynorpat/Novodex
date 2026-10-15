# Phase 6: Effector scalar deleting destructor (`phys_fn_003938`)

IDA decompilation at RVA `0x0008eec0` identifies the abstract `Effector` slot-1 scalar deleting destructor. It calls the base destructor, conditionally frees `this` through the Foundation allocator when flags bit 0 is set, and returns the original pointer. The candidate map resolves the emitted `??_GEffector@@UAEPAXI@Z` thunk and `Effector` vtable.

`NxPhysicsEffectorTests` now directly invokes the oracle RVA and candidate map thunk on scratch 0x24-byte objects for flags 0 and 1. It checks return value, stack balance, free count, and exact freed size. Both pairs report no free for flag 0 and exactly one 0x24-byte free for flag 1.

For the candidate-only mutation, `Effector::operator delete` was temporarily changed to a no-op. The direct flag-1 probe reported `frees=0`, `freed_size=0`, `mismatches=1`; the candidate target exited 1 against oracle exit 0 (`stdout_delta=12`). Restoring the allocator call and rebuilding returned the focused differential to exact equality (`both exits=0`, `stdout_delta=0`, exact stderr). The captured baseline, mutant, and restored transcripts are compressed under `evidence/raw/phase6-effector-deleting-thunk-003938-20261015/`.

The probe adds two registered coverage markers; each is checked against both oracle and candidate. Full Phase 6 and Phase 7 gates are rerun at `1,668/1,668` and `2,884/2,884`, respectively; compressed transcripts are `phase6-gate.log.gz` and `phase7-gate.log.gz` beside the focused differentials. Public headers are unchanged; the row is closed in Phase 6 while the full-DLL audit remains open.
