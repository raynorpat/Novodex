# Phase 6 closure: D6 breakability setter (`phys_fn_004445`)

`phys_fn_004445` at RVA `0x000b0760` is `NpD6Joint::setBreakable`. The public D6 staged-pair case sets non-default break force and torque values after construction and reads them back through `NxJoint::getBreakable`.

The restored fixture is exact against the pinned oracle: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily forwarding `0.0f` instead of the caller's maxForce changes the observed force from `418a0000` to `00000000`. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=4`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was confined to the isolated worktree and restored before the exact control run. The fixture adds one registered coverage assertion to each of the Phase 6 and Phase 7 gates.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004445-baseline.log` — fixture baseline, exact output.
- `joint-004445-mutation.log` — forced-zero maxForce mutation, `stdout_delta=4`.
- `joint-004445-restored.log` — restored exact control.

The oracle Physics DLL is SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL is SHA-256 `91ed8aecac8743ebd716215005327b501ff6cf22492a9b7de29d788e5a1f4312`; the restored candidate Physics DLL is SHA-256 `79b7681fee8ca4c70c82f6bb5109fa2066cedb666834a9b41afb7628d04fb9c5`.
