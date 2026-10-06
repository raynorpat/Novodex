# Disabled fluid release and empty-manager teardown

The pinned SDK cannot create a fluid, but `releaseFluid` still has an observable
unsupported path when the lazy manager exists. `NpScene::releaseFluid` takes
the scene write lock and forwards the NpFluid internal pointer at `+0x14` to
`Scene::releaseFluid`. The Scene path calls the manager, whose disabled-backend
branch reports `NXE_DB_WARNING` (206), line 183,
`NxScene::releaseFluid(): Feature not available!`. The empty manager is then
destroyed and Scene `+0x61c` is cleared.

The regression first calls `createFluid` to allocate the manager, then passes a
pointer-shaped local marker as a non-member fluid. Since the oracle manager's
fluid list is empty, it compares no list entries and does not dereference the
marker. This probes the empty-manager teardown branch only; it does not stand in
for a valid fluid release. The initial candidate RED omitted the warning and
left the manager installed. After reconstruction, oracle and candidate both
exit 0 with identical output (`stdout_delta=0`, `stderr_exact=True`) in
`build/FluidGate/fluid-release-green.log` (2026-10-05).

This evidence covers the wrapper, disabled warning, and empty manager cleanup.
Removing a live fluid, its virtual destructor dispatch, nonempty parallel arrays,
and callback reentry remain open because the pinned release cannot create a
valid fluid through its public API.
