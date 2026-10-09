# Phase 6 closure: joint limit-point setter (`phys_fn_004109`)

`Joint::setLimitPoint` selects solver-body order, transforms the world-space point into the selected body's frame, stores the local limit point, purges limit planes, and wakes both bodies. The registered public D6 joint case calls `setLimitPoint` and records the returned point even though this SDK reports the optional limit-point feature as unavailable.

With the staged-pair matrix exact against the pinned oracle, temporarily added `1.0f` to the stored local X point and rebuilt NxPhysics.dll. The registered differential caught the mutation with `stdout_delta=2`, both processes exiting zero, and exact stderr; the returned point changed from `bf400000.3fc00000.40100000` to `3e800000.3fc00000.40100000`. The mutation candidate Physics DLL was SHA-256 `15c496dadf774335b169ab3e5fe4b94c1af38bc2e6ce992fb0ae3ee96363f828`.

After restoring the source and rebuilding, the staged differential returned to exact output (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `40bebcff33df2e1eb5822f4e770bc623bfd777e9275d1908c82d3406b097bd20`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-limitpoint-row004109-baseline.log`, `build/joint-limitpoint-row004109-mutant.log`, and `build/joint-limitpoint-row004109-restored.log`.
