# Fluid static-collision bucket hash: `phys_fn_003526`

`phys_fn_003526` at RVA `0x86ad0` hashes packed signed 16-bit XYZ cell coordinates as `x + 101*y + 7919*z`. It casts the sum to unsigned before taking modulo the bucket count, then increments the third dword of the selected 12-byte row. It returns the input cell count; an empty range leaves the bucket array untouched.

`Physics/src/FluidCollision.cpp` reconstructs this helper as `nxFluidCollisionCountBuckets`. The registered `NxPhysicsFluidEmitterAbiTests` fixture directly calls the pinned oracle RVA and the candidate function resolved from the rebuilt DLL's map. Seven coordinate triples include negative X, Y, and Z values. All bucket counts match the oracle, the other two words in every row remain unchanged, the return is 7, and both x86 stacks balance. The zero-cell case returns 0 and preserves both bucket rows.

The mutation changed each selected count increment to `+= 2`. The populated differential detects `counts=0` and `mismatches=1`; the restored DLL returns zero mismatches. The raw control and mutation runs, including the rebuild, are retained in [raw/phase7-fluid-collision-hash-count-003526](raw/phase7-fluid-collision-hash-count-003526).

Fresh Phase 5 and Phase 7 gates pass at 2,645/2,645 and 1,672/1,672 registered coverage assertions. The full reconstruction-tool suite passes 825/825 tests, and inventory validation still accounts for 6,338 functions and 5,138 data objects with zero unexplained executable bytes. The restored candidate DLL SHA-256 is `53d0f1480ca1a72b2e75b3f59e63f98e9992f09ed8859f036927524708cdf3a8`; the pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes the hash/count leaf used by `phys_fn_003528` and the dirty static-collision rebuild `phys_fn_003505`. Those caller rows remain open.
