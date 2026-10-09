# Phase 6 closure: D6 name setter (`phys_fn_004453`)

`phys_fn_004453` at RVA `0x000b08e0` is `NpD6Joint::setName`. The public D6 staged-pair case sets the name `d6:phase6-setname` and reads it back through `getName`.

The restored fixture is exact against the pinned oracle: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily replacing the wrapper body with a no-op leaves the candidate name null instead of `d6:phase6-setname`. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=2`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was confined to the isolated worktree and restored before the exact control run. The fixture adds one registered coverage assertion to each of the Phase 6 and Phase 7 gates.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004453-baseline.log` — fixture baseline, exact output.
- `joint-004453-mutation.log` — no-op setter mutation, `stdout_delta=2`.
- `joint-004453-restored.log` — restored exact control.

The oracle Physics DLL is SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL is SHA-256 `bd1f1e65fc6814475b62aee6103bcbec515ee1876ccf7636965934067b35514d`; the restored candidate Physics DLL is SHA-256 `9af83548c36c6c36bd12bf39c78e45c7b22f31266baf67765431aac0348ae728`.
