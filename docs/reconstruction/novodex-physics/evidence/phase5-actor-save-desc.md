# Phase 5 actor saveToDesc, dynamic vtable slot 82

The pinned actor dynamic table at RVA `0x104530` points slot 82 at RVA
`0x3690` (`phys_fn_000120`). Ghidra and the Win32 table identify a write-guarded
`saveToDesc(NxActorDescBase&)` implementation. It copies the body pose for a
static actor, or converts the dynamic record quaternion at `+0x5c` and copies
its translation at `+0x50`. It also copies body density at `+0x18`, actor flags
at `+0x14`, actor group at `+0x1c`, and actor user data at `+4`. It does not
touch the descriptor's body pointer or name.

`NpActorVtable::saveToDesc` now transcribes those writes. The new public-DLL
probe exercises a static actor, a normal dynamic actor, a half-turn actor, and
a quarter-turn actor. For each, both the oracle and candidate return the same
12 pose words and descriptor metadata, including preservation of the sentinel
body pointer and name. The dynamic probe initially exposed a separate
construction mismatch: `nxActorLoadFromDescInternal` had left body density at
zero. It now stores the descriptor density at body `+0x18`, as the oracle does.

Eight new output lines are pinned in the Phase 5 gate, bringing its assertion
floor from 667 to 675. The inventory row is `reconstructed`. The family gate
remains red because the shape final tables and actor class family are not yet
closed; the explicit `CANDIDATE-MISSING` marker remains in place.

## Body descriptor dispatch

The immediately preceding actor slot 81 points at RVA `0x24c0`
(`phys_fn_000046`). Its low-level gather had already been differentially
reconstructed as `nxGatherDescriptor0046`, but the public actor virtual still
returned false. `saveBodyToDesc` now calls that gather under the scene read
guard. A new public-DLL probe seeds every byte of an `NxBodyDesc` with `0xa5`,
then hashes the entire descriptor after dispatch. The static actor returns
false and leaves all bytes untouched. Dynamic, rotated and quarter-turn actors
return true and agree byte for byte with the oracle, including mass, flags and
the final solver iteration count.

The first comparison revealed that `nxActorComputeMass` had left the dynamic
record's `+0x110` solver count at zero. It now stores the body descriptor's
`solverIterationCount` there; the four public dispatch cases match. These add
four more Phase 5 assertions, raising the floor to 679. The low-level row was
already `reconstructed` in inventory; this closes its public actor dispatch.
