# Phase 6 closure: `RevoluteJoint::getFlags` (`phys_fn_004336`)

`phys_fn_004336` is `RevoluteJoint::getFlags` in `Physics/src/core/RevoluteJoint.cpp`. The existing revolute cases in `NxPhysicsJointStagedPairTests` now print the returned flags through the public `NxRevoluteJoint` interface, and the Phase 6 target registry requires the index-0 value.

Built the Win32 Release `NxPhysics` and `NxPhysicsJointStagedPairTests` targets with CMake. The pinned-oracle staged-pair differential passed before mutation. In a temporary source mutation, changed only `return mRevoluteFlags` to `return mRevoluteFlags + 1`. The test caught the wrong result (`flags=00000001` instead of `flags=00000000`); oracle and candidate both exited 0, `stdout_delta=28`, and stderr matched exactly. Mutant candidate DLL SHA-256: `6c9f6a9188a97a3410ff8e663f6fb46df9ebab04cb148eb8a5b34db126263d15`.

Restored the source, performed a clean Release rebuild, and reran the staged-pair differential. Both processes exited 0, the transcript was exact (`stdout_delta=0`), and stderr matched. Restored candidate DLL SHA-256: `69c471776465bc8dad7fa7ed88df9c44bb0773350a55658f4b0e4bc60c4aaf13`. Full build and differential transcripts are retained in the ignored `build/phase6-004336-*` logs.
