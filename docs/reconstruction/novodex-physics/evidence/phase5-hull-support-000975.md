# Phase 5 box-hull support bounds — phys_fn_000975

`NxPhysicsObjectLayoutTests` drives the support-bounds row using the eight-vertex fixture and compares the candidate minimum and maximum words against the oracle (`c19c0000`, `4eada5a5`).

Mutation: reversed `BoxHullFacade::supportBounds` minimum comparison from `<` to `>`. The test reported `hull support candidate ok=0 min_bits=7f7fffff max_bits=4eada5a5` and `layout candidate mismatches=1` (exit 1). After restoring the comparison and rebuilding, it reported `hull support candidate ok=1 min_bits=c19c0000 max_bits=4eada5a5` and `layout candidate mismatches=0` (exit 0).

The existing Phase 5 oracle-differential coverage registry pins this candidate transcript. This closes the measured support-bounds behavior for the registered fixture.
