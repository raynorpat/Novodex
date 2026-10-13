# Contact-pair constraint generation `phys_fn_000897` / `phys_fn_000899`

The two oracle ranges are emitted into one candidate method,
`NxActorPair::row000897`. It refreshes patch state, reads contact-stream
headers and points, selects restitution and friction patches, builds spring or
penalty constraints, chooses anchors, and dispatches the friction solver.
The registered simulation target covers sliding contact, ordinary contact,
spring contact, sphere pairs, and mesh contact.

For the mutation check, the shared candidate method body was replaced with an
immediate return and `NxPhysics` was rebuilt. Both processes exited 0, but the
registered staged transcript changed by 6,644 bytes and the differential
rejected the mutant (`stdout_delta=6644`, `stderr_exact=True`). Restoring the
body and rebuilding returned the differential to exact output (`stdout_delta=0`,
`stderr_exact=True`). The mutant and restored candidate Physics DLL hashes
were `3440c00f3581681211a1c45dfc774bc74a9c9e0c8e81d2f77478f8e6fb246df2` and
`99dbd33998662ed46456cd2c3f47054db294fdab4858acb6467206b945fa8108`;
the pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This falsifies the combined candidate implementation for the registered
contact/friction contract. Individual rare stream flags, material-feature
combinations, and every x87 precision branch remain part of the broader
Phase 8 audit.

Raw mutant and restored logs and the source mutation are in
`evidence/raw/phase7-contact-pair-constraints-000897-000899-20261013/`.
