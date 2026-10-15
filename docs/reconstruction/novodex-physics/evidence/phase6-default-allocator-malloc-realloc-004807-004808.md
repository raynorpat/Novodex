# Default allocator malloc and realloc rows 004807 and 004808

`phys_fn_004807` at oracle RVA `0x000b4030` and `phys_fn_004808` at
`0x000b4040` are the default SDK allocator's `malloc` and `realloc` vtable
entries. IDA shows 004807 forwarding its requested byte count to the linked
runtime allocator and discarding the memory-type argument. Row 004808 forwards
the old block and requested size to the linked runtime `realloc`. The matching
production definitions are the existing `SdkDefaultAllocator::malloc` and
`SdkDefaultAllocator::realloc` bodies in
`Physics/src/include/NxSdkAllocatorAccess.inl`. The candidate map resolves
`?malloc@SdkDefaultAllocator@@UAEPAXIW4NxMemoryType@@@Z` at `0x10052e60` and
`?realloc@SdkDefaultAllocator@@UAEPAXPAXI@Z` at `0x10052e90`, both in
`PhysicsInternal.obj`.

`NxPhysicsObjectLayoutTests` resolves the allocator globals through the
candidate map and invokes slots 0 and 2 on the actual oracle and rebuilt DLLs.
Three allocation sizes (1, 16, and 64 bytes) are seeded with distinct byte
patterns, grown by 37 bytes, and checked for prefix preservation before the
oracle and candidate blocks are each released through their own allocator.
The restored differential reports `mismatches=0` and `layout result=differential-pass`.

Both rows were falsified independently in a throwaway `git archive` of commit
`93d6e870`:

- Mutating only `phys_fn_004807` to return null from `malloc` produced
  `mismatches=3` and failed the registered `NxPhysicsObjectLayoutTests`
  staged-pair target (mutant DLL SHA-256
  `d526c8663835330f0007e6ce0b265ecb3fc19053cfd2b4786e03b136b70b471a`).
- Mutating only `phys_fn_004808` to return null from `realloc` produced
  `mismatches=3` and failed the target (mutant DLL SHA-256
  `58c0df009c5c0b127bcdbf069bb3bbf6d6fee6b160766ded73b2754b70c96ec9`).
  Restoring both bodies in the archive returns the target to an exact differential (control DLL SHA-256
`0ea901311ad90d0a4c17a12754f40b3bb73cd8a85ea5212fe2ec84e6e8a3d9fe`). The
oracle DLL identity was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
The raw runner logs are in
`evidence/raw/phase6-default-allocator-004807-004808-20261015/`.

The full Phase 5, 6, and 7 gates passed at 2,784/2,784, 1,684/1,684, and
2,900/2,900 coverage assertions. Reconstruction tooling tests passed (831 tests
and 743 subtests). No public Physics headers changed. The adjacent `free` row
004810 remains open because this fixture uses it for cleanup and does not yet
independently observe its release behavior.
