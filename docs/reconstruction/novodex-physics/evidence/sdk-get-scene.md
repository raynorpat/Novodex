# `NxPhysicsSDK::getScene` wrapper lookup

`NpPhysicsSDK::getScene` was a placeholder even though the internal SDK scene
array, scene wrapper pointer, and both creation and release paths were already
reconstructed. The oracle row `phys_fn_000240` forwards the requested index to
`PhysicsSDK::getScene` (`phys_fn_000450`) and then returns the public wrapper
stored in the internal scene at `+0x6cc`. The oracle does not null-check the
internal result, so an out-of-range lookup faults; this fixture intentionally
tests valid indices only.

The paired `NxPhysicsSDKTests` case creates two scenes, checks that indices 0
and 1 return their public wrappers, releases index 0, checks that the moved
survivor is now at index 0, then releases it. With `getScene` mutated back to
return null, the candidate reports `first_lookup=0 second_lookup=0` and
`survivor_lookup=0`; the differential is red (`stdout_delta=4`, oracle exit 0,
candidate exit 1). Reading `NxSceneInternal::publicScene()` after the internal
SDK lookup makes the candidate transcript identical (`stdout_delta=0`, exact
stderr, both exits 0).

This closes the valid-index wrapper lookup slice. Out-of-range crash behavior
is supported by the disassembly and deliberately not exercised in-process.
After a fresh configure exposed the static-proof target linking both
`NarrowPhase.cpp` and test stubs for the same overlap functions, CMake now keeps
those stubs out of that target and includes the topology unit required by its
triangle-mesh dependencies. The complete Phase 2 gate passes, including the
internal static proof and all three differentials. The Viewer suite also passes
48/48 selected tests after this change: all 39 available scenes are included,
with 43 tests passing and five existing signature-verified asset skips. Public
headers remain unchanged.
