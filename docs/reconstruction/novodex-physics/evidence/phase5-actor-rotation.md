# Phase 5 actor rotation and public orientation — 2026-09-24

`NxPhysicsActorLifecycleTests` now creates identity static, half-turn dynamic,
and quarter-turn dynamic box actors through staged pinned-oracle and rebuilt
Physics/Foundation DLL pairs. It records the dynamic record's quaternion words,
the nine words returned by `getGlobalOrientationVal()`, and the four words
returned by `getGlobalOrientationQuatVal()`.

The initial staged comparison failed on the half-turn quaternion and returned
uninitialized words for the candidate's orientation matrix. The oracle's
dynamic record uses +0x5c..+0x68 for `(x,y,z,w)`. The 180-degree Z case is
`00000000.00000000.3f800000.00000000`. Converting the descriptor matrix with
the existing Foundation `NxQuat(NxMat33)` convention reproduced that result
and the 90-degree Z quaternion
`00000000.00000000.3f3504f3.3f3504f3`.

Actor vtable slot 7 at RVA `0x000046c0` reads the dynamic record's quaternion
and composes a matrix, or copies the static outer body's matrix at +0x20.
Slot 8 at RVA `0x00002f30` copies a dynamic quaternion or converts that static
matrix to one. The static holder now receives the descriptor matrix. The
quarter-turn matrix established that float-width intermediate products are
insufficient: the oracle's diagonal word is `331302ae`, while the float
version returned `33800000`. Double-width intermediates followed by float
stores match the pinned oracle's full nine-word result in this case.

The actor staged differential is exact after the fixes. A temporary mutation
that zeroed the dynamic matrix's first element failed it with `stdout_delta=4`.
A separate mutation that set the public quaternion's X to 1 likewise failed
with `stdout_delta=4`; both were restored and the clean build retested. Ten
new transcript lines are registered, raising the Phase 5 floor to 148.

These cases establish the tested rotations and ABI paths, not every numeric
context, nonorthonormal descriptor, or mutation method. Phase 5 still fails on
the independent final-vtable placeholder.
