# Phase 6 closure: D6 drive setter write-lock paths

This evidence closes `phys_fn_004463` (`NpD6Joint::setDriveOrientation`, RVA `0x000b0ab0`), `phys_fn_004465` (`NpD6Joint::setDriveLinearVelocity`, RVA `0x000b0b10`), and `phys_fn_004467` (`NpD6Joint::setDriveAngularVelocity`, RVA `0x000b0b70`). Their folded internal drive setters are empty, so the public staged-pair fixture marks the scene lock state as owned by another thread and records each wrapper's rejected-write callback. The observed reports are `NXE_INVALID_OPERATION` (`code=2`) at source lines 47, 54, and 61 respectively.

The baseline matches the pinned oracle exactly: both processes exit zero, `stdout_delta=0`, and stderr matches. Each mutation temporarily suppressed only that row's `reportWriteLocked` call. The registered `NxPhysicsJointStagedPairTests` differential caught each missing callback with both processes exiting zero, `stdout_delta=24`, and exact stderr. The restored control returns to `stdout_delta=0`.

The mutations were isolated to the worktree and individually restored before their controls. The fixture adds three registered coverage lines to each of the Phase 6 and Phase 7 gates. Public headers are unchanged.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004463-004467-baseline.log` — all three contention paths, exact output.
- `joint-004463-mutation.log` — orientation-report mutation, `stdout_delta=24`.
- `joint-004465-mutation.log` — linear-velocity-report mutation, `stdout_delta=24`.
- `joint-004467-mutation.log` — angular-velocity-report mutation, `stdout_delta=24`.
- `joint-004463-004467-restored.log` — all three restored exact controls.

The oracle Physics DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate DLL SHA-256 values are `e8a728ac73bde43c7258b4f0b358899cb3752ee4ae4c584667f6f0998aa0e396` (orientation), `35050a8db7a4aa2e4573ae8e5d011673d2c38e0dcac4c92dba964adec1790b4f` (linear velocity), and `017127070decaa9112a84260bb301d26fd10b9cd85bf873242746f5b9e1146eb` (angular velocity). The restored candidate DLL SHA-256 is `bac9db9357cfaa1f482555d68dfb9a07380e6fa4bcc7e0adb823611a52a38bf9`.
