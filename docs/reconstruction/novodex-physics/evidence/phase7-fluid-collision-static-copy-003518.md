# Fluid static-collision array copy: phys_fn_003518

`Physics/src/FluidCollision.cpp` reconstructs the cdecl function beginning at
oracle RVA `0x86590`. The function copies the bucket records, allocates enough
space for the sum of their cell counts, flattens each bucket's referenced
16-byte cell range into output order, copies the 8-byte index records, and
returns the index count. Its discontiguous code rows `phys_fn_003520` and
`phys_fn_003522` are continuations of this same function and are exercised by
the same direct call.

The oracle's callback slots for `CDBucketAttributesArrayCopy`,
`CDCellAttributesArrayCreate`, and `CDIndexArrayCreate` are temporarily bound
to small test allocation adapters. This lets the pinned oracle execute the
reconstructed function without an installed FluidModel extension. The
adapters allocate and initialize the three output array headers; the probe
compares the resulting array state and the candidate uses the SDK allocator.
The oracle reads the third bucket dword from an uninitialized stack slot, so
the differential deliberately excludes that word from comparison and the
candidate retains the copied source value.

The registered `NxPhysicsFluidEmitterAbiTests` differential passes three
bucket rows with counts/first-cell offsets `(2,1)`, `(1,0)`, and `(0,2)`. The
output bucket counts and prefixes are `(2,0)`, `(1,2)`, and `(0,3)`; all three
cell records are copied in bucket order, all four 8-byte index records match,
the source records remain intact, and the function returns `4` with balanced
x86 stacks. The probe reports `mismatches=0`.

The mutation control incremented the second word of each copied index record.
It was detected as `indices_match=0` and `mismatches=1`; restoring the copy
returned to zero mismatches. The mutation and control transcripts are in
[`raw/phase7-fluid-collision-static-copy-003518`](raw/phase7-fluid-collision-static-copy-003518).

The restored candidate DLL SHA-256 is
`d3e932b4c7c908aab6e9a7589a7d7764f1c8a177e043f9422aaf953bceb25713`; the
pinned oracle is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
