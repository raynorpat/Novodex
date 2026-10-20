# The convex mesh shape's debug visualisation and cached mass frame

Rows `phys_fn_001393` and `001395` (RVA `0x00027f10`, 3,835 B together: the inventory splits one function at the
jump target `0x00028370`), `001397` (`0x00028e10`, 104 B), and the three rows slot 4 calls that had no code of the
candidate's own to stand on: `phys_fn_002241` (`0x00054bb0`, 692 B), `002156` (`0x000538e0`, 46 B) and `000835`
(`0x0001c5a0`, 30 B). Slot 3 and slot 4 of the mesh shape's vtable (`0x00107630`). All six are naked listing
transcriptions in `Physics/src/ObjectModel.cpp`, 1,457 instructions.
`evidence/phase3-mesh-debug-listing-compare.py` disassembles the built candidate and the oracle and finds every
instruction equal, with the 32 call mappings as expected (`raw/phase3-mesh-debug-mass-20261020/listing-compare.txt`).

## Slot 3, 001393 and 001395: debug visualisation

Before this the vtable's slot 3 aborted. The row: after the flag gate (`+0xde & 8`) and the shape's own axes
(`001305`, the box row the candidate shares), and under `NX_VISUALIZE_COLLISION_SHAPES`, it draws through the
renderer's slot 8 (`+0x20`, a line and a colour):

* the hull's polygons (`001472` builds them first when absent): their outline lines (`0xff8040`) and a line per
  polygon from its centroid along its normal, scaled (`0x4080ff`);
* the hull's edges (`001502` builds them first when absent), `0xff00ff00`;
* the mesh's triangle edges whose bit in the per-triangle flags at `mesh + 0x3c` is clear, and the edge list's
  edges with a negative entry (`002188` builds the list first when absent), `0xffff0000`;
* the vertex-normal lines (`002081` builds the normals first when absent), `0x2266aa`, and the face-normal
  lines, `0xaa6622`.

The four parameters the listing reads at `0x10123b4c`, `0x10123bc4`, `0x10123bd0` and `0x10123bd4` are elements 13,
43, 46 and 47 of the live parameter array. The candidate's array is a file static of `PhysicsSDK.cpp` and the test
targets that link `ObjectModel.cpp` define their own `nxPhysicsSDKParameters`, so a wrapper (`MeshShape::nxMeshDebugRender`,
the vtable entry) copies those four into statics the body reads. That is the one place the code is not the
listing alone; the compare script reads both as the same four named operands.

`NxPhysicsCollisionTests`, `nxDriveMeshDebug`: the oracle's row and the candidate's through a recording renderer,
over the six Task 2i meshes, six hull kinds (none; polygons and edges absent; polygons built; both built;
two hand-built polytopes), identity and posed shapes, twelve parameter sets (zero, NaN, a scale of zero, doubled,
the axes guard on), the flag gate, lazily built normals and edge list present or absent, random per-triangle flag
words, and now and then an empty mesh: 720 cases, 486 with lines, 18,728 lines, under both x87 control words. The
digest of every line (colour and six float words), the line count and the lazily built state are equal: zero
mismatches, equal digests.

Falsification (one instruction of the listing edited, rebuilt, run against the oracle, restored). The counts are
those of the harness as it stood when each was run (`raw/.../md_mut.txt`, `md_mut2.txt`, `mm_mut.txt`); the ones
the ledger quotes were re-run against the final harness (`fin_md.txt`, `fin_mm.txt`) and are rejected with the
counts shown in the tables.

| address | mutation | mismatches |
|---|---|---|
| 0x00027f22 | flag gate `je` made `jne` | 653 |
| 0x00027f4b | shapes-off `jnp` made `jp` | 538 |
| 0x00028000 | vertex array read at `+0x14` | access violation |
| 0x00028022 | pose column read at `+0x10` | 156 |
| 0x00028104 | polygon colour `0xff8040` made `0xff8041` | 400 |
| 0x000281b0 | `fdivr` made `fdiv` | 287 |
| 0x0002825b | normal colour `0x4080ff` made `0x4080fe` | 287 |
| 0x00028460 | triangle colour changed | 286 |
| 0x00028561 | empty-triangle `jbe` made `jb` | access violation |
| 0x00028709 | flag-table null test inverted | 84 |
| 0x0002870f | flag byte read at `+8` for `+0xc` | 90 |
| 0x00028716 | flag bit `1` tested as `2` | 88 |
| 0x0002879d | edge-list null test inverted | 90 |
| 0x00028961 | edge word read at `+4` | 90 |
| 0x000289f8 | vertex-normal null test inverted | access violation |
| 0x00028b16 | face-normal colour changed | 61 |
| 0x00028b73 | face normals read at `+0x20` | access violation |
| 0x00028d23 | scale constant `1/3` made `0.5` | 57 |
| 0x00028d83 | colour changed | 57 |
| 0x000287c3, 0x00028a06, 0x00028b7b | empty-loop `jbe` made `jb` | access violations |
| 0x000287b2 | edge-list present test inverted | 84 |

