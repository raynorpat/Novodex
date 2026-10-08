# Box sweep mutation proof (`phys_fn_000951`)

`phys_fn_000951` is `BoxShape::nxBoxSweep`, BOX table slot 7 at RVA `0x00020b20`. The registered `NxPhysicsShapeVtableTests` oracle differential calls the oracle row and candidate implementation over 84 combinations of three rotations, two translations, two extent sets, and seven directions, including near-parallel slab directions and zero motion.

The clean target reports `shape vtable boxsweep oracle_digest=2c5d5c09 cases=84 failures=0` and exits 0. A source mutation that returns false after a successful `NxSegmentSlabs` hit was rebuilt into that target; the fixture reported 60 sweep mismatches and the target exited 1. Restoring the true return and rebuilding returned to the 84-case clean result with zero failures.

This closes mutation sensitivity for the measured box-sweep contract. It does not claim every degeneracy of the slab kernel or every collision configuration is exhausted. Public headers and production behavior are unchanged.
