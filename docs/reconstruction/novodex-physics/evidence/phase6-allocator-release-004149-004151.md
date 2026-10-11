# Allocator-owned field release rows 004149 and 004151

`phys_fn_004149` at oracle RVA `0x9a500` releases the non-null fields at
`+0x0c`, `+0x14`, and `+0x08`, in that order, through
`NxFoundationSDKAllocator` virtual slot `+0x14`; each field is cleared after
its free. `phys_fn_004151` at `0x9a560` is a five-byte jump to 004149. IDA's
disassembly and decompilation establish both the member convention and the
import indirection: the Physics import slot points to the allocator global,
which then points to the allocator object.

`Physics/src/ObjectModel.cpp` now contains the production helper and two
no-inline candidate entrypoints. `NxPhysicsAllocatorReleaseTests` resolves
those candidate functions and the allocator import slot from the staged
candidate's `NxPhysics.map`; on the oracle side it calls the two pinned RVAs.
The test binds a recording allocator at the imported global, then tests all
eight null/non-null masks. It checks free order and count, that owned fields
are cleared, and that null fields remain null. Both oracle and candidate print
`cases=8 failures=0` for both rows, and the staged-pair runner reports
`stdout_delta=0` with exact stderr.

For row-specific falsification, the candidate helper was temporarily mutated
to skip the `+0x14` release and rebuilt. The oracle remained exact; the
candidate reported four failing masks for each row and exited 1, with
`stdout_delta=4` and exact stderr. Restoring the implementation and rebuilding
returned the staged-pair differential to an exact match. The mutation was
reverted before the clean build.

The Phase 6 closure ledger advances to 156 closed and 277 deferred functions.
The fresh full Phase 6 gate passes with all 11 staged-pair targets and its
registered oracle differentials at 1,601/1,601 coverage assertions. No public
Physics headers changed.
