# Phase 5 global orientation quaternion mutation evidence

`phys_fn_000094` is `NpActorVtable::getGlobalOrientationQuatVal` at RVA
`0x00002f30`. The registered `NxPhysicsActorLifecycleTests` differential
compares the static matrix-conversion arm and dynamic identity, rotated, and
quarter-turn actors. Its baseline matches exactly (`stdout_delta=0`,
`stderr_exact=True`).

Changing the dynamic record source from `record+0x5c` to `record+0x60` changes
the rotated and quarter-turn quaternion outputs and is caught with
`stdout_delta=4`. Both mutated processes exit zero and stderr is exact. The
source is restored byte-for-byte, the DLL and lifecycle target are rebuilt,
and the clean differential returns to zero delta with exact stderr.

Build and run logs are in the ignored worktree `build/` directory:
`phase5-orientation-quat-baseline.log`,
`phase5-orientation-quat-mutant.log`, and
`phase5-orientation-quat-restored.log`.
