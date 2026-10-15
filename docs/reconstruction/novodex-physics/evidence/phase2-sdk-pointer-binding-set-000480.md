# SDK pointer-binding setter mapping (`phys_fn_000480`)

Oracle RVA `0x0000edc0` is the 388-byte cdecl setter
`nxSetSdkPointerBinding(void*, void*)` in `Physics/src/PhysicsInternal.cpp`.
It rejects a null key, creates the table on demand for a non-null value,
updates an existing key, removes a binding by replacing it with the last pair,
and destroys and clears the table after its final entry is removed.

The candidate Win32 Release map resolves
`?nxSetSdkPointerBinding@@YA_NPAX0@Z` at `0x100535c0` in
`PhysicsInternal.obj`. `NxPhysicsInternalTests::testPointerBindings` covers
insertion, rebind, removal, allocator frees, and recreation. The registered
Phase 2 mutation that leaves an empty table after final removal fails at
`check_failed emptying the table releases its entries and the table itself`;
the restored internal test suite passes all 98 checks. This is candidate
assertion coverage, not a public oracle DLL differential.
