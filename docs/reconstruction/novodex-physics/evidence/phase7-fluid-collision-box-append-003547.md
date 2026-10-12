# Fluid collision box-array appender (`phys_fn_003547`)

`phys_fn_003547` at RVA `0x87810` appends one 92-byte box record to the caller-owned typed array. The private candidate implementation is `nxFluidCollisionAppendBox` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA confirms the cdecl `(descriptor, shape)` signature. The helper accepts only shape type 2 at `+0xd0`. The three geometry words begin at `shape+0xe4` and occupy `+0xe4`, `+0xe8`, and `+0xec`; it packs the common collision record after those words, writes 92 bytes to `array.data + 92 * array.count`, increments the count, and returns the descriptor. Non-type-2 shapes return the shape unchanged. The final local dword is undefined because the shared packer ORs a flag into an uninitialized word, so the differential compares the first 88 bytes and verifies the untouched tail.

`NxPhysicsFluidEmitterAbiTests` compares the pinned oracle RVA and candidate DLL for a selected type-2 box and an ignored type-1 shape. Distinct decoy and geometry words expose the `+0xe4` base and each successive word; the test also checks expected returns/counts, stack balance, and bytes beyond the record. The restored control reports `candidate_present=1`, `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

The initial candidate/test assumption used `shape+0xe0`; the oracle diagnostic showed words `+0xe4,+0xe8,+0xec`, matching IDA's `add ecx,0xe4` before three loads. A row-specific mutation then sourced the third word from `+0xe8` instead of `+0xec`. It was rejected (`oracle_contract=0`, `mismatches=1`, exit 1); mutant candidate SHA-256 `8191738d1e02cd90fc078272513a4319037bc588487c6efa1bb803f53f01bfb4`. Restoring the source returned all three appender differentials to zero mismatches. Passing candidate SHA-256 `04e2a65495d126c9e373f7b3bccf491a131c03dde8c7acad316db6a8cf86faca`; pinned oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes only the box typed-array appender. The mesh and plane appenders, router `phys_fn_003553`, scene collector `phys_fn_003555`, and dirty static-collision builder `phys_fn_003505` remain open.

Fresh Release gates passed after merging to main: Phase 5 completed at 2,656/2,656 assertions and Phase 7 at 1,683/1,683; both exited 0. The tooling suite passed 810/810 tests.
