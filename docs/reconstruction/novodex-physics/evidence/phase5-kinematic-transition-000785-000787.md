# Kinematic body-flag transition falsification — 2026-10-07

`phys_fn_000785` and its disable-arm tail `phys_fn_000787` implement the dynamic record transition used by `raiseBodyFlag(NX_BF_KINEMATIC)` and `clearBodyFlag(NX_BF_KINEMATIC)`. The registered `NxPhysicsActorBodyFlagTests` fixture observes inverse mass/inertia, record and manager flags, allocator changes, dirty-list state, and island-root refresh. Its clean pinned-oracle differential passes exactly (`stdout_delta=0`, `stderr_exact=True`; `build/kinematic-transition-restored-differential.log`).

Two temporary mutations were rebuilt independently. Changing the enable arm's first inverse-mass write from `0.0f` to `0.5f` is caught with `stdout_delta=2`; changing the disable arm's inverse-mass reconstruction to `0.5f` is also caught with `stdout_delta=2`. Both oracle and mutated candidate processes exit zero and stderr matches exactly (`build/kinematic-enable-mutation-full.log`, `build/kinematic-disable-mutation-full.log`). The source was restored byte-for-byte and rebuilt; the baseline returned to an exact differential.

This closes the two transition rows' mutation-sensitivity requirement. Phase 5 remains pending, and the full DLL reconstruction remains open. Public Physics headers are unchanged.
