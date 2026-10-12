# Fluid collision plane-array appender (`phys_fn_003551`)

`phys_fn_003551` at RVA `0x87940` appends one 96-byte record for a plane shape. The private candidate implementation is `nxFluidCollisionAppendPlane` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA confirms the cdecl `(descriptor, shape)` signature. Only shape type 0 at `+0xd0` is selected; other types return null without changing the descriptor. The selected path calls shape vtable slot 13 as a thiscall with a 96-byte scratch query, then appends a 96-byte record and increments the descriptor count. The scratch starts with the oracle's identity/default words; the first 16 appended bytes are defined by the query result, while the remaining 80 record bytes are copied from uninitialized oracle stack storage and are not compared. Bytes after the 96-byte stride remain untouched.

`NxPhysicsFluidEmitterAbiTests` covers selected type 0 and ignored type 1, checks the virtual callback count, descriptor count and return, the four defined record words, stack balance, and the untouched tail. The first oracle-only run passed those contract checks while remaining red because the candidate symbol was absent. The completed direct differential reports `candidate_present=1`, `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation shifted the candidate's copied scratch window from word 19 to word 20. The oracle contract stayed valid and the differential rejected the mutant (`mismatches=1`, exit 1); restoring word 19 returned the direct differential and fluid-emitter aggregate to zero mismatches. Passing candidate SHA-256 `23f082f19e119d86fe3b37ca6d0ea94ee9fc72869e3a24c0df4f52c92765ae15`; pinned oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw oracle-only, passing differential, and mutation transcripts are in `evidence/raw/phase7-fluid-collision-plane-append-003551/`. This closes only the plane typed-array appender. The mesh appender, router `phys_fn_003553`, scene collector `phys_fn_003555`, and dirty static-collision builder `phys_fn_003505` remain open.
