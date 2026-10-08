# Populated disabled FluidManager teardown

`phys_fn_003641` (`0x00089e00`) is the manager's destructor. The pinned IDA
decompilation first installs its base vtable, then walks the primary fluid
array and invokes slot 0 with deleting flag 1 for each non-null entry. It next
checks the backend-available byte at `+0x2b`, performs extension cleanup, and
frees and zeros the secondary and primary array headers. The shipped build has
the backend flag clear.

The previous candidate freed only the two array allocations. A focused
`NxPhysicsSimulationTests` fixture seeds two fake internal fluids with a
recording deleting destructor and allocates both array blocks through the
Foundation allocator. Against the pinned DLL it observed two deleting
dispatches, the second fixture object as the final target, and all six array
header words cleared. The test failed against the previous candidate with zero
dispatches and uncleared headers. `nxFluidManagerDestroyArrays` now reproduces
the pinned disabled-backend behavior, and the focused differential passes with
`stdout_delta=0` and `stderr_exact=True` in
`build/FluidGate/fluid-manager-populated-destructor-focused-repeat.log`. The
full Phase 7 gate passes with the new coverage line and evaluates 1,386
assertions against a floor of 1,385 in
`build/FluidGate/fluid-manager-populated-destructor-phase7.log`.

This closes the populated-array teardown behavior available in the pinned
backend. It does not claim extension-backed callback behavior: the pinned
constructor leaves that backend unavailable, so its global callback table and
module lifetime cannot be observed through this scene fixture.

No public `Physics/include` header changed.

After rebuilding the candidate, the approved Release Viewer selection also
passed: all 48 CTest selections covered the 39 available scenes, with 43
passing and the five established signature-verified pinned-oracle asset cases
skipped. The Viewer physics-step and contact checks passed as well
(`build/FluidGate/viewer-all-scenes-fluid-manager.log`).
