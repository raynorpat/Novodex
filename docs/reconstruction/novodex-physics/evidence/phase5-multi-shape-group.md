# Phase 5 two-box actor group — 2026-09-24

The opt-in `NX_PHYSICS_PROBE_MULTI=1` branch of
`NxPhysicsActorLifecycleTests` creates a dynamic actor with two box shapes
after the registered single-shape create/release cases. It uses the pinned
Physics/Foundation DLL pair and the guarded SDK allocator. The ordinary
staged differential leaves this branch disabled until the whole two-shape
path is reconstructed.

The oracle allocates an outer body (0x50), public actor (0x18), group
(0x110), first shape (0x228), first helper (0x1c), two 0x8 arrays, second
shape (0x228), second helper (0x1c), dynamic record (0x260), then 0xc0 and
0x20 blocks. Creation also frees older 0x60 and 0x10 blocks as those later
allocations grow. The group is body+0x10. Its +0xe0 array has two 0x228 shape
pointers and its +0xf0 array has two 0x1c helper pointers, each with count
and capacity two. Group+4 points to the 0x50 outer body, whose +4 points
to the 0x710 internal Scene. Scene+0x48 points to a 0xa8 auxiliary manager.
The candidate now matches these links. Its previous constructor wrote that
manager pointer at byte 0x12 instead of dword index 0x12 (byte 0x48). The
analogous collector pointer at dword index 0x1b3 (byte 0x6cc) was corrected
at the same time. No public headers changed.

The candidate now builds and frees that group/child graph in the measured
order, including the two 0x8 arrays. Both DLLs report a 0x110 group and
`2/2` for each array. This is a partial reconstruction, not a closed
multi-shape contract: the candidate creates ten allocations where the
oracle creates twelve. The oracle's later 0xc0 and 0x20 allocations are
absent. During release the oracle allocates another 0x18 block and frees
eleven blocks in order
`18.260.1c.228.1c.228.8.8.8.110.50`; the candidate allocates none and
frees ten in order `18.260.1c.228.1c.228.8.8.110.50`.

Allocator stack capture tied the oracle's 0xc0 and 0x20 allocation returns
to RVAs `0x0005c584` and `0x0005c350`, inside the shape-registration paths.
The extra 0x8 free comes from an older array through the group's destructor
at `0x00022d00`, which calls the shared-array growth path at `0x00026c90`.
That path allocates the 0x18 replacement on release. These calls, their
array owner, and the broader shape/vtable behavior are the next closure
packet. The allocator instrumentation used to identify call sites was
removed from the committed probe; the opt-in output records the stable
sizes, counts, and release order.
