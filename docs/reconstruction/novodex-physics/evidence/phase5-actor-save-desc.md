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

## Local point velocity

Actor dynamic slot 66, RVA `0x5b40` (`phys_fn_000148`), was a zero-returning
virtual stub. The shipped function transforms the local point through the
record quaternion and the mass-frame orientation, then adds angular velocity
cross the transformed point to linear velocity. The null-record arm returns
zero. The candidate now implements this path under the scene read guard.

The public probe seeds nonzero velocities and a quarter-turn mass frame in the
dynamic record, restores all modified bytes after each call, and compares
static, identity, half-turn and quarter-turn actors. The first three matched
immediately. The quarter-turn Z output differed by one ULP because the image
keeps local-point products on the x87 stack until after the cross product and
velocity addition. Reproducing that store schedule gives `0x40cfffff` in both
DLLs. Four gate assertions raise the Phase 5 floor to 683; inventory row
`phys_fn_000148` is `reconstructed`.

A further 16-case grid varied quaternion, mass-frame matrix, point and both
velocity vectors. The initial floating-point transcription disagreed in nine
cases, usually by one or two ULP. The image stores each combined matrix cell
as float but retains all three transformed point coordinates on the x87 stack.
It spills only the first two cross-product terms before adding linear velocity.
The candidate now follows that sequence; all 16 triples match bit for bit.
Those 16 cases raise the Phase 5 assertion floor to 699.

## World point velocity

The neighboring actor dynamic slot 65, RVA `0x58f0`
(`phys_fn_000146`), computes velocity at a world-space point. It rotates the
mass-frame offset at record `+0x100` by the body quaternion, adds record
translation at `+0x50` to obtain the world mass center, subtracts that center
from the input point, and adds angular velocity cross the resulting radius to
linear velocity. The no-record arm returns zero.

Ghidra did not recognize this 579-byte body as a function, so the Capstone
instruction range was used directly. An initial scalar transcription differed
in 13 of 16 varied cases. The shipped x87 sequence spills center X, the
second and third rotation dot products, radius X/Y and cross-product X/Y to
float. It retains radius Z and the third cross-product term in extended
precision. Matching those store points made all 16 grid triples agree exactly.
Four actor cases also cover the static/null path and normal dynamic dispatch.
These 20 public comparisons raise the Phase 5 assertion floor to 719;
`phys_fn_000146` moves to `reconstructed`.
