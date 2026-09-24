# Phase 5 actor ownership and release — 2026-09-24

`NxPhysicsActorLifecycleTests` drove four box actors and released the fourth
through staged oracle and rebuilt DLL pairs. The guarded allocator records
allocation sizes, pointer identities, and successful free sizes. This corrected
an earlier interpretation of the actor graph: the **public actor wrapper is
0x18 bytes**, and its `+0x14` points to a separate 0x50-byte outer body. The
dynamic record at body+8 is 0x260 bytes; record+0x19c points **back to the same
outer body**, rather than to a third 0x50-byte pose allocation. The outer body
contains the matrix at +0x20 and translation at +0x44. The wrapper's +0x0c
and +0x10 aliases match the public scene's +0x0c/+0x10 lock links, shared by
all actors. Each +0x10 link is 4 bytes and points to a 0x20-byte lock block.

The outer body's first word points back to the public actor. Its +0x10 points
to a 0x228-byte shape object, and shape+0x9c points to a 0x1c-byte helper.
The tested dynamic record array at internal Scene+0x56c has two entries and
capacity two before the fourth actor, then three entries and capacity six.
Its 0x18-byte grown array is retained when that actor is released.

The fourth creation's oracle allocation sequence is
`50.18.228.1c.260.18` hexadecimal: outer body, public wrapper, shape,
shape helper, dynamic record, then grown Scene array. Release frees five
blocks in the observed order `18.260.1c.228.50`, leaving the Scene array.
The oracle's `NpScene::releaseActor` wrapper at RVA `0x0000c500` passes
actor+0x14 to the internal Scene release at `0x000123d0`. That routine finds
the public actor via body+0, swaps the last actor pointer over the removed
entry in the +0x55c array, runs the body destructor at `0x00001c40`, then
frees the body. The rebuilt DLL now gives the same staged transcript: actor
count four to three, released actor absent from the list, six allocation sizes
and roles, five frees and their sizes, and no output differences.

The earlier candidate had a 0x50-byte public actor, an unnecessary separate
0x50-byte pose, direct 0x20-byte scene lock pointers, no public actor count,
and a no-op release. This packet corrects those observed links. It also
removes an out-of-bounds write in an unused actor-construction helper, which
had retained the obsolete 0x50-byte layout assumption.

The shape's other fields, exact lock protocol, callbacks, error paths, slot
reuse, static/multi-shape teardown, and physical state remain open. The test
establishes this box-actor lifecycle, not full Phase 5 closure. Its 24 new
registered lines raise the Phase 5 assertion floor to 176; the unrelated
final-vtable placeholder still keeps the phase gate red.

A temporary mutation that left the Scene actor-array end unchanged after the
swap made the staged differential fail with `stdout_delta=4`. Restoring the
write returned the clean comparison to zero, so the public count and removed
actor checks exercise the release implementation rather than only actor
creation.
