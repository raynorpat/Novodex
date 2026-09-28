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

NpActor.cpp completion Task 4 (`units/npactor-contract.md`, "## Task 4: shape
add/remove") replaced the partial candidate path with Actor.cpp's 000036 and
000024 and their chain, and extended this target past the one-to-two
transition: every family the harness builds on static and dynamic actors,
appends that grow the group arrays, releases at every position down to the
group teardown, single-root installs and releases, the Actor.cpp E1 reports,
the group's own poses, and a Scene whose first shape comes from createShape.
The removed child of a group is still only unlinked (001028), as measured
above; a released single root and an emptied group are deleted. 136 more
oracle lines are registered; the nine lines above are unchanged.

NpActor.cpp completion Task 5 (`units/npactor-contract.md`, "## Task 5: mass
from shapes and setDynamic") adds two blocks to this target on fresh Scenes:
updateMassFromShapes (000164 -> Actor.cpp 000008 -> each family's slot 4)
with a density and with a total mass over every family, groups, triggers and
planes, with its argument errors; the creation path's own mass pass (000026)
and its failures; and setDynamic (000122) on static, dynamic, shapeless and
jointed actors, each actor then used and released through the rewritten
releaseActor (000628 -> 000030). 110 more oracle lines are registered; every
line above is unchanged.
