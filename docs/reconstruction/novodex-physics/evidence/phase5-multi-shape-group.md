# Phase 5 two-box actor group — 2026-09-24

`NxPhysicsActorLifecycleTests` now creates and releases a dynamic actor with
two box shapes through both staged DLL pairs. Its 29 new transcript lines
are registered in the Phase 5 gate; the assertion floor is 209. The ordinary
staged differential has no output differences for this actor. This proves
the measured lifecycle path, not the complete shape implementation or final
vtable family.

The oracle and candidate allocate these blocks on creation, in hex size
order: `50.18.110.228.1c.8.8.228.1c.260.c0.20`. The 0x50 outer body owns
the 0x110 group at +0x10. Group+4 points back to the body; body+4 points to
the 0x710 internal Scene, whose +0x48 points to the 0xa8 auxiliary manager.
The group contains two parallel `2/2` arrays: shape pointers at +0xe0
(each 0x228) and helper pointers at +0xf0 (each 0x1c). The candidate follows
the oracle's group/child allocation order.

The later 0xc0 and 0x20 allocations are broadphase table growth, not shape
objects. Allocator ownership tracing located the table's 0x3c-byte owner at
internal Scene+0x648, with buffers at owner+0x14 and +0x18. The owner is
absent after Scene creation, appears for the first dynamic actor with one
entry and capacity four, and has two entries after the earlier actor
releases. Registering the two children and group grows it from `2/4` to
`5/8`, replacing 0x60 and 0x10 buffers with 0xc0 and 0x20. The five
reference slots point to `child0+0xa4`, `child1+0xa4`, the two surviving
single shapes' `+0xa4` fields, and `group+0xa4` in that order. Release
returns the table to `2/8` without shrinking its buffers. The candidate
matches those counts, sizes, free sizes, and pointer roles.

Internal Scene+0x6e4 is the next shape ID. A LIFO ID array at +0x6e8
receives ID 3 when the earlier quarter-turn actor is released and ID 0
when the static actor is released. Group creation takes ID 0 first, the
first child takes ID 3, and the second child receives new ID 4. The array
is then empty with capacity two. Releasing the group returns child IDs 3
and 4, then group ID 0. Its third append grows the array to capacity six,
allocating 0x18 and freeing the old 0x8 buffer. The candidate matches the
oracle's final `3/6` array and values `3.4.0`.

The two DLLs release the actor in the same hex free-size order:
`18.260.1c.228.1c.228.8.8.8.110.50`. The oracle calls at RVAs
`0x000effc0`/`0x000f00c0` expose broadphase table growth, and the group's
destructor at `0x00022d00` reaches the recycled-ID array growth path at
`0x00026c90`. Temporary guarded-allocation owner tracing located these
buffers and was removed after the compact staged-pair checks were added.
No public Physics headers changed.

The first dynamic actor's broader Scene initialization is still incomplete:
an opt-in `NX_PHYSICS_PROBE_DYNAMIC_INIT=1` observation reports 17 oracle
allocations against nine candidate allocations in this harness. This is a
separate open dependency. The Phase 5 gate also remains red on the explicit
final-vtable placeholder in `PhysicsObjectLayoutTests`.
