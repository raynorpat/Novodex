# Phase 5 public shape combined dimensions

Actor-created box and capsule public shape handles now implement their
combined dimension setters. `NxBoxShape::setDimensions` copies the three
half extents to internal offsets `+0xe4` through `+0xec`.
`NxCapsuleShape::setDimensions` stores radius at `+0xe0` and half the
requested full height at `+0xe4`. Each call applies one owner update and
marks geometry dirty with `0x20`.

The dynamic setter drive changes a box to half extents `(2, 3, 4)` and
a capsule to radius `1`, full height `3`. It compares public getters,
internal fields, update flags, pruner generation deltas, and the internal
local AABB against the pinned DLL. All four new output lines match the
oracle; the Phase 5 coverage floor rises from 772 to 776. Invalid input,
scene-lock paths, and other shape operations remain open.