Three mutations survive and are equivalent in every state the builders reach: `0x00027f89` tests the polygon
pointer (`+0x28`) where the listing tests the polygon count (`+0x24`), `0x00028316` the edge count where the
listing tests the edge array, and `0x00027f9a` is the polygon loop's empty arm for a hull built with no polygons.
Each pair is set together by `001472` and `001502`.

Not covered: the shape-axes block and the bounding-sphere block of `001305` with a posed shape. The candidate's C++
model of `001305` (`BoxShape::nxDebugRender`) and the oracle's differ in the last bits for general poses, so a
posed shape keeps the axes guard at zero here; the row is still entered when the guard is on, with an identity
pose. Mesh images whose vertex normals were never built and have no vertices are not driven (the oracle's block
is then read uninitialised).

## Slot 4, 001397, and 002241, 002156, 000835

Slot 4 (thiscall, `ret 0xc`): with any of the low three flag bits set it returns true; otherwise it asks
`002241` for the mesh's cached mass block (`mesh + 0xb0`: mass, nine inertia words, centre), builds a `MassFrame`
from it (`000827`, the nine doubles to floats by `002156`), folds the shape's own payload in (`000835`, then the
merge `000839`) and returns true, or false when `002241` returns null. `002241` returns the block at once when the
cached mass is not negative; otherwise it runs the Foundation's `NxComputeVolumeIntegrals` over the mesh, narrows
the result, checks the twelve words and the mass with `_fpclass` (any NaN or infinity returns null), reports a
negative volume through `FoundationSDK::error` and flips the sign of the mass-derived words, then stores the block.
The two Foundation imports are the candidate's own IAT slots (the oracle's calls are indirect too), `_fpclass` is the
CRT's.

`NxPhysicsCollisionTests`, `nxDriveMeshMass`: the oracle's slot and the candidate's over fifteen meshes (the ten of
Task 2i, a cube, a tetrahedron, one too large for a float, one with a NaN vertex, one empty), each wound both
ways, with a cache that is negative, NaN, valid or `-0`, five flag values and a random payload, under both control
words: 480 cases, 60 computing the cache, 28 of those failing, 16 with a negative-volume report. The return, the
destination frame, the whole cache block and the Foundation's reports (code, line, message and file hashes) are
equal: zero mismatches. `NxPhysicsShapeVtableTests`' existing 48-sample slot-4 differential still passes.

| row | address | mutation | mismatches |
|---|---|---|---|
| 002241 | 0x00054bd8 | cache sign test `jp` made `jnp` | 210 |
| 002241 | 0x00054bc1 | cache read at `+0xb4` | 150 |
| 002241 | 0x00054bde, 0x00054c05 | triangle count or pointer read from the wrong word | access violations |
| 002241 | 0x00054c0c | density `1.0` made `2.0` | 40 |
| 002241 | 0x00054c29 | integrals result test inverted | 60 |
| 002241 | 0x00054c4b | centre stored at `+0xdc` | 60 |
| 002241 | 0x00054de1 | negative-volume `jp` made `jnp` | 32 |
| 002241 | 0x00054dec, 0x00054df6 | report line or code changed | 16 |
| 002241 | 0x00054e0e, 0x00054e1d | sign flip written to the wrong word | 16 |
| 001397 | 0x00028e16 | flag mask `7` made `3` | 90 |
| 001397 | 0x00028e2e, 0x00028e31, 0x00028e42 | block word, centre address or payload address off | 182, 32, 122 |
| 001397 | 0x00028e66 | success `mov al, 1` made `0` | 452 |
| 001397 | 0x00028e2c | null test inverted | access violation |
| 000835 | 0x0001c5ae | translation record `+0x24` made `+0x28` | 35 |
| 002156 | 0x000538ea | row count `3` made `2` | 60 |
| 002156 | 0x000538e4 | source offset `+0x30` made `+0x34` | 60 |

One mutation survives: `0x00054c6d` drops the signalling-NaN class (`0x207` made `0x206`) from the first finite
check. A float signalling NaN loaded by `fld` is quietened, so the class is unreachable. The checks of the other
eleven words each read one distinct word; the cases make them all non-finite together, so a mutation that moves
one check to another word is not distinguished.

Logs and the mutation and generator scripts are in `raw/phase3-mesh-debug-mass-20261020/`.
