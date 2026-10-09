# Phase 6 closure: D6 limit-point setter (`phys_fn_004447`)

`phys_fn_004447` at RVA `0x000b07c0` is `NpD6Joint::setLimitPoint`. The public D6 staged-pair case sets the point `(-0.75, 1.5, 2.25)` on body 1 and observes the point returned by `getLimitPoint`.

The restored fixture is exact against the pinned oracle: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily forwarding the zero vector instead of the caller's point changes the observed point from `bf400000.3fc00000.40100000` to `00000000.00000000.00000000`. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=2`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was confined to the isolated worktree and restored before the exact control run. The fixture adds one registered coverage assertion to each of the Phase 6 and Phase 7 gates.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004447-baseline.log` — fixture baseline, exact output.
- `joint-004447-mutation.log` — forced-zero point mutation, `stdout_delta=2`.
- `joint-004447-restored.log` — restored exact control.

The oracle Physics DLL is SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL is SHA-256 `32598ff36039f32015bab544e3f96bad58ca7d0a26e9dfdff93b781ccda03566`; the restored candidate Physics DLL is SHA-256 `09361e4e51775abcbfb193c029a1da5aed9dafc8efc23f5c21a6e6ed1e18b370`.
