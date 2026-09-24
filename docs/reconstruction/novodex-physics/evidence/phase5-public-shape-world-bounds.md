# Phase 5 public shape world bounds

The common public `NxShape::getWorldBounds` dispatches to internal shape
slot 9. Differential probes cover an actor-created rotated box, default
sphere, capsule and plane, plus box and capsule bounds after dimension
changes. All six output rows now match the pinned DLL bit for bit.

The rotated box probe exposed an incorrect dimension-axis mapping in the
candidate's internal `BoxShape::nxBoxWorldAABB`: Y and Z dimensions were
swapped in each row of the absolute rotation. The corrected row order
matches both the original and resized box. The Phase 5 coverage floor rises
from 776 to 782. Other common public shape methods and full vtable-family
closure remain open.
