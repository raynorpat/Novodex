# Disabled fluid manager construction, dispatch, and release

The pinned SDK cannot create a fluid, but `releaseFluid` still has an observable
unsupported path when the lazy manager exists. `NpScene::releaseFluid` takes
the scene write lock and forwards the NpFluid internal pointer at `+0x14` to
`Scene::releaseFluid`. The Scene path calls the manager, whose disabled-backend
branch reports `NXE_DB_WARNING` (206), line 183,
`NxScene::releaseFluid(): Feature not available!`. The empty manager is then
destroyed and Scene `+0x61c` is cleared.

The empty-manager probe now also checks the constructor fields read by the
oracle: the owner Scene, both empty array headers, the initialized flag, and
the unavailable extension/backend flags. A separate scene loads the manager's
first vtable entry and calls its scalar-deleting destructor with flag 1. The
oracle listing identifies that entry as `phys_fn_003647` (`0x89fd0`). The
candidate now installs a callable slot and routes manager teardown through it.
The test first failed because the candidate's placeholder vtable had no
destructor; after reconstruction, candidate and oracle both exit 0 and their
1,025-line normalized simulation transcripts match exactly
(`build/FluidGate/fluid-vtable-candidate-green.log`,
`build/FluidGate/fluid-vtable-oracle-green.log`; pair identity/footer lines
excluded because they necessarily contain different paths and hashes).

The regression first calls `createFluid` to allocate the manager, then passes a
pointer-shaped local marker as a non-member fluid. Since the oracle manager's
fluid list is empty, it compares no list entries and does not dereference the
marker. This probes the empty-manager teardown branch only; it does not stand in
for a valid fluid release. The initial candidate RED omitted the warning and
left the manager installed. After reconstruction, oracle and candidate both
exit 0 with identical output (`stdout_delta=0`, `stderr_exact=True`) in
`build/FluidGate/fluid-release-green.log` (2026-10-05).

This evidence covers disabled manager state, its first virtual deleting
destructor on empty arrays, the wrapper warning, and empty manager cleanup.
Removing a live fluid, nonempty parallel arrays, extension-backed manager
initialization/destruction, registry-controlled fluid flags, and callback
reentry remain open because the pinned release cannot create a valid fluid
through its public API.
