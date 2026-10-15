# SDK pointer-binding lookup mapping (`phys_fn_000454`)

The row at oracle RVA `0x0000df90` is the complete 58-byte cdecl lookup
`nxGetSdkPointerBinding(void*)` in `Physics/src/PhysicsInternal.cpp`. IDA
decompilation and disassembly confirm the null guards, 8-byte key/value scan,
first-match return, and null-on-miss behavior. This corrects a phase-8 audit
note that accidentally cited VA `0x10007460`, which belongs to the unrelated
actor velocity setter at `0x100073f0`.

The candidate Win32 Release map resolves
`?nxGetSdkPointerBinding@@YAPAXPAX@Z` at `0x100534e0` in
`PhysicsInternal.obj`. `NxPhysicsInternalTests::testPointerBindings` covers
missing keys, insertion, rebind, removal, and final-table cleanup. A temporary
return-null mutation of the getter fails at `check_failed both read back`
(candidate exit 1). The restored source rebuilds `NxPhysics.dll` and the
internal test target; the latter passes all 98 checks. This is candidate
assertion coverage rather than an oracle DLL differential, and the row remains
open in the full-census backlog.
