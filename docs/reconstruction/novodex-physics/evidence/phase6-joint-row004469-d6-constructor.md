# Phase 6 closure: D6 wrapper constructor (`phys_fn_004469`)

`phys_fn_004469` at RVA `0x000b0bd0` is `NpD6Joint::NpD6Joint(D6Joint*)`. The registered staged-pair fixture creates public D6 joints and exercises their methods and lifecycle.

Temporarily changed the constructor to initialize `NpJointShared<NxD6Joint, D6Joint>` with a null internal pointer. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation: the oracle exits zero; the candidate access-violates (`0xc0000005`) with `stdout_delta=2916` and exact stderr. Restoring the constructor yields both exits zero, `stdout_delta=0`, and exact stderr.

The mutation was isolated to the worktree and restored before the exact control run. This uses existing registered fixture coverage and adds no coverage assertion.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`. Logs: `joint-004469-mutation.log` and `joint-004469-restored.log`. The oracle Physics DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
