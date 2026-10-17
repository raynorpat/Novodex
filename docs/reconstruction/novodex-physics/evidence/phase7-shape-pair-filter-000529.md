# Phase 7 shape-pair filter, `phys_fn_000529`

`NxFilterShapePair` at oracle RVA `0x00010570` rejects a shape pair when either shape carries filter flag `0x10`, when the two collision-group masks do not allow the pair, or when the scene's owner-pair record disables collision. The candidate implementation lives in `Physics/src/NarrowPhase.cpp`; the current CMake Release map resolves `?NxFilterShapePair@@YA_NPBIPBXPBUNxCollisionShape@@2@Z` at VA `0x10031670` in `NarrowPhase.obj`.

The row is covered by seven direct candidate-versus-oracle calls in `NxPhysicsCollisionTests`. The test target is now registered in the Phase 7 oracle-differential set, where this row is owned. Its existing required transcript line, `matrix contact_filter checks=7 wrong=0`, is part of that phase's enforced coverage floor.

For mutation testing, the candidate implementation was replaced with an unconditional `return true`, and `NxPhysicsCollisionTests` was rebuilt. The oracle disagreed in three of the seven cases: disabled group-mask cases 3 and 4, and owner-pair case 1. The mutant emitted `matrix contact_filter checks=7 wrong=3`, so the registered exact coverage assertion is absent and Phase 7 rejects the transcript. The mutation was then removed byte-for-byte and both `NxPhysics` and `NxPhysicsCollisionTests` rebuilt. The restored control emitted `matrix contact_filter checks=7 wrong=0` and the full collision harness completed with `collision matrix_wrong=0 index_wrong=0 mismatches=0`.

The restored candidate DLL SHA-256 was `957d5e5d98280722fac41ce412c17766b2ff9c3d5a5e45a0fc157536172419d4`. The oracle image is pinned at SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
