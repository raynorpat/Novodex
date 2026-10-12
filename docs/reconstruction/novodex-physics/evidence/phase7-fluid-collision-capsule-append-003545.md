# Fluid collision capsule-array appender (`phys_fn_003545`)

`phys_fn_003545` at RVA `0x877a0` appends one 88-byte capsule record to the caller-owned typed array. The private candidate implementation is `nxFluidCollisionAppendCapsule` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA confirms the cdecl `(descriptor, shape)` signature. The helper accepts only shape type 3 at `+0xd0`, copies the two geometry words at `+0xe0` and `+0xe4`, packs the common collision record after them, writes 88 bytes to `array.data + 88 * array.count`, increments the count, and returns the descriptor. Other types return null without touching the descriptor.

`NxPhysicsFluidEmitterAbiTests` directly compares the pinned oracle RVA and candidate DLL for one selected capsule and one ignored non-capsule. The fixture assigns distinct values to both geometry words, checks the first 84 defined record bytes and the untouched tail, and validates expected returns/counts and cdecl stack balance. The final dword is excluded because the oracle packer updates a bit in an otherwise uninitialized local word. Restored output reports `candidate_present=1`, `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation copied `shape+0xe0` into the second geometry word in place of `shape+0xe4`. The differential rejected it (`oracle_contract=0`, `mismatches=1`, process exit 1); mutant candidate SHA-256 `e63298009d750cc0e2d8b4aad7cbebba3a4f6645e11e02e6a1700e2d4c570f8a`. Restoring the source returned the differential to zero mismatches; passing candidate SHA-256 `dc7da7a81ba7fde2782423ba5ca8971a8d4fd04a1f6a45653ca8443434ddba28`. The pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes only the capsule typed-array appender. The plane, box, and mesh appenders, parallel scene collector `phys_fn_003555`, and enclosing dirty static-collision builder `phys_fn_003505` remain open.

Fresh Release Phase 5 and Phase 7 gates pass with the committed candidate: Phase 5 at 2,655/2,655 registered assertions and Phase 7 at 1,682/1,682. Phase 5 also re-runs the five collision-object family secondary and primary destructor probes.
