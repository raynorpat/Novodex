# `phys_fn_000283`: compiler-generated `NxScene` scalar-deleting destructor

The oracle's `NxScene` base vtable is at `0x10105988`; its slot 0 contains
`0x1000c2f0`, the 31-byte row. IDA disassembly shows the MSVC scalar-deleting
destructor contract: test the flag at `[esp+4]`, install the base vptr, skip
deallocation when bit 0 is clear, otherwise call CRT `_free(this)`, return
`this`, and clean the one-word argument with `ret 4`.

The public header is frozen at `Physics/include/NxScene.h:83-87`: `NxScene` has
an inline protected constructor and inline virtual destructor. The fresh
Win32 Release build emits `??_GNxScene@@MAEPAXI@Z` at `0x10040a90` and
`??_7NxScene@@6B@` at `0x10123804`, both in `NpScene.obj`. The candidate
vtable's first word is `0x10040a90`. Its generated destructor restores that
base vptr, returns `this`, and uses `ret 4`; with bit 0 set it calls the MSVC
sized-delete wrapper with size 8. The wrapper delegates to ordinary scalar
delete and then the same CRT `_free` imported by the oracle. This is the
compiler's emitted implementation of the immutable inline declaration, so
there is no private source body to reconstruct or change.

The row is therefore reclassified as a Phase 8 `compiler_artifact` and marked
`classified`; Phase 7 no longer carries it as unreconstructed code. This is a
static classification proof, not a claim that a row-specific runtime mutation
was run. Validation must continue to confirm the unchanged public headers and
the CMake-produced symbol/vtable relationship.

The fresh Phase 7 Win32 Release gate passed at 1,707/1,707 assertions, with
inventory validation reporting Phase 7 at 194 closed / 366 deferred and zero
unexplained rows. Full transcript: [phase7-gate.log.gz](raw/phase7-nxscene-artifact-classification-20261012/phase7-gate.log.gz).

Fresh evidence commands:

```powershell
dumpbin /rawdata:4 /range:0x10123804,0x10123810 build/Release/NxPhysics.dll
dumpbin /disasm /range:0x10040a90,0x10040ab4 build/Release/NxPhysics.dll
dumpbin /disasm /range:0x1011d720,0x1011d735 build/Release/NxPhysics.dll
dumpbin /disasm /range:0x1011e6b0,0x1011e6b5 build/Release/NxPhysics.dll
```
