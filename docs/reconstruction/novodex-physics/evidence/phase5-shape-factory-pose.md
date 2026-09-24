# Phase 5 shape-factory pose initialization

The existing `nxShapeFactory` allocated and zeroed its internal shape but did
not initialize the three pose slots. The oracle's shape constructor at RVA
`0x25530` initializes all three to identity, and descriptor loading copies
`NxShapeDesc::localPose` into the third slot at `+0x6c`. The candidate now
copies that descriptor pose and composes a world pose through the owned-shape
update path, then mirrors the result to `+0x3c`.

A dynamic actor creates its record after its shape. During scene registration
the shape pose must be refreshed from that record's quaternion. For a
quarter-turn actor, the matrix-to-quaternion round trip produces nonzero
low-bit residuals even where the descriptor matrix contains zeroes. The
candidate now refreshes single shapes and compound children after record
creation. The public actor test compares the default identity local and world
poses plus a rotated, translated descriptor's local, world, and mirrored
poses. All six new lines match the pinned oracle, raising the Phase 5 floor
from 734 to 740.

This is a partial factory reconstruction. Final shape class allocation,
descriptor branches, and the Phase 5 vtable-family marker remain open.
