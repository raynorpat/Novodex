# Phase 5 `saveBodyToDesc` mutation evidence

`phys_fn_000046` is the dynamic actor vtable's descriptor-gather wrapper at
RVA `0x000024c0`. The registered `NxPhysicsActorLifecycleTests` differential
checks a static actor with no body record and dynamic, rotated, and quarter-
turn actors with populated records. The baseline is exact
(`stdout_delta=0`, `stderr_exact=True`).

Replacing the successful result return with `false` preserves descriptor bytes
but flips the three dynamic actors' public success result. The mutation is
caught with `stdout_delta=6`; both processes exit zero and stderr is exact.
The source is restored byte-for-byte, `NxPhysics` and the lifecycle target are
rebuilt, and the clean differential returns to `stdout_delta=0` with exact
stderr.

Build and run logs are in the ignored worktree `build/` directory:
`phase5-save-body-desc-baseline.log`, `phase5-save-body-desc-mutant.log`, and
`phase5-save-body-desc-restored.log`.
