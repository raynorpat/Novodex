# Phase 7 fluid-emitter global-to-local pose conversion: `phys_fn_003611`

IDA identifies RVA `0x89410` as the shared operation that converts a requested global `NxMat34` into the emitter's local frame. An unattached emitter copies the 12 pose words directly. An attached emitter uses either the actor matrix at `+0x20` or its dynamic body's quaternion and translation, applies the inverse actor rotation to the global matrix and translation, then refreshes the cached world pose through `EmitterSetGlobalPose`.

`NxPhysicsFluidEmitterAbiTests` directly compares the pinned oracle entry and candidate map-resolved `nxNpFluidEmitterSetGlobalPoseInternal`. The three cases cover an unattached emitter, a matrix-backed frame actor, and a quaternion-backed dynamic-body actor. Each comparison checks the 12 local words, the 12 cached world-pose words, the callback tuple and pose, the return value, and x86 stack balance.

Replacing only the candidate's local X translation calculation with the untransformed global delta was rejected in both attached cases (`oracle_exit=0`, `candidate_exit=1`, `mismatches=2`). Restoring the transform produces an exact staged differential (`oracle_exit=0`, `candidate_exit=0`, `callback_checks=3`, `stack_checks=3`, `mismatches=0`).

This closes the shared conversion helper. Public global-position and global-orientation setter wrappers and broader emitter simulation behavior remain separate reconstruction rows.

The implementation lives in `Physics/src/ZzFluidEmitterPose.cpp`. Keeping the row after `PhysicsSDK.cpp` in CMake's source order preserves the baseline `NxCreatePhysicsSDK` RVA (`0x00055e50`); placing the helper before the factory shifted it by the helper's `0x610` bytes and caused the Phase 5 allocator fixture to return into the middle of an instruction. With the final order, the direct allocator test and the complete Phase 5 gate both pass. The restored pose differential remains exact at three cases.
