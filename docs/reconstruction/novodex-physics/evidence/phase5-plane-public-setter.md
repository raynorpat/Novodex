# Phase 5 public plane equation

The scene's raw plane-shape allocation already stored the descriptor normal
and negated distance, but left the tangent basis and axis classification
zero. It now initializes both tangent vectors with the same
`NxNormalToTangents` routine used by the reconstructed `PlaneShape`
constructor and classifies the axis from the normal's absolute words.

The public `NxPlaneShape::setPlane` slot now reaches the existing
`PlaneShape::nxPlaneSetEquation` implementation and marks the shape's
deferred equation update. A public actor probe compares the default basis
and class, then changes the plane to normal `(0,0,1)` and distance `2.5`.
The resulting normal, negative distance, tangent/binormal words, axis class,
owner-update flags and one pruner-generation increment match the pinned
DLL. The two new lines raise Phase 5's floor from 770 to 772. The final
vtable-family marker remains open.
