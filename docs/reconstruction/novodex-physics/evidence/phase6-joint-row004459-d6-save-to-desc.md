# Phase 6 closure: D6 descriptor save (`phys_fn_004459`)

`phys_fn_004459` at RVA `0x000b09f0` is `NpD6Joint::saveToDesc`. The registered public D6 staged-pair case saves its descriptor and records its saved anchors and other fields.

The baseline candidate matches the pinned oracle exactly: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily replacing `NpD6Joint::saveToDesc` with a no-op changes the saved D6 anchor0 from `40000000.40800000.00000000` to zero (and also clears anchor1). The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=92`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was isolated to the existing worktree and restored before the exact control run. This closes an existing fixture assertion; it adds no new coverage assertion.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004459-baseline.log` — fixture baseline, exact output.
- `joint-004459-mutation.log` — no-op save mutation, `stdout_delta=92`.
- `joint-004459-restored.log` — restored exact control.

The oracle Physics DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The restored candidate Physics DLL SHA-256 is `a4f0d0cb60650242368cc6a1df7e63fd4fc68b8a19a77b46b9ec95ef4eac8a48`.
