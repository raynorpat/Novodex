# Scene allocation failure differential — 2026-10-14

`NxPhysicsSceneAllocatorTests` now forces the Foundation allocator to fail the `0x710` byte `NxSceneInternal` allocation during `NxPhysicsSDK::createScene`. It verifies that the public result is null, `sdk->getNbScenes()` remains zero, and exactly one injected allocation failure occurred. The test records the failed phase through the allocator transcript and is registered as a required marker in both Phase 5 and Phase 7.

The focused oracle/candidate differential passed: both processes exited zero, stdout matched exactly, and stderr matched exactly. In the failure phase both allocators recorded zero allocations/frees and zero cross-frees. A mutation that bypasses the null-allocation return guard was rejected: oracle exit 0, candidate access violation (`0xC0000005`), with a 64-byte stdout delta.

The full Phase 5 gate passed at 2,722/2,722 coverage assertions. The full Phase 7 gate passed at 1,793/1,793. Both ran the registered staged-pair suites against the pinned `D:\FlamingEnt__\Unreal_3\Binaries` oracle. A clean Release CMake build of `NxPhysics.dll` and the Phase 5/7 test targets succeeded with the repository's existing MSVC warnings.

This adds failure-path coverage for Scene construction; it does not close the full `phys_fn_000476` row or complete Scene reconstruction. Public headers are unchanged. Raw focused differential and mutant transcripts are in [`raw/phase5-scene-allocation-failure-20261014`](raw/phase5-scene-allocation-failure-20261014/).
