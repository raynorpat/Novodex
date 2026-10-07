# Phase 5 `saveToDesc` mutation evidence

`phys_fn_000120` is `NpActorVtable::saveToDesc` at RVA `0x00003690`. The
registered lifecycle differential runs the pinned oracle and rebuilt DLL from
isolated pairs. Its static and dynamic actors now use distinct groups 3 and 7;
the rotated dynamic cases inherit group 7. The baseline is exact
(`stdout_delta=0`, `stderr_exact=True`) and records descriptor pose, density,
flags, group, user data, and preserved body/name/type fields.

Two independent field-source mutations demonstrate row-specific sensitivity:

- Read group from body+0x18 (density) instead of body+0x1c: caught by
  `NxPhysicsActorLifecycleTests` with `stdout_delta=8`.
- Read density from body+0x14 (flags) instead of body+0x18: caught by the same
  target with `stdout_delta=6`.

Both mutated processes exited zero and had exact stderr, so detection came from
the transcript comparison. After each mutation, the original source file was
restored byte-for-byte, `NxPhysics` and `NxPhysicsActorLifecycleTests` were
rebuilt, and the clean differential returned to `stdout_delta=0` with exact
stderr. The intermediate experiment using group zero was insensitive because
the deliberately wrong density value was also zero for the static actor; the
distinct group values correct that fixture weakness and make the group copy
independently observable.

Build and run logs are in the ignored worktree `build/` directory:
`phase5-save-to-desc-distinct-groups-baseline.log`,
`phase5-save-to-desc-group-mutant.log`,
`phase5-save-to-desc-density-mutant.log`,
`phase5-save-to-desc-group-restored.log`, and
`phase5-save-to-desc-density-restored.log`.
