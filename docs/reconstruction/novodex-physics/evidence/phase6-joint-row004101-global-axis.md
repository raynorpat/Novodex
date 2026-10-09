# Phase 6 closure: shared joint global-axis setter (`phys_fn_004101`)

`Joint::setGlobalAxis` normalizes the supplied world axis, builds its tangent frame, transforms it into each attached body's local frame, refreshes the world frame, and wakes the bodies. The registered `NxPhysicsJointStagedPairTests` matrix already reaches this row through its public joint construction and D6 `setGlobalAxis` readback.

With the matrix exact against the pinned oracle, temporarily added `1.0f` to the normalized X axis before tangent construction. The registered differential catches the mutation with `stdout_delta=6`, both processes exiting zero, and exact stderr. The D6 replacement-axis readback changes from `3e9b28d0.bf4ee116.3f014cae` to `3fa6ca34.bf4ee116.3f014cae`. The mutation candidate Physics DLL was SHA-256 `6252542a01c3a710138db043492d278568e0f2eba3161e6ba5089e705b496b42`.

After restoring the source and rebuilding, the registered differential is exact (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `77b5db149dfa86bd21b4ff72e3d23d0854696d02cf1dbc110c9904c7e5425498`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. No additional fixture line was needed; the existing readback and matrix coverage exercise this path in both Phase 6 and Phase 7.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-axis-row004101-baseline.log`, `build/joint-axis-row004101-mutant.log`, and `build/joint-axis-row004101-restored.log`.
