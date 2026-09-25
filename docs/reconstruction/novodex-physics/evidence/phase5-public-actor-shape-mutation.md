# Phase 5 public actor shape mutation

The pinned Win32 oracle accepts `NxActor::createShape` on a static actor that
already owns one box. It allocates a 0x228 internal shape, a 0x1c public
handle, a 0x110 group, and two 8-byte child arrays in that order. Both arrays
contain the original child followed by the new child. The original public
handle remains stable, and the actor's shape count changes from one to two.

Calling `releaseShape` on the newly added handle reduces both array lengths
from two to one without an immediate allocation or free. The group remains the
root. Releasing the actor then allocates two 8-byte blocks and frees the public
actor, remaining handle and child, both arrays, group, and body in the measured
order. The removed child is not freed by this actor-release path; its later
lifetime remains to be investigated.

`NxPhysicsActorShapeMutationTests` captures nine oracle-derived lines for this
single-to-group transition. The candidate matches the pinned pair on all nine
lines, and the Phase 5 coverage floor rises from 820 to 829. A separate oracle
probe showed that appending to an existing two-shape group allocates six blocks
and frees four while changing the count to three. The candidate does not yet
implement that expansion, so the registered case deliberately stops at the
measured one-to-two transition. Single-shape removal and dynamic-actor
mutation also remain open.

The Phase 5 gate remains red at the explicit final-vtable marker in
`NxPhysicsObjectLayoutTests`; this packet does not close the shape finals and
actor classes family.
