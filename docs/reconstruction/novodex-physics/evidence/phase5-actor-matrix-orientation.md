# Phase 5 actor matrix orientation

Actor dynamic vtable slot 3 at RVA `0x9110` (`phys_fn_000200`) accepts a
`NxMat33`. A static actor copies all nine input words to body `+0x20`. A
dynamic actor converts the matrix to a quaternion using the public SDK's
`NxMat33::toQuat` branch order, copies it to record `+0x5c` and shadow
`+0x24`, marks dirty bit 2, refreshes mass-frame data, and calls the owned
shape update. The candidate now implements these branches.

The public setter drive checks one nonnegative-trace matrix and three
negative-trace matrices whose largest diagonal is respectively X, Y, and Z.
The matrix cases compare the dirty bit, current/shadow quaternion words and
owned shape rotation words. A static case checks both the body's cached
matrix and its shape world matrix. All six added lines match the pinned
oracle and candidate, raising Phase 5's floor from 740 to 746.

Error/scene/group branches and the final shape/actor vtable family remain
open. The explicit `CANDIDATE-MISSING` marker still keeps Phase 5 red.
