# `phys_fn_000024` actor shape release report

`phys_fn_000024` (`NxPhysics.dll+0x1860`, 328 bytes) is implemented by `nxActorReleaseShape` in `Physics/src/Scene.cpp`. The row handles release re-entry, missing roots, static actors that cannot lose their last shape, grouped child removal and group teardown, mismatched single roots, and deletion of the single root. The registered `NxPhysicsActorShapeMutationTests` fixture covers the full actor shape lifecycle, including the dynamic actor with no shape at `t4_dy_release_empty`.

The clean staged pair reports the pinned oracle and candidate DLL identities and matches exactly: both processes exit 0, `stdout_delta=0`, and stderr is exact. The no-root case emits one `NXE_INVALID_PARAMETER` report at source line `0x1a4`.

For row-specific falsification, changed only that report's error code from `NXE_INVALID_PARAMETER` to `NXE_INVALID_OPERATION`, leaving the branch, source line, and message intact. The target still exits 0 on both sides with exact stderr, while the registered transcript differential rejects the candidate with `stdout_delta=2`. Restoring the original enum, rebuilding, and rerunning returns the exact baseline (`stdout_delta=0`, `stderr_exact=True`; both exit 0). Captured runs: `build/m2-release-shape-baseline-runner.log`, `build/m2-release-shape-report-mutant-runner.log`, and `build/m2-release-shape-final-runner.log`.

Closure: `differential_falsified` on `NxPhysicsActorShapeMutationTests`, `stdout_delta=2`. No public Physics headers changed.
