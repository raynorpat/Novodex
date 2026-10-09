# Phase 6 closure: D6 limit-plane refusal (`phys_fn_004449`)

`phys_fn_004449` at RVA `0x000b0820` is `NpD6Joint::addLimitPlane`. The public D6 staged-pair case calls it with a non-default normal and point, then resets and reads the limit-plane iterator. The pinned DLL returns false and exposes no plane in this fixture; the test records both results explicitly.

The restored fixture is exact against the pinned oracle: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily returning true after forwarding the operation changes the observed `added` value from 0 to 1. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=2`, and exact stderr. Restoring the implementation returns the differential to zero. This closes the wrapper's observed refusal result for this public D6 case; it does not claim a successful D6 limit-plane insertion path.

The mutation was confined to the isolated worktree and restored before the exact control run. The fixture adds one registered coverage assertion to each of the Phase 6 and Phase 7 gates.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004449-baseline.log` — fixture baseline, exact output.
- `joint-004449-mutation.log` — forced-true return mutation, `stdout_delta=2`.
- `joint-004449-restored.log` — restored exact control.

The oracle Physics DLL is SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL is SHA-256 `394da13d3002b6ea77e043dd23ad6b6972ac2ad9d18338cbcbf99a86896e7c69`; the restored candidate Physics DLL is SHA-256 `e7b6f0663ab1e9552d93fdab862d3de9afccc6e149ebd52cd434362a8e608723`.
