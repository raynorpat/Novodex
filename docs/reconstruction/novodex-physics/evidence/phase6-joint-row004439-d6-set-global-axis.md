# Phase 6 closure: D6 global-axis setter (`phys_fn_004439`)

`phys_fn_004439` at RVA `0x000b06a0` is `NpD6Joint::setGlobalAxis`. The existing D6 descriptor fixture did not call this public joint setter, so the D6 staged-pair case now supplies a replacement axis after joint creation and records the world-axis readback.

The restored fixture is exact against the pinned oracle: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily forwarding `(0, 1, 0)` instead of the supplied axis changes the readback from `3e9b28d0.bf4ee116.3f014cae` to `00000000.3f800000.00000000`. The registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=6`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was confined to the isolated worktree and restored before the exact control run. The fixture adds one registered coverage line per staged pair, raising the Phase 6 and Phase 7 coverage floors by two each.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004439-expanded-baseline.log` — fixture baseline, exact output.
- `joint-004439-mutation.log` — forced-axis mutation, `stdout_delta=6`.
- `joint-004439-restored.log` — restored exact control.

The oracle Physics DLL is SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL is SHA-256 `3eb679dca29e17df6ef7591e26fe1757fc8d3617c97206963f7687f84bb2687b`; the restored candidate Physics DLL is SHA-256 `6d7e6fa1702770a86d97f46a70eefeaef3762de3f359beb6cdffb1569c564497`.
