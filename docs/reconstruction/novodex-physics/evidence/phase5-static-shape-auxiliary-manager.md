# Static-shape auxiliary registration (intermediate)

The pinned Win32 oracle is `NxPhysics.dll` with SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
`NxPhysicsActorLifecycleTests` creates a static box, several dynamic boxes, a
two-box group, then releases and reuses actors through staged oracle and candidate
DLL pairs. The test prints allocation sizes and normalized auxiliary-table state;
the Phase 5 registry requires 37 new lines, raising its floor from 289 to 326.

The first static actor creates five 256-entry arrays in the Scene's 0xa8-byte
auxiliary manager. The arrays begin at offsets `0`, `0x10`, `0x20`, `0x30`, and
`0x90`; the last holds internal shape pointers. The first three use a temporary
`0x800`-byte staging buffer copied into a retained `0x400`-byte block. The
first eleven creation allocations now match the oracle in order:

```
50.18.228.800.400.800.400.800.400.400.400
```

The three staging buffers are freed in the same `800.800.800` sequence. Each
internal shape, including the group object, occupies a slot indexed by its
Scene shape ID. The active list appends slot IDs. Removing a shape swaps the
last active ID into its position, clears its pointer and flags, and sets its
inverse index to `d00beed0`. Shape-ID recycling determines the next reused
slot. The checked active counts progress `1 → 2 → 5 → 2 → 1 → 2`; all sampled
flags, active indices, inverse indices, and pointer-presence words match the
oracle at these points. A temporary mutation that wrote zero into an appended
active slot caused the staged differential to fail; the mutation was reverted.

This is **not** full first-static-actor initialization. The oracle makes 24
allocations in that drive, while the candidate makes 13. After the public
shape handle, the oracle allocates a `0x90` object linked at Scene+`0x640` and
shape+`0xc4`, then several smaller objects and Scene arrays. Those allocations
and their behavior remain to be reconstructed. The Phase 5 gate still reports
the explicit final-vtable missing marker. Public box `setGroup`, `setMaterial`,
and name methods remain outside this packet, as do capacity growth and other
shape classes. No public header was changed.
