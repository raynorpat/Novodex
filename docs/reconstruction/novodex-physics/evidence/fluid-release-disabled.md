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
The isolated full Phase 7 gate then passed all 11 targets with zero
differential deltas and evaluated the updated floor exactly at 1,320/1,320
(`build/FluidGate/phase7-fluid-manager-vtable-gate.log`).

The regression first calls `createFluid` to allocate the manager, then passes a
pointer-shaped local marker as a non-member fluid. Since the oracle manager's
fluid list is empty, it compares no list entries and does not dereference the
marker. This probes the empty-manager teardown branch only; it does not stand in
for a valid fluid release. The initial candidate RED omitted the warning and
left the manager installed. After reconstruction, oracle and candidate both
exit 0 with identical output (`stdout_delta=0`, `stderr_exact=True`) in
`build/FluidGate/fluid-release-green.log` (2026-10-05).

A second focused fixture seeds both manager arrays with two internal fluid
pointers, then releases the first
through `NxScene::releaseFluid`. The oracle swap-removes the target from both
arrays and calls that object's vtable slot 0 with scalar deleting flag 1. The
new candidate fixture first went RED with matching array contents but no
destructor call (`calls=0`, `flags=0`); `nxFluidManagerReleaseDisabledFluid`
now dispatches that slot after the removals. The exact differential is green
(`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-array-target-final-differential.log`),
including a check that the dispatched object is the one removed from the arrays.
The full Phase 7 gate passes all 11 targets at 1,321/1,321 assertions
(`build/FluidGate/phase7-fluid-array-target-final.log`).

The same disabled manager remains attached while the fixture creates and then
releases a public sphere actor. The oracle calls `phys_fn_003637` and
`phys_fn_003635`, reporting `NXE_DB_WARNING` at FluidManager.cpp lines 263 and
250; before the change both candidate calls were silent. The candidate now
matches both tuples and the complete simulation transcript exactly
(`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-actor-notify-green.log`).
The manager arrays are empty in this pinned backend, so iteration into live
`NpFluid` notifications remains open.

This evidence covers disabled manager state, its first virtual deleting
destructor on empty arrays, the wrapper warning, empty manager cleanup, and
swap-removal/destructor dispatch with test-seeded array state. It does not
exercise a real `NpFluid` object or its destructor body. Extension-backed
manager initialization/destruction, registry-controlled fluid flags, live
emitter ownership, and callback reentry remain open because the pinned release
cannot create a valid fluid through its public API.

Mutation sensitivity was also probed against the registered Phase 7 simulation
differential. Replacing the `Scene::releaseFluid` forwarding call in
`NpScene::releaseFluid` with a no-op kept the candidate process alive but changed
four normalized transcript lines; the runner reported `stdout_delta=4` and
rejected the candidate (`build/scene-release-fluid-mutation.log`). This was a
temporary mutation in the active checkout, restored and rebuilt immediately
afterward. It is supplemental evidence only and is not used to close the Phase 7
row, whose full live-fluid behavior remains unavailable in the pinned SDK.
