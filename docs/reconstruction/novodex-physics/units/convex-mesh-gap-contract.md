# IceAdjacencies..ContactConvexHeightfield gap: survey and contract

Written by convex-mesh-gap Task 1 (plan: `docs/superpowers/plans/2026-09-28-convex-mesh-gap.md`;
timing and results in `evidence/convex-mesh-gap.md`). It follows the conventions of
`units/revolute-contract.md` and `units/joint-families-contract.md`: addresses are RVAs (image
base 0x10000000), rows are named by stable ID (the `phys_fn_` prefix is dropped in tables), and
the Capstone listing (`oracle/capstone/manifest.json`) is authoritative over any decompile. A change
to a row list, a layout or a file assignment is made here in the same commit as the change.

## Scope

| unit (work_units.json) | rows | not started (discovered) | other states |
|---|---:|---:|---|
| `IceAdjacencies.cpp` | 2 | 661 B | - |
| `gap:IceAdjacencies.cpp..ContactConvexHeightfield.cpp` | 157 | 61,083 B | dynamically_gated 23,937 B (28 rows), reconstructed 945 B (20 rows) |
| `ContactConvexHeightfield.cpp` | 3 | 4,632 B | - |
| `gap:ContactConvexHeightfield.cpp..ContactMeshMesh.cpp` | 12 | 9,543 B | - |
| adopted: `phys_fn_001537` (from `gap:ConvexHull.cpp..IceAdjacencies.cpp`) | 1 | 196 B | see `## Sub-units` A |
| **total** | **175** | **76,115 B** (127 rows) | |

Every code row from 0x0002daf0 to 0x00046780 is assigned to exactly one sub-unit below (checked
by script: none missing, none twice). The dynamically_gated and reconstructed rows are listed so
the file assignment is complete; they are not rewritten unless a sub-unit says so.

## Evidence used

- **Bundles** (`tools/unit_bundle.py`, committed with this contract): `units/IceAdjacencies.cpp.md`,
  `units/gap__IceAdjacencies.cpp__to__ContactConvexHeightfield.cpp.md`,
  `units/ContactConvexHeightfield.cpp.md`,
  `units/gap__ContactConvexHeightfield.cpp__to__ContactMeshMesh.cpp.md`. Every row now has a
  decompile: 131 from the Ghidra manifest and 43 from the supplement.
- **Supplement** (`oracle/ghidra/supplement.json`): `ghidra/DecompileSupplement.java` run headless
  (Ghidra 12.1.2, `-readOnly -noanalysis`, options sha 9245451897...) over the union of the 34
  previously requested RVAs and the 43 rows of these units that Ghidra never made functions
  (continuations and register-convention helpers): 77 requested, 77 `ok`. The 34 earlier entries
  are byte-identical to the committed ones, and a second run produced a byte-identical file.
- **Strings.** The decompiles carry five `__FILE__` strings, not three: `IceAdjacencies.cpp`
  (001539 lines 266/267, 001541), **`ContactBoxMeshICE.cpp`** (001772, line 1706),
  `ContactConvexHeightfield.cpp` (001847 line 583, 001849 line 2594), **`ContactMeshHeightfield.cpp`**
  (001865, line 328) and `ContactMeshMesh.cpp` (001876). "Opcode is not OK." (0x10107bc4, pooled)
  is the message of all four OPCODE-failure reports. See `### Two translation units the census
  missed`.
- **Dispatch tables** (relocated pointers in `oracle/pe.json`): the support-map tables at
  0x10107848, 0x1010785c, 0x1010786c, 0x10107890; the shape-pair matrices written by 002338
  (`evidence/phase3-narrow-phase.md`, `### Matrix A` / `### Matrix B`); the switch tables at
  0x0002e550 (001558) and 0x0003acc0 (001745); the function pointer 0x00046d16 -> 001874 in 001876.
- **.rdata order** (read from the oracle image, `Unreal_3/Binaries/NxPhysics.dll`, sha 4b7db3e1...):
  see `### Translation-unit boundaries in .rdata`.
- **Earlier phases**: `evidence/phase3-narrow-phase.md` (matrices, box/box, capsule/capsule,
  segment/segment, the 002264 stop), `evidence/phase3-exports.md` and `phase3-leaf-kernels.md`
  (the Nx* intersection exports), `evidence/phase4-formats.md` (the mesh column blocked on OPCODE
  and on `mesh`, the TriangleMesh reader), `evidence/phase4-pmap-reconstruction.md`,
  `evidence/phase5-object-model.md` (the reconstructed small rows: 001544, 001552..001589,
  001645, 001649, 001655, 001657, 001659, 001663, 001665, 001668, 001787).
- **Vendored correspondence**: `evidence/phase4-third-party-map/opcode_map.csv` and
  `opcode_review.csv`.

### Two translation units the census missed

`tools/work_units.py` seeds units from `ghidra["strings"]` keyed by the string entry's RVA, but
Ghidra's entries for two of these file names start two bytes and one byte early, on junk bytes:
`0x00107b8a '#<\Epic\...\ContactBoxMeshICE.cpp'` and `0x00107cff '=\Epic\...\ContactMeshHeightfield.cpp'`.
The code references the real starts (0x10107b8c, 0x10107d00), so the lookup misses both and the
image names 60 translation units, not 57. The third one elsewhere is
`0x001194cf '?\...\core\Articulation.cpp'`. The fix is to key `files` by the RVA of the path's
first backslash rather than by the entry's RVA; it changes `work_units.json` and the names of
these units' bundles, so this task records it and does not apply it (see `## Open items`).

### Translation-unit boundaries in .rdata

Every translation unit that includes the Foundation's `NxMath.h` emits the same 24-byte block at
the start of its .rdata contribution: `NxPiF64`, `NxHalfPiF64`, `NxPiF32`, `NxHalfPiF32`
(`182d4454fb210940 182d4454fb21f93f db0f4940 db0fc93f`). A unit with none of the Foundation
headers (the ICE-derived files) has no block. "Start" and not "end": the block at 0x10107610
immediately precedes the MESH shape table at 0x10107630, and under the "end" reading the MESH
shape's unit (Foundation code) would have to run on to the next block at 0x101078a8, across
`ConvexHull.cpp`'s and `IceAdjacencies.cpp`'s strings.
The groups of referenced data between consecutive blocks, in image order:

| block at | data after it, and the rows that reference it |
|---|---|
| (none) 0x10107740..0x101078a0 | IceAdjacencies strings (001539, 001541); support-map tables 0x10107848..0x1010789c (001552..001589); doubles 0x10107880/88 (first use 001573) |
| 0x101078a8 | 0x101078cc 0.9999f (001661) |
| 0x101078d8, 0x101078f0, 0x10107908 | none (three units with no .rdata of their own) |
| 0x10107920 | 0x10107938 1e-5f (001690) |
| 0x10107940 | 0x10107958 1e-5f (001694) |
| 0x10107960 | 0x10107978 -FLT_MAX (001698 NxSeparatingAxis) |
| 0x10107980, 0x10107998, 0x101079b8 | none |
| 0x101079d0 | 0x101079e8 -1e-6f (001712 NxRayTriIntersect) |
| 0x101079f0 | 0x10107a08 1e-5f (001724, 001728), 0x10107a0c/10 +-FLT_EPSILON (001732, first use) |
| 0x10107a18 | 0x10107a30 0.99999988f (001734 NxRayCapsuleIntersect) |
| 0x10107a38 | none |
| 0x10107a50 | box/box tables and 0.999f (001741, 001743, 001745) |
| 0x10107b38 | 0.666f, 89128.96f (001753 box/capsule) |
| 0x10107b60 | {0,2,1} table and 0.01f (001768), `ContactBoxMeshICE.cpp` (001772), "Opcode is not OK." |
| 0x10107bd8 | 0.9998f (001775 capsule/capsule) |
| 0x10107bf8, 0x10107c18 | none |
| 0x10107c38 | double 1e-6 (001816, 001838) |
| 0x10107c60 | `ContactConvexHeightfield.cpp` (001847, 001849) |
| 0x10107cb8 | {0,2,1} table (001857) |
| 0x10107ce0 | 0.1f (001859), `ContactMeshHeightfield.cpp` (001865) |
| 0x10107d40 | `ContactMeshMesh.cpp` (001876) |

What this establishes, and what it does not:

- 001690 (segment/segment) and 001694 (segment/triangle) are in **different** units, as are the
  box/box rows (001739..001749) and box/capsule (001751, 001753), and capsule/capsule (001775)
  and box/mesh. With the empty blocks between them, this suggests roughly one file per kernel.
- 001768 and 001772 share a unit: **ContactBoxMeshICE.cpp owns 001755..001772** (the rows between
  the box/capsule unit and 001772 in .text).
- 001857 is in a unit of its own, after `ContactConvexHeightfield.cpp` and before
  `ContactMeshHeightfield.cpp`; 001859 and 001865 share `ContactMeshHeightfield.cpp`. 001855 has
  no data; it sits between 001853 and 001857 and goes with 001857.
- The support-map classes and the ICE-shaped helpers (MeshBuilder2, the vertex reduction, the
  valencies) have no block, so they are in Foundation-free units; the table cannot split them
  further. Float literals are not reliable single-unit markers (0x10107a08 is shared by five
  rows in three areas), so only strings, tables and first uses are used above.

### Vendored ICE correspondence

The range is ICE-derived in places, but **none of its rows has vendored source**. OPCODE 1.3's
`Ice/` (the only ICE tree in `External/opcode`) holds IceAABB, IceContainer, IceHPoint,
IceIndexedTriangle, IceMatrix3x3/4x4, IceOBB, IcePlane, IcePoint, IceRandom, IceRay,
IceRevisitedRadix, IceSegment, IceTriangle and IceUtils; the classes identified here come from
the full ICE library and are not in it:

| rows | ICE shape | evidence |
|---|---|---|
| 001537..001548 | `Adjacencies` (IceAdjacencies.cpp): AddTriangle, UpdateLink, CreateDatabase, ComputeNbBoundaryEdges, ~Adjacencies, Init | the two method names in the asserts; the 12-byte AdjTriangle with 29-bit references (0x1fffffff = boundary) and the edge number in bits 30-31; the radix-sorted edge database |
| 001591..001637 | `MeshBuilder2` | 13 ICE `Container`s, a create block with twelve flag bytes, the Init/AddFace/Build/FreeUsedRam shape, CRT `new`/`free` |
| 001663..001668 | `Valencies` | per-vertex valence from an edge list, three owned arrays |
| 001645/001647/001659 | vertex reduction (ICE `ReducedVertices`-like) | radix sort on three float keys, remap and reduced array |
| 001550..001589 | support-vertex maps (cube-map lookup plus three fillers) | 6*n*n byte maps, face-major directions |

So `tools/vendored_match.py` cannot be pointed at them: its inputs are the qhull and OPCODE 1.3
trees and their `phase4-third-party-map` maps, and none of these functions has a source line to
map to. Name matching was done by hand against the asserts and the upstream `Ice/` file list
above; nothing in the range matched a vendored function. What the range **calls** is vendored and
already mapped (`opcode_map.csv`), and the candidate reaches it through `External/opcode`:

| callee | vendored function | called by |
|---|---|---|
| 004836, 004838, 004840, 004844, 004846 | `Container::Container`, `Empty`, `Resize`, copy ctor, `~Container` | MeshBuilder2, 001641, 001661, 001753, compound, mesh/height-field |
| 005157, 005159, 005163, 005177 | `RadixSort::RadixSort`, `~RadixSort`, `Sort`, `SetRankBuffers` | 001541, 001647, 001849 |
| 005189 | `IndexedTriangle::FindEdge` | 001539 |
| 005191 | `IceMaths::InvertPRMatrix` | 001653 |
| 005197 | `Matrix4x4::Invert` | 001822 |
| 005179, 005183, 005185 | `Triangle::Area`, `Triangle::Center`, `Triangle::Inflate` | 001849, 001708 |
| 004986, 005027, 005067 | `AABBTreeCollider::Collide`, `LSSCollider::Collide`, `OBBCollider::Collide` | mesh/mesh, capsule/mesh, box/mesh, convex/height-field, mesh/height-field |

## Shared structures

Layouts named here are established by the listing sites cited; anything not cited is not
established and must not be filled in from what ICE or NovodeX usually do.

**Collision shape** (`Physics/src/include/NarrowPhase.h` `NxCollisionShape`): rotation +0x0c
(row major, nine floats), translation +0x30, owner +0x04, collision object +0x9c, type +0xd0,
geometry +0xe0 (box extents +0xe4..+0xec; capsule radius +0xe0, half height +0xe4; **mesh: +0xe0
is the `TriangleMesh*`**, read at 0x0003bb4b in 001755 and at 0x000411eb/0x000411fd in 001820). Matrix A
entries are `cdecl(shape0, shape1, NxContactSink*, context)`, matrix B entries
`cdecl(shape0, shape1, context)` returning `al`.

**TriangleMesh** (`Physics/src/include/TriangleMesh.h`, 0xb0+ bytes). Established there: +0x00
vtable 0x10108608 (19 slots), +0x08 InternalTriangleMesh (+0x10 vertices, +0x14 triangles, +0x28
OPCODE model), +0x6c, +0x7c height-field axis, +0x80 extent, +0x8c..+0x98, +0xa0 convex mesh.
Established by this survey for the first time:
- **+0x04 is a second vtable pointer**: the constructor stores the 41-slot abstract table
  0x10106a58 there (0x00055493) and then 0x101085d4 (0x000554a4; the destructor stores it again
  at 0x00055581), a 12-slot polygon interface. Slots used in this range: 2 (+0x8), 3 (+0xc,
  polygon count), 4 (+0x10, polygon i; its plane at polygon+0xc), 11 (+0x2c, projection on an
  axis). Slot owners: 002211, 002213, 002215 (reconstructed), 002221, 002223, 002225, 002227,
  002229, 002231, 002217, 002219, **002249 (slot 11, 459 B, which calls the support-map lookup
  001556)**; nine of the twelve (649 B) are not started. 001820 passes `mesh+4` for both shapes
  (`add ecx,4` at 0x000412f0, `add eax,4` at 0x00041341).
- +0x84 is a cached three-way state (0, 1, other): 001859 reads it at 0x00044b83 and 0x00044ba7
  and writes it at 0x00044bb1.
- +0x88 is a lazily built pointer: 001834 reads it at 0x00041c23 and calls 002188 at 0x00041c31
  when it is null.
- +0xa4 and +0xa8 are read by 001820 (0x000411f1..0x0004120d) and passed to 001818.

**Adjacencies** (001546): +0x00 NbFaces, +0x04 faces (`new[]` of 12-byte AdjTriangle through
004803 slot 0 with a count cookie, trivial constructor 0x00027f00). AdjTriangle word: bits 0-28
the adjacent face (0x1fffffff = boundary), bit 29 the active-edge flag, bits 30-31 the adjacent
face's edge number. Temporary AdjEdge array, 3*NbFaces entries of {Ref0 = min vertex, Ref1 = max
vertex, FaceNb} (12 bytes), allocated with type 1 (temporary) and freed by 001546 after
CreateDatabase. ADJACENCIESCREATE: +0x00 NbFaces, +0x04 DFaces (udword*), +0x08 WFaces (uword*),
+0x0c vertices (when non-null the active-edge pass runs through an EdgeList on the stack).

**Support maps** (001550..001589): base +0x00 vptr (0x10107848, slots 1 and 2 `_purecall`
0x000f41dc), +0x04 n, +0x08 6*n*n. A (0x1010785c) and B (0x1010786c): +0x0c byte map, +0x10
hull (+0x18 centroid, +0x24 polygon count, +0x28 polygons of 0x24 bytes with the plane at +0xc;
both built lazily by 001472 when zero). C (0x10107890): +0x0c and +0x10 byte maps, +0x14 vertex
source (+0x0c count, +0x10 vertices). Slot 0 scalar deleting destructor, 1 allocate, 2
compute(sample, dir), 3 the no-op 001583. The census object `phys_data_000893` is two 4-slot
tables (A at 0x1010785c, B at 0x1010786c), not one 8-slot table.

**MeshBuilder2** (001593, 001623): 13 Containers at +0x00, +0x10, ..., +0xc0; +0xd0 (create
+0x04), +0xd4/+0xd8/+0xdc stream counts (create +0x00/+0x08/+0x0c), +0xe0 face count,
+0xec/+0xf0/+0xf4 the three duplicated 12-byte streams (create +0x10/+0x14/+0x18), +0xf8 faces
(0x30-byte records), +0xfc vertex references (12-byte records), +0x100, +0x104, +0x118..+0x123
the create block's twelve flag bytes (+0x1c..+0x27). Its allocations are the static CRT's
`operator new` (005701, 0x000f48c0) and `free` (005700, 0x000f48bb), which pair with each other.
Allocator census over 0x0002daf0..0x00046ab0: 33 calls to the 004803 getter, 20 to CRT
`operator new`, 29 to CRT `free`, and no reference to the imported `nxFoundationSDKAllocator`.

**Contact sink**: `NxContactSink` in `Physics/src/include/ContactGeneration.h`; emission through
000873 (`NxEmitContact`, dynamically_gated) or 000875 (0x0001d8e0, 915 B, not started).

## Sub-units

Each table lists the rows, their state and phase, the direct callers from
`oracle/dependencies.dot` (which attributes a table slot to the constructor that installs the
table, and a continuation's edges to the row it continues) and what the row does. "Candidate"
means what `Physics/src` has today.

### A. IceAdjacencies.cpp - `Physics/src/IceAdjacencies.cpp` (new)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001537 | 0x0002daf0 | 196 | discovered | 4 | 001546 | Adjacencies::AddTriangle (AddEdge x3 inlined): face ATri = 0xffffffff x3, three AdjEdge {min, max, face}; register arguments (EAX = &nbEdges, EDI = v0) |
| 001539 | 0x0002dbc0 | 293 | discovered | 4 | 001541 | Adjacencies::UpdateLink: FindEdge (005189) on both faces, asserts at lines 266/267, writes ATri = (edge << 30) + tri; register arguments (EDI = tri0, EAX = tri1) |
| 001541 | 0x0002dcf0 | 368 | discovered | 4 | 001546 | Adjacencies::CreateDatabase: alloca'd key array, RadixSort twice (005157/005163/005159), non-manifold assert, UpdateLink per matching pair |
| 001542 | 0x0002de60 | 91 | discovered | 3 | 001465 | Adjacencies::ComputeNbBoundaryEdges: counts ATri words with (w & 0x1fffffff) == 0x1fffffff |
| 001544 | 0x0002dec0 | 37 | reconstructed | 2 | 001465, 002186, 002239 | Adjacencies::~Adjacencies (frees mFaces through 004803 slot 3); already reconstructed as an ObjectModel model |
| 001546 | 0x0002def0 | 490 | discovered | 2 | 001465, 002186 | Adjacencies::Init(ADJACENCIESCREATE): new[] faces and temporary edges through 004803, AddTriangle per face (DFaces, WFaces or 0/1/2), CreateDatabase, then active-edge bits (0x20000000) from an EdgeList (002052/002063/002060) |
| 001548 | 0x0002e0e0 | 122 | discovered | 2 | continuation | continuation of 001546 (the active-edge loop and the EdgeList release) |

Totals: 7 rows; discovered 1,560 B, reconstructed 37 B

- **Evidence.** The asserts name the file and two methods; 001537 (AddTriangle, register
  convention) is called only by 001546 and sits immediately before 001539, so it belongs here and
  not to the ConvexHull.cpp gap where the census left it (`evidence/phase4-formats.md` put it in
  `convex_wrappers` by layout adjacency only). No .rdata block: the file includes no Foundation
  header.
- **Entry rows.** 001546 (Init), 001542, 001544. Callers outside: 001465 (`ConvexHull.cpp`, 791 B),
  002186 (0x000543d0) and 002239 (TriangleMesh span). **Candidate:** none of the three callers
  exists; 001544 is modelled in `ObjectModel.cpp` (keep that test green, or route it to the new
  row).
- **Callees outside.** 004803; RadixSort 005157/005163/005159 (vendored); IndexedTriangle::FindEdge
  005189 (vendored); 002160 (the SetIceError report row, reconstructed); 000001, 005695
  (`__chkstk` for the alloca in 001541); **EdgeList.cpp 002052 (zero), 002063 (`EdgeList::Init`,
  not started), 002060 (release, reconstructed)**. 002063's closure is 002054, 002058, 002061,
  002063 (3,179 B, `EdgeList.cpp`, not started) plus vendored `Plane::Set` (005155) and
  `Triangle::Normal` (005181) - see `## Out-of-range prerequisites`.
- **x87.** None of consequence (integer code); 001546's active-edge pass is EdgeList's.
- **Test route.** New `NxPhysicsThirdPartyTests` family `ice_adjacencies`: build an
  ADJACENCIESCREATE over each of the six `NxMesh` fixtures that file already has (height field,
  soup, flat grid, degenerate, single triangle, closed box), as 32-bit DFaces and as 16-bit
  WFaces, with and without vertices; call the oracle's 001546 at its RVA on an oracle-side object
  and the candidate's on its own; compare the return value, NbFaces, every AdjTriangle word, and
  001542's count. The closed box is manifold; the soup and grid have boundaries; the degenerate
  set reaches 001539's invalid-edge arm (a triangle with a repeated vertex) and a three-faces-per-
  edge case reaches 001541's non-manifold arm - capture the report the way the OPCODE families
  capture SetIceError. Release through 001544 on each side.

### B. Support-vertex maps - `Physics/src/IceSupportMaps.cpp` (new; file name chosen by this contract)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001550 | 0x0002e160 | 131 | discovered | 4 | 001556 | cube-face projection: dominant axis of the direction -> face*2 or sign, and the two in-face coordinates scaled by 1/abs(major) |
| 001552 | 0x0002e1f0 | 17 | reconstructed | 4 | 001565, 001571, 001575 | base constructor: vptr 0x10107848, +4/+8 = 0 |
| 001554 | 0x0002e210 | 7 | reconstructed | 4 | 001577, 001585, 001587 | base vptr store 0x10107848 |
| 001556 | 0x0002e220 | 196 | discovered | 2 | 001407, 002249 | lookup(dir) -> sample index (face*n + round(u))*n + round(v), with the 0.5 fix-up of the rounding |
| 001558 | 0x0002e2f0 | 122 | discovered | 4 | 002255 | Init(n): +4 = n, +8 = 6*n*n, slot 1 (allocate), then per face, row and column a direction and slot 2 (compute); switch table 0x0002e550 |
| 001560 | 0x0002e370 | 478 | discovered | 4 | continuation | continuation of 001558 (the six-face switch body) |
| 001563 | 0x0002e570 | 35 | reconstructed | 4 | 001552 | base scalar deleting destructor, frees through 004803 |
| 001565 | 0x0002e5a0 | 34 | reconstructed | 4 | 002255 | constructor wrapper A: +0x10 = hull, vptr 0x1010785c |
| 001567 | 0x0002e5d0 | 63 | discovered | 4 | 001565 | slot 1 of A and B: hull polygon count (+0x24, built lazily by 001472), fails above 255, allocates +8 bytes through 004803 into +0xc |
| 001569 | 0x0002e610 | 34 | discovered | 4 | 001565 | slot 2 of A: byte = support vertex of the hull for dir (001496) |
| 001571 | 0x0002e640 | 34 | reconstructed | 4 | 002255 | constructor wrapper B: vptr 0x1010786c |
| 001573 | 0x0002e670 | 323 | discovered | 4 | 001565 | slot 2 of B: from the hull centroid (+0x18) along dir, the nearest facing polygon plane (0x24-byte polygons at +0x28) -> byte index |
| 001575 | 0x0002e7c0 | 35 | reconstructed | 4 | 002255 | constructor wrapper C: +0x14 = vertex source, vptr 0x10107890 |
| 001577 | 0x0002e7f0 | 77 | reconstructed | 4 | 001589 | C destructor body: frees +0x10 and +0xc, stores the base vptr |
| 001579 | 0x0002e840 | 70 | discovered | 4 | 001575 | slot 1 of C: vertex count ([+0x14]+0xc) below 256, allocates the two byte maps +0xc and +0x10 |
| 001581 | 0x0002e890 | 472 | discovered | 4 | 001575 | slot 2 of C: brute-force minimum and maximum projection over the vertex array -> two bytes |
| 001583 | 0x0002ea70 | 1 | dynamically_gated | 2 | 000008, 000333, 000647, 000663, 000893, 001397, 001552, 0... | one-byte ret: slot 3 of every table here and of nine other tables |
| 001585 | 0x0002ea80 | 72 | reconstructed | 4 | 001565 | A scalar deleting destructor |
| 001587 | 0x0002ead0 | 72 | reconstructed | 4 | 001565 | B scalar deleting destructor |
| 001589 | 0x0002eb20 | 34 | reconstructed | 4 | 001575 | C scalar deleting destructor |

Totals: 20 rows; discovered 1,889 B, reconstructed 417 B, dynamically_gated 1 B

- **Evidence.** Four tables, all installed by 001552/001565/001571/001575, whose constructors
  are called only by 002255 (the TriangleMesh finishing row). The tables sit directly after
  IceAdjacencies' strings with no .rdata block, so the file has no Foundation header; it may be
  IceAdjacencies.cpp itself or a neighbouring ICE-style file - not established. The name is a
  choice.
- **Entry rows.** 001558 (Init, thiscall ret 4), 001556 (lookup), the three constructor wrappers.
  Callers outside: 002255 (constructs and inits), 002249 (TriangleMesh polygon-interface slot 11,
  calls 001556), 001407 (MESH shape slot 7, calls 001556). **Candidate:** the reconstructed small
  rows among 001552..001589 are modelled in `ObjectModel.cpp`; 001407 is modelled in
  `ObjectModel.cpp` (`MeshShape::nxMeshSweepPrepared`); 002255 and 002249 do not exist.
- **Callees outside.** **001472 (0x0002b6f0, 664 B) and 001496 (0x0002c8f0, 296 B)**, both not
  started, in the ConvexHull.cpp gap - P-Hull in `## Out-of-range prerequisites`. 004803.
- **x87.** 001556, 001558/001560, 001573, 001581 are float code: `/arch:IA32`.
- **Test route.** `NxPhysicsThirdPartyTests` family `support_maps`: for each class, construct
  on each side through its own wrapper (001565/001571/001575) over a synthetic hull (A, B: the
  +0x18/+0x24/+0x28 fields filled so 001472 is not reached, which keeps the differential usable
  before P-Hull lands) or vertex source (C), call 001558 with n = 1..8, compare the byte maps
  (6*n*n bytes) and the return; call 001556 over random and axis-aligned directions (ties at the
  face and cell boundaries) and compare the index.

### C. MeshBuilder2 - `Physics/src/IceMeshBuilder2.cpp` (new; name from the ICE correspondence)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001591 | 0x0002eb50 | 141 | discovered | 4 | 001599, 001603, 001607, 001627, 001661, 002087 | Container::Add of three dwords (Resize 004840 when full) |
| 001593 | 0x0002ebe0 | 301 | discovered | 4 | 002087 | MeshBuilder2 constructor: 13 Containers (004836) at +0x00..+0xc0, zeroed fields to +0x123 |
| 001595 | 0x0002ed10 | 134 | discovered | 4 | 001623 | duplicate a 12-byte-element array with CRT operator new (count cookie, trivial constructor 0x27f00), copy or zero |
| 001597 | 0x0002eda0 | 1,251 | discovered | 4 | 002087 | face validation and record (degenerate and range tests) |
| 001599 | 0x0002f290 | 61 | discovered | 4 | 001633 | build pass: unshared-vertex expansion (flags +0x11a/+0x121) |
| 001601 | 0x0002f2d0 | 480 | discovered | 4 | continuation | continuation of 001599 |
| 001602 | 0x0002f4b0 | 272 | discovered | 4 | 001633 | build pass: vertex reduction (001645/001647/001659) and face remap |
| 001603 | 0x0002f5c0 | 954 | discovered | 4 | 001633 | build pass: normals (flags +0x11a/+0x11b) |
| 001605 | 0x0002f980 | 58 | discovered | 4 | continuation | continuation of 001603 |
| 001607 | 0x0002f9c0 | 380 | discovered | 4 | 001633 | build pass: copy the kept vertex, uvw and colour streams into output Containers (+0x50, +0x60, ...) |
| 001609 | 0x0002fb40 | 257 | discovered | 4 | 001617 | helper: first-reference vertex copy into an output Container |
| 001611 | 0x0002fc50 | 938 | discovered | 4 | 001625 | build pass: per-stream reduction (called three times by 001625 with 1, 2, 4) |
| 001613 | 0x00030000 | 186 | discovered | 4 | continuation | continuation of 001611 |
| 001615 | 0x000300c0 | 120 | discovered | 4 | continuation | continuation of 001611 |
| 001617 | 0x00030140 | 141 | discovered | 4 | 001627 | helper: per-face remap (calls 001609) |
| 001619 | 0x000301d0 | 222 | discovered | 4 | continuation | continuation of 001617 |
| 001621 | 0x000302b0 | 364 | discovered | 4 | 001623, 001629 | FreeUsedRam: Empty the 13 Containers, CRT free of the owned arrays |
| 001623 | 0x00030420 | 398 | discovered | 4 | 002087 | Init(create): copies 12 flag bytes (create+0x1c..+0x27 -> +0x118..+0x123), duplicates three streams (001595) |
| 001625 | 0x000305b0 | 101 | discovered | 4 | 001633 | pass driver: 001611 over streams 1, 2 and 4 |
| 001627 | 0x00030620 | 1,764 | discovered | 4 | 001631 | AddFace (1,764 B): face record, smoothing groups, per-corner references |
| 001629 | 0x00030d10 | 127 | discovered | 4 | 002087 | destructor: FreeUsedRam then ~Container x13 |
| 001631 | 0x00030d90 | 441 | discovered | 4 | 001633 | build pass: face sort (RadixSort) and output |
| 001633 | 0x00030f50 | 346 | discovered | 4 | 002087 | Build(result): the pass sequence 001625, 001599, 001602, 001603, 001607, 001631, then the result fields |
| 001635 | 0x000310b0 | 711 | discovered | 4 | continuation | continuation of 001633 |
| 001637 | 0x00031380 | 88 | discovered | 4 | continuation | continuation of 001633 |

Totals: 25 rows; discovered 10,236 B

- **Evidence.** One call-graph cluster: every row is reached from 001633/001623/001627/001629/
  001593, all called only by 002087 (0x000523c0, the `EdgeList.cpp..InternalTriangleMesh.cpp`
  gap, Phase 4), except 001591 (also used by 001661). No strings, no .rdata.
- **Entry rows.** 001593 (constructor), 001623 (Init), 001627 (AddFace), 001633 (Build), 001629
  (destructor), 001621 (FreeUsedRam), called in that order by 002087. **Candidate:** 002087 does
  not exist.
- **Callees outside.** Vendored Container rows; RadixSort; the vertex reduction 001645/001647/
  001659 (sub-unit D - write 001647 with this sub-unit, see `## Task split`); CRT `operator new`
  005701 and `free` 005700 (the candidate uses its own CRT's; the two are paired).
- **x87.** 001597, 001603 (normals), 001627 are float code: `/arch:IA32`.
- **Test route.** `NxPhysicsThirdPartyTests` family `ice_meshbuilder2`, driven as 002087 drives
  it: constructor, Init with a create block (vertex, uvw and colour streams; the twelve flags
  toggled per case), AddFace per triangle of each `NxMesh` fixture with duplicated vertices,
  degenerate faces and several materials, Build, then compare the result block and every output
  array by content (the two sides allocate from different CRT heaps: never compare pointers).

### D. Mesh utilities - `Physics/src/IceMeshTools.cpp` (new; name chosen by this contract)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001639 | 0x000313e0 | 157 | discovered | 7 | 002296 | function-static pose: identity 4x4 at 0x10123ca0 plus six zero words, guarded by the byte 0x10123c78 |
| 001641 | 0x00031480 | 61 | discovered | 3 | 001465 | edge-pair list dedupe over a copied Container (004844): removes both copies of an edge that appears twice |
| 001643 | 0x000314c0 | 433 | discovered | 3 | continuation | continuation of 001641 |
| 001645 | 0x00031680 | 29 | reconstructed | 2 | 001451, 001476, 001602, 001611 | vertex-reduction init: [+4] = arg1, [+0] = arg2, zero +8/+0xc/+0x10 |
| 001647 | 0x000316a0 | 492 | discovered | 2 | 001451, 001476, 001602, 001611 | vertex reduction: RadixSort on the x, y and z keys, writes the remap and the reduced vertex array (004803) |
| 001649 | 0x00031890 | 61 | reconstructed | 3 | 001461 | release of two owned pointers [+0], [+4] |
| 001651 | 0x000318d0 | 1,240 | discovered | 3 | 001461 | per-face and per-vertex normal arrays for a mesh description (allocates nbFaces*12 and nbVerts*12 unless supplied); calls 002144 |
| 001653 | 0x00031db0 | 1,618 | discovered | 3 | 001818, 001849, 002264 | two poses (null = identity, else InvertPRMatrix 005191) and the relative transform of two frames (1,618 B) |
| 001655 | 0x00032410 | 66 | reconstructed | 3 | 001431 | buffer pop |
| 001657 | 0x00032460 | 60 | reconstructed | 4 | 001472 | array reverse |
| 001659 | 0x000324a0 | 65 | reconstructed | 2 | 001451, 001476, 001602, 001611 | release [+0x10], [+0xc] (the vertex-reduction destructor) |
| 001661 | 0x000324f0 | 155 | discovered | 2 | 001514, 001812, 001834, 001836 | add a unique axis to a Container of directions: canonical sign, rejected when abs(dot) > 0.9999 against a stored axis |
| 001663 | 0x00032590 | 19 | reconstructed | 3 | 001411 | Valencies constructor: zero +0..+0x10 |
| 001665 | 0x000325b0 | 95 | reconstructed | 3 | 001411, 001415, 001419 | Valencies destructor: free +8, +0xc, +0x10 |
| 001667 | 0x00032610 | 512 | discovered | 3 | 001411 | Valencies::Compute: per-vertex edge counts through an EdgeList (epsilon 0.001f), offsets, adjacent vertices |
| 001668 | 0x00032810 | 40 | reconstructed | 3 | 001413 | conditional dot-delta sum |

Totals: 16 rows; discovered 4,668 B, reconstructed 435 B

- **Evidence.** The rows between MeshBuilder2 and the distance kernels, with no strings. 001661
  opens the first Foundation-header unit after the ICE files (its 0.9999f follows the block at
  0x101078a8); 001639..001659 and 001663..001668 have no .rdata. The grouping into one candidate
  file is a choice; each row is independent.
- **Entry rows and callers outside.** 001639 <- 002296 (Phase 7, TriangleMesh..Controller gap);
  001641 <- 001465 (ConvexHull.cpp); 001645/001647/001659 <- 001451, 001476 (ConvexHull gap) and
  MeshBuilder2; 001649/001651 <- 001461 (0x0002ae60); 001653 <- 001818, 001849, 002264 (the
  continuous-collision sweep, a recorded STOP in `evidence/phase3-narrow-phase.md`); 001661 <-
  001514 (convex wrappers), 001812, 001834, 001836; 001663..001668 <- 001411, 001413, 001415,
  001419 (0x00029780.., MESH-shape helpers). **Candidate:** 001413 is modelled in `ObjectModel.cpp`
  (calls 001668's model); the rest have no candidate callers.
- **Callees outside.** 002144 (0x000532e0, 217 B, not started, closure empty) for 001651; 005191
  (vendored) for 001653; 004844 (vendored) for 001641; the EdgeList closure for 001667.
- **x87.** 001651, 001653, 001661, 001668: `/arch:IA32`.
- **Test route.** Leaf families in `NxPhysicsCollisionTests` for the float rows (001653 over random
  poses with and without the null arms; 001661 over direction sets with near-parallel pairs at the
  0.9999 threshold) and `NxPhysicsThirdPartyTests` for the ICE ones (001641 edge lists with
  duplicates; 001647 over the NxMesh fixtures with welded duplicates; 001651 over the fixtures;
  001667 over the fixtures). 001639 initialises oracle globals once: compare the block it returns.

### E. Distance kernels - `Physics/src/Distance.cpp` (new; name chosen by this contract)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001670 | 0x00032840 | 403 | discovered | 4 | 001688 | point-OBB squared distance: point into the box frame, clamp to the extents, optional closest point |
| 001672 | 0x000329e0 | 1,326 | discovered | 2 | 001694, 001927, 002029 | point-triangle squared distance (region test), barycentric outputs |
| 001674 | 0x00032f10 | 1,751 | discovered | 4 | 001676 | line-box helper Face (register convention; reached only through 001676) |
| 001676 | 0x000335f0 | 194 | discovered | 4 | 001684 | line-box helper CaseNoZeros (register convention) |
| 001678 | 0x000336c0 | 471 | discovered | 4 | 001684 | line-box helper Case0 (register convention) |
| 001680 | 0x000338a0 | 195 | discovered | 4 | 001684 | line-box helper Case00 (register convention) |
| 001682 | 0x00033970 | 214 | discovered | 4 | 001684 | line-box helper Case000 (register convention) |
| 001684 | 0x00033a50 | 237 | discovered | 4 | 001688 | line-box squared distance: box frame, sign flips, dispatch to 001676/001678/001680/001682 |
| 001686 | 0x00033b40 | 445 | discovered | 4 | continuation | continuation of 001684 |
| 001688 | 0x00033d00 | 378 | discovered | 4 | 001751, 001753, 001785 | segment-box squared distance: 001684, then the segment clamp, 001670 at the ends |
| 001690 | 0x00033e80 | 1,836 | dynamically_gated | 2 | 001694, 001774, 001775 | segment-segment squared distance, already in Physics/src/NarrowPhase.cpp |
| 001692 | 0x000345b0 | 684 | discovered | 3 | 001762, 001844, 001859 | closest points of two lines given origin and direction pairs (684 B) |
| 001694 | 0x00034860 | 6,266 | discovered | 4 | 001779 | segment-triangle squared distance (6,266 B): parallel and general cases, 001672 and 001690 at the boundaries |

Totals: 13 rows; discovered 12,564 B, dynamically_gated 1,836 B

- **Evidence.** Leaf float code with no strings; the .rdata blocks put 001690 and 001694 in
  different units, and the three empty blocks before 001690 are consistent with one unit per
  kernel. One candidate file is a choice; 001690 stays in `NarrowPhase.cpp` (dynamically_gated
  there) and is called from the new file.
- **Entry rows and callers outside.** 001670 <- 001688; 001684 <- 001688; **001688 <- 001751,
  001753, 001785** (box/capsule and capsule/compound); 001672 <- 001694, 001927 (sphere/mesh
  area), 002029; 001692 <- 001762, 001844, 001859; 001694 <- 001779 (capsule/mesh). 001674..001682
  are register-convention helpers reached only from 001676/001684 and are proved through 001684.
  **Candidate:** no caller exists except through the matrix entries in this contract.
- **x87.** All: `/arch:IA32`, `double` register lifetimes, the listing's grouping (the
  segment/segment notes in `NarrowPhase.cpp` show the pattern: wide operands kept in registers,
  quotients stored narrowed).
- **Test route.** `NxPhysicsCollisionTests` leaf families in the `segment_segment` style (oracle
  row by RVA against the candidate function, random and aimed inputs, both control words, bitwise
  comparison of the squared distance and every output parameter, ceilings where a register-
  lifetime divergence remains): `point_box` (001670), `line_box` (001684: axis-parallel lines hit
  the zero-component arms 001678/001680/001682), `segment_box` (001688), `point_triangle`
  (001672), `line_line` (001692, including parallel lines), `segment_triangle` (001694, parallel,
  edge and interior regions).

### F. Intersection helpers - `Physics/src/Geometry.cpp` (existing)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001696 | 0x000360e0 | 269 | dynamically_gated | 3 | none (table) | NxSeparatingAxis |
| 001698 | 0x000361f0 | 1,037 | dynamically_gated | 3 | continuation | continuation of 001696 |
| 001700 | 0x00036600 | 135 | dynamically_gated | 3 | continuation | continuation of 001696 |
| 001702 | 0x00036690 | 1,308 | dynamically_gated | 3 | 001738, 001791 | NxBoxBoxIntersect |
| 001704 | 0x00036bb0 | 162 | dynamically_gated | 3 | 001261, 001407 | NxRayPlaneIntersect |
| 001706 | 0x00036c60 | 291 | dynamically_gated | 3 | none (table) | NxSegmentPlaneIntersect |
| 001708 | 0x00036d90 | 227 | discovered | 3 | 001822 | ray against a triangle list: per triangle Triangle::Inflate (005185), then NxRayTriIntersect (001712); first hit |
| 001710 | 0x00036e80 | 198 | dynamically_gated | 3 | 001377 | NxRaySphereIntersect |
| 001712 | 0x00036f50 | 782 | dynamically_gated | 3 | 001708 | NxRayTriIntersect |
| 001714 | 0x00037260 | 737 | dynamically_gated | 3 | none (table) | NxSegmentBoxIntersect |
| 001716 | 0x00037550 | 705 | dynamically_gated | 3 | none (table) | NxSegmentOBBIntersect |
| 001718 | 0x00037820 | 645 | dynamically_gated | 3 | none (table) | NxRayOBBIntersect |
| 001720 | 0x00037ab0 | 449 | dynamically_gated | 3 | none (table) | NxSegmentAABBIntersect |
| 001722 | 0x00037c80 | 317 | dynamically_gated | 3 | 000680, 000682, 000688 | NxRayAABBIntersect |
| 001724 | 0x00037dc0 | 165 | dynamically_gated | 3 | continuation | continuation of 001722 |
| 001726 | 0x00037e70 | 317 | dynamically_gated | 3 | 000949 | NxRayAABBIntersect2 |
| 001728 | 0x00037fb0 | 152 | dynamically_gated | 3 | continuation | continuation of 001726 |
| 001730 | 0x00038050 | 61 | discovered | 2 | 000951, 001762 | slab test of a ray against an AABB given as corner pairs: entry and exit parameters, face index (-1 = none) |
| 001732 | 0x00038090 | 296 | discovered | 2 | continuation | continuation of 001730 |
| 001734 | 0x000381c0 | 1,605 | dynamically_gated | 3 | 001010 | NxRayCapsuleIntersect |
| 001736 | 0x00038810 | 434 | dynamically_gated | 3 | none (table) | NxSweptSpheresIntersect |

Totals: 21 rows; dynamically_gated 9,708 B, discovered 584 B

- Eighteen rows are the dynamically_gated Nx* exports already in `Geometry.cpp`. Three are not
  started: **001708** (ray against inflated triangles; <- 001822) and **001730/001732** (the
  slab test; <- 000951, BOX shape slot 7, modelled provisionally in `ObjectModel.cpp`, and 001762).
  Callees: 001712 (in `Geometry.cpp`), vendored `Triangle::Inflate` 005185. `/arch:IA32` (already).
- **Test route.** Leaf families `ray_inflated_tris` (001708) and `aabb_slab` (001730) in
  `NxPhysicsCollisionTests`; wire `ObjectModel.cpp`'s provisional 000951 to call 001730 and keep
  its test green.

### G. Box/box and box/capsule - `Physics/src/ContactGeneration.cpp` and `NarrowPhase.cpp` (existing)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001738 | 0x000389d0 | 192 | dynamically_gated | 3 | none (table) | matrix B [BOX][BOX] |
| 001739 | 0x00038a90 | 258 | dynamically_gated | 3 | 001741 | box/box quad depth |
| 001741 | 0x00038ba0 | 2,953 | dynamically_gated | 3 | 001745 | box/box clipping |
| 001743 | 0x00039730 | 1,240 | dynamically_gated | 3 | continuation | continuation of 001741 |
| 001745 | 0x00039c10 | 4,271 | dynamically_gated | 3 | 001748 | box/box separating-axis search |
| 001748 | 0x0003ace0 | 240 | dynamically_gated | 3 | 001749, 001753 | box/box transpose shim |
| 001749 | 0x0003add0 | 775 | dynamically_gated | 3 | none (table) | matrix A [BOX][BOX] |
| 001751 | 0x0003b0e0 | 370 | discovered | 3 | none (table) | matrix B [BOX][CAPSULE]: capsule segment against the box through 001688 |
| 001753 | 0x0003b260 | 2,267 | discovered | 3 | none (table) | matrix A [BOX][CAPSULE] (2,267 B): 001688, 001748 shim, 001917 sphere-box data, contacts through 000873 |

Totals: 9 rows; dynamically_gated 9,929 B, discovered 2,637 B

- Box/box (001738..001749) is dynamically_gated. **001751** (matrix B [BOX][CAPSULE], index 15)
  goes to `NarrowPhase.cpp` beside the other B entries; **001753** (matrix A [BOX][CAPSULE],
  index 15) to `ContactGeneration.cpp` beside the other A entries. Their unit is its own (block at
  0x10107b38). Callees: 001688 (sub-unit E), 001748 and 001917 (dynamically_gated), 000873,
  004840. 001753 reads one indirect call `[eax+0x14]` at 0x0003b3da: resolve it from the listing
  before writing.
- **Callers.** Only the matrices (002338 writes them; 002348 reads them). **Candidate:**
  `ShapePairFunctionTable` in `PhysicsInternal.cpp` is cleared and no entry is wired; the Phase 3
  harness calls entries directly. Do not wire the table in this plan.
- **Test route.** `contact_box_capsule` (A index 15) and `overlap_box_capsule` (B index 15) in
  `NxPhysicsCollisionTests`, built like `contact_capsule_capsule`: synthetic `NxCollisionShape`
  pairs (random poses, capsules crossing faces, edges and corners, capsule axis parallel to a box
  axis, deep penetration), sink digest and contact count per pair, both control words.

### I. ContactBoxMeshICE.cpp - `Physics/src/ContactBoxMeshICE.cpp` (new; asserted name)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001755 | 0x0003bb40 | 386 | discovered | 3 | 001770 | box/mesh helper: one triangle into the box frame (mesh +0x10 vertices, +0x14 triangles) |
| 001757 | 0x0003bcd0 | 464 | discovered | 3 | none (table) | matrix B [BOX][MESH]: OBBCollider (005067) query, any hit |
| 001758 | 0x0003bea0 | 694 | discovered | 3 | 001772 | box against a convex mesh (001818) or a height-field mesh (001849); called by 001772 |
| 001760 | 0x0003c160 | 222 | discovered | 3 | 001762, 001770, 001779, 001844, 001865, 001929 | triangle plane from three vertices (thiscall, result in this) |
| 001762 | 0x0003c240 | 493 | discovered | 3 | 001772 | box/mesh triangle contact (493 B + continuations 001764/001766/001768); imports NxComputeBoxPoints, NxGetBoxEdges, NxBoxContainsPoint |
| 001764 | 0x0003c430 | 1,258 | discovered | 3 | continuation | continuation of 001762 |
| 001766 | 0x0003c920 | 922 | discovered | 3 | continuation | continuation of 001762 |
| 001768 | 0x0003ccc0 | 1,053 | discovered | 3 | continuation | continuation of 001762 |
| 001770 | 0x0003d0e0 | 1,054 | discovered | 3 | 001772 | box/mesh per-triangle separating-axis test and contact (1,054 B), uses 001755 and 001760 |
| 001772 | 0x0003d500 | 911 | discovered | 3 | none (table) | matrix A [BOX][MESH]; ContactBoxMeshICE.cpp line 1706 "Opcode is not OK." |

Totals: 10 rows; discovered 7,457 B

- **Evidence.** 001772's `__FILE__` and line 1706; 001768's table and constant share its .rdata
  unit; .text contiguity from 001755 to 001772.
- **Entry rows.** 001772 (matrix A [BOX][MESH], index 16) and 001757 (matrix B [BOX][MESH]).
  001760 is also called by 001779, 001844, 001865, 001929 (sphere/mesh, matrix A [SPHERE][MESH]).
- **Callees outside.** 001692 (E), 001730 (F), **001818 (L) and 001849 (M)** through 001758's
  convex and height-field arms, 001855 (N), 000873, **000875** (not started, 915 B), 000505
  (reconstructed), 000929, **002081** (59 B, not started; calls 002144), 002266, 001281,
  OBBCollider 005067 (vendored), and the Foundation imports NxComputeBoxPoints (0x10104180),
  NxGetBoxEdges (0x10104178), NxBoxContainsPoint (0x10104184) and NxComputeBoxVertexNormals (0x1010417c).
- **Test route.** Needs the mesh fixture (`## Mesh fixture`). `contact_box_mesh` (A index 16)
  and `overlap_box_mesh` (B) over the six mesh fixtures, boxes resting on, straddling and inside
  the mesh; the "Opcode is not OK." arm by a model whose collider fails (capture the Foundation
  error report).

### J. Capsule/capsule overlap and capsule/mesh - `NarrowPhase.cpp` and `Physics/src/ContactCapsuleMesh.cpp` (new; name chosen)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001774 | 0x0003d890 | 320 | discovered | 3 | none (table) | matrix B [CAPSULE][CAPSULE]: segment-segment (001690) against the radius sum |
| 001775 | 0x0003d9d0 | 2,463 | dynamically_gated | 3 | none (table) | matrix A [CAPSULE][CAPSULE] |
| 001777 | 0x0003e370 | 447 | discovered | 3 | none (table) | matrix B [CAPSULE][MESH]: LSSCollider (005027) query |
| 001779 | 0x0003e530 | 1,656 | discovered | 3 | none (table) | matrix A [CAPSULE][MESH] (1,656 B + continuations): LSSCollider query, 001694 per triangle, contacts through 000873/000875 |
| 001781 | 0x0003ebb0 | 867 | discovered | 3 | continuation | continuation of 001779 |
| 001783 | 0x0003ef20 | 1,127 | discovered | 3 | continuation | continuation of 001779 |

Totals: 6 rows; discovered 4,417 B, dynamically_gated 2,463 B

- **Evidence.** 001775 (dynamically_gated, in `ContactGeneration.cpp`) opens the unit after
  ContactBoxMeshICE (block 0x10107bd8); 001774 precedes it and 001777..001783 follow it in .text.
- **001774** (matrix B [CAPSULE][CAPSULE], index 21, 320 B, calls 001690) goes to `NarrowPhase.cpp`
  and has no out-of-range dependency. **001777/001779..001783** (matrix B and A [CAPSULE][MESH])
  need 001694 (E), 001760 (I), 000873, **000875**, LSSCollider 005027 (vendored) and the mesh
  fixture.
- **Test route.** `overlap_capsule_capsule` (B index 21) with the Phase 3 capsule generators;
  `contact_capsule_mesh` / `overlap_capsule_mesh` over the mesh fixtures.

### K. Compound entries - `ContactGeneration.cpp` (A) and `NarrowPhase.cpp` (B) (existing)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001785 | 0x0003f390 | 471 | discovered | 3 | none (table) | matrix B [CAPSULE][COMPOUND]: per child, capsule-box through 001688 |
| 001787 | 0x0003f570 | 56 | reconstructed | 3 | none (table) | matrix B [PLANE][COMPOUND] and [MESH][COMPOUND]: AABB refresh, always false |
| 001789 | 0x0003f5b0 | 334 | discovered | 3 | none (table) | matrix B [SPHERE][COMPOUND]: per child, sphere-box (001913) |
| 001791 | 0x0003f700 | 418 | discovered | 3 | none (table) | matrix B [BOX][COMPOUND]: per child, NxBoxBoxIntersect (001702) |
| 001793 | 0x0003f8b0 | 345 | discovered | 3 | 001795 | compound contact expander: child AABB refresh (004886), each child re-dispatched through 002348 (000529) |
| 001795 | 0x0003fa10 | 29 | discovered | 3 | none (table) | matrix A [*][COMPOUND]: swaps the shapes and calls 001793 |
| 001797 | 0x0003fa30 | 77 | discovered | 3 | none (table) | matrix A [COMPOUND][COMPOUND]: child pairs through 002348 |
| 001799 | 0x0003fa80 | 409 | discovered | 3 | continuation | continuation of 001797 |
| 001801 | 0x0003fc20 | 346 | discovered | 3 | continuation | continuation of 001797 |

Totals: 9 rows; discovered 2,429 B, reconstructed 56 B

- **Entry rows.** B: 001785 [CAPSULE][COMPOUND], 001789 [SPHERE][COMPOUND], 001791 [BOX][COMPOUND]
  (001787, the always-false [PLANE]/[MESH][COMPOUND], is reconstructed). A: 001795 (five slots),
  001797 [COMPOUND][COMPOUND].
- **Callees outside.** 001688 (E), 001702 and 001913 (dynamically_gated), 004886, **002348**
  (the pair dispatcher, 719 B, not started), **000529** (134 B + 004153, not started). The A
  entries re-dispatch every child pair through 002348, so they cannot run until the dispatcher
  and the function table exist on the candidate side.
- **Test route.** B entries: `overlap_*_compound` families with a synthetic compound shape whose
  children array (+0xe0 begin, +0xe4 end, read at 0x0003fa43 and 0x0003fa37) points at primitive shapes. A
  entries: only after 002348 is written; then drive 001795/001797 with a compound of primitives
  and compare the sink.

### L. Convex/convex separating axes - `Physics/src/ContactConvexConvex.cpp` (new; name chosen)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001803 | 0x0003fd80 | 152 | discovered | 3 | 001807, 001816 | convex SAT: one axis, both objects' slot 11 (+0x2c) projections, overlap and depth |
| 001805 | 0x0003fe20 | 117 | discovered | 3 | 001809 | convex SAT: one axis against one object's projection (caller cleans 8 B: needs an assembly thunk, phase5 3z214) |
| 001807 | 0x0003fea0 | 301 | discovered | 3 | 001809 | convex SAT: face normals of object A (slot 3 count, slot 4 polygon; plane at polygon+0xc) against B |
| 001809 | 0x0003ffd0 | 432 | discovered | 3 | 001816 | convex SAT: face normals with the separating-distance bookkeeping (13 arguments) |
| 001810 | 0x00040180 | 611 | discovered | 3 | 001812 | box-frame edge axis helper (4x4 pose, register arguments) |
| 001812 | 0x000403f0 | 125 | discovered | 3 | 001816 | convex SAT: edge-edge axes through 001661 (unique-axis Container) |
| 001814 | 0x00040470 | 381 | discovered | 3 | continuation | continuation of 001812 |
| 001816 | 0x000405f0 | 1,490 | discovered | 3 | 001818 | convex/convex separating-axis search (1,490 B, alloca) |
| 001818 | 0x00040bd0 | 1,478 | discovered | 3 | 001758, 001820 | convex/convex contact: 001816, then 001653, contacts through 001909 (1,478 B) |
| 001820 | 0x000411a0 | 442 | discovered | 3 | 001876 | convex/convex entry (called by 001876): poses to 4x4, the TriangleMesh+0x04 interfaces and +0xa4/+0xa8, then 001818 |

Totals: 10 rows; discovered 5,529 B

- **Evidence.** One cluster, entered at 001820 (called by 001876, `ContactMeshMesh.cpp`, when both
  meshes are convex) and at 001818 (also called by 001758). 001816's double 1e-6 opens the unit two
  blocks before ContactConvexHeightfield.cpp's.
- **Callees outside.** 001653, 001661 (D); **001909** (0x00048e30, 733 B, ContactPlaneMesh gap;
  closure 001903, 001907, 000875); 004886; 002266; 001281; the polygon interface slots 2/3/4/11 of
  `TriangleMesh+0x04` (002215, 002221, 002223, **002249**; see `## Shared structures`), which reach
  the support maps (B) and the hull (P-Hull). 001805 is caller-cleans with two 8-byte arguments:
  per `evidence/phase5-object-model.md` 3z214 it needs a hand-written assembly thunk for the
  oracle call in the harness.
- **Test route.** Needs the mesh fixture with convex meshes (the polygon interface and +0xa4/+0xa8
  filled). `contact_convex_convex` through 001820 over pairs of fixture hulls (boxes, prisms,
  a sphere-like hull) in touching, separated and deep configurations; leaf families for 001803/
  001807 on the same objects.

### M. ContactConvexHeightfield.cpp - `Physics/src/ContactConvexHeightfield.cpp` (new; asserted name)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001822 | 0x00041360 | 717 | discovered | 3 | 001844 | convex against a triangle: inflated-triangle ray tests through 001708, Matrix4x4::Invert (005197) |
| 001824 | 0x00041630 | 348 | discovered | 3 | 001826, 001830 | project a mesh vertex subset on an axis, each vertex once (timestamp from 000505) |
| 001826 | 0x00041790 | 166 | discovered | 3 | 001828, 001833, 001836 | axis test: an object's slot 11 projection against 001824's interval |
| 001828 | 0x00041840 | 235 | discovered | 3 | 001832 | face-normal loop (slots 3/4) for the height-field path |
| 001830 | 0x00041930 | 123 | discovered | 3 | 001832 | axis test on a precomputed interval |
| 001832 | 0x000419b0 | 496 | discovered | 3 | 001849 | convex/height-field separating-axis driver |
| 001833 | 0x00041ba0 | 112 | discovered | 3 | 001840 | edge-axis loop helper |
| 001834 | 0x00041c10 | 961 | discovered | 3 | 001840 | edge-edge axes against triangle edges (961 B, alloca; mesh +0x88 built lazily by 002188) |
| 001836 | 0x00041fe0 | 342 | discovered | 3 | 001849 | triangle-normal axes (342 B + continuation 001838) |
| 001838 | 0x00042140 | 790 | discovered | 3 | continuation | continuation of 001836 |
| 001840 | 0x00042460 | 247 | discovered | 3 | 001849 | convex/height-field axis search (calls 001833, 001834) |
| 001842 | 0x00042560 | 623 | discovered | 3 | 001849 | convex/height-field contact reduction and emission through 001909 |
| 001844 | 0x000427d0 | 2,170 | discovered | 3 | 001847 | convex against mesh triangles (2,170 B + continuation 001846, alloca): 001822, 001855, 001692, 001760 |
| 001846 | 0x00043050 | 640 | discovered | 3 | continuation | continuation of 001844 |
| 001847 | 0x000432d0 | 943 | discovered | 3 | 001876 | ContactConvexHeightfield.cpp entry (943 B): line 583, OBBCollider (005067), 001844 |
| 001849 | 0x00043680 | 2,899 | discovered | 3 | 001758, 001851 | ContactConvexHeightfield.cpp main (2,899 B): line 2594, RadixSort, Triangle::Area/Center, 001832/001836/001840/001842, 001653 |
| 001851 | 0x000441e0 | 790 | discovered | 3 | 001853 | convex/height-field entry: poses, CCD guard (002266), 001849 |
| 001853 | 0x00044500 | 5 | discovered | 3 | 001876 | five-byte jmp to 001851 (called by 001876) |

Totals: 18 rows; discovered 12,607 B

- **Evidence.** The asserts (001847 line 583, 001849 line 2594). 001822..001846 lie between the
  convex/convex cluster and 001847 and are called only from 001844 and 001849; they may be the
  head of `ContactConvexHeightfield.cpp` or the tail of the convex/convex file (the .rdata cannot
  separate them: 001838 shares 001816's double). They are assigned here because every caller is
  here.
- **Entry rows.** 001851 (called through the 5-byte jmp 001853 by 001876) and 001847 (called by
  001876); 001849 also by 001758 (box/mesh).
- **Callees outside.** 001653, 001661 (D); 001692 (E); 001708 (F); 001760 (I); 001855 (N);
  **001472** (from 001822) and **001461, 001502** (from 001844) - P-Hull; **002188** (152 B,
  needs the EdgeList closure) from 001834/001849; **002081**; **000875**; 001909; RadixSort,
  Triangle::Area/Center, Matrix4x4::Invert, OBBCollider (vendored); 000505; 002266; 004886.
- **Test route.** Needs the mesh fixture with a height-field mesh (+0x7c axis, +0x80 extent; the
  `NxMesh` height-field fixture) and a convex mesh. `contact_convex_heightfield` through 001851
  (and 001847 for the second entry), over the height field with the hull resting on, crossing and
  below the surface; the "Opcode is not OK." arms by a failing collider.

### N. Mesh/height-field and ContactMeshMesh.cpp's leading rows - `Physics/src/ContactMeshHeightfield.cpp` (new; asserted name) and `Physics/src/ContactMeshMesh.cpp` (new; asserted name)

| row | rva | bytes | state | phase | callers | role |
|---|---|---:|---|---:|---|---|
| 001855 | 0x00044510 | 837 | discovered | 3 | 001762, 001844, 001859 | segment against triangle edges, closest points (837 B); shared by box/mesh, convex/mesh and mesh/height-field |
| 001857 | 0x00044860 | 774 | discovered | 3 | 001859 | triangle-edge contact helper: the {0,2,1} table at 0x10107cd4 and the adjacency words (& 0x1fffffff) |
| 001859 | 0x00044b70 | 2,892 | discovered | 3 | 001861 | mesh/height-field triangle-pair contact (2,892 B): mesh +0x84 state (0, 1, other), 001855, 001857, 001692 |
| 001861 | 0x000456c0 | 538 | discovered | 3 | 001869 | mesh/height-field AABBTreeCollider (004986) pass (538 B + continuation 001863) |
| 001863 | 0x000458e0 | 1,155 | discovered | 3 | continuation | continuation of 001861 |
| 001865 | 0x00045d70 | 1,498 | discovered | 3 | 001869 | mesh/height-field OBBCollider pass (1,498 B + continuation 001867); ContactMeshHeightfield.cpp line 328 |
| 001867 | 0x00046350 | 439 | discovered | 3 | continuation | continuation of 001865 |
| 001869 | 0x00046510 | 64 | discovered | 3 | 001876 | mesh/height-field entry: 001861 then 001865 |
| 001870 | 0x00046550 | 394 | discovered | 3 | none (table) | matrix B [MESH][MESH]: AABBTreeCollider (004986), any pair |
| 001872 | 0x000466e0 | 146 | discovered | 3 | 001874 | stdcall contact accumulator into the globals at 0x10123d8c.. (at most 32) |
| 001874 | 0x00046780 | 801 | discovered | 3 | none (table) | mesh/mesh vertex callback passed by 001876 (pointer at 0x00046d16); two local objects built by 001349 (0x277c0) |

Totals: 11 rows; discovered 9,538 B

- **Evidence.** 001865's `__FILE__` (line 328) and 001859's constant share a unit; 001857 is in the
  unit before it (its own {0,2,1} table) with 001855. 001870..001874 precede 001876 and are used
  only by it (001874 by function pointer); they go to `ContactMeshMesh.cpp` with 001876, which is
  outside this plan.
- **Entry rows.** 001869 (called by 001876); 001855 also by 001762 and 001844; 001870 (matrix B
  [MESH][MESH], index 28); 001872/001874 are callbacks of 001876.
- **Callees outside.** 001692 (E), 001760 (I), **002186** (143 B; its closure is IceAdjacencies
  + EdgeList), **002188**, **002081**, 000443, 000505, 000873, AABBTreeCollider and OBBCollider
  (vendored), 005686 (`_atexit`: a function-static in 001861's area - decode before writing), and
  for 001874 **001351** (33 B, closure 1,282 B) and the reconstructed 001349/001357. Two further
  pointers into these rows need decoding before they are written: 0x000f8100 (CRT area) ->
  0x00044883 inside 001857, and .rdata 0x00101bdc -> 0x00046850 inside 001874 (probably
  exception or unwind scope entries).
- **Test route.** Needs the mesh fixture (two meshes, one a height field). `contact_mesh_heightfield`
  through 001869, `overlap_mesh_mesh` through 001870; 001872/001874 are compared through 001876
  once that row is written (outside this plan) - until then, drive 001872 directly and compare
  the globals block 0x10123d8c.. against the candidate's.

## Mesh fixture

Sub-units I, J (mesh part), L, M and N cannot be driven until each side has a TriangleMesh that
its own rows can read. No mesh object exists on the candidate side (`evidence/phase4-pmap-
reconstruction.md`: none of the `mesh` component's rows is reconstructed), and the oracle's
reader accept arm has never been driven. The recommended route, before the first of those
sub-units:

1. Build each side's image by hand in the harness at the established offsets
   (`## Shared structures`): the primary and secondary vtables (oracle: base+0x108608 and
   base+0x1085d4), InternalTriangleMesh arrays from an `NxMesh` fixture, the OPCODE model at +0x28
   built by that side's own `Model::Build` (as `opcode_model_build` in `NxPhysicsThirdPartyTests`
   already does for both sides), the height-field fields, +0x84/+0x88 as the first sub-unit to
   read them establishes, and for convex meshes the +0xa0..+0xa8 fields and a hull.
2. The candidate side needs the rows the images reach: the polygon interface (002211..002249;
   nine rows, 649 B, not started - P-Mesh), and for convex meshes the support maps (B) and the
   hull (P-Hull).
3. A mesh shape is an `NxCollisionShape` with `geometry` (+0xe0) pointing at that side's image.

This is harness code plus the polygon-interface rows; it is the first step of the first
mesh-dependent task in `## Task split`.

## Out-of-range prerequisites

Rows outside the four units that the product code here calls and that do not exist on the
candidate side. Product code cannot link without them. Each needs a controller decision: adopt
it into the task named, or defer the dependent sub-unit.

| id | rows (not started) | bytes | owner unit | needed by |
|---|---|---:|---|---|
| P-EdgeList | 002054, 002058, 002061, 002063 (+ vendored 005155, 005181) | 3,179 | `EdgeList.cpp` | A (001546), D (001667), and through 002188/002186 M, N |
| P-Hull | 001441, 001445, 001449, 001459, 001463, 001465 (ConvexHull.cpp), 001472, 001496, 001502 | 3,107 | `ConvexHull.cpp` and the gaps either side | B (001567/001569/001573), M (001822; 001844 via 001502). Itself needs A, D (001641, 001651), P-EdgeList and 002144 |
| P-Mesh | TriangleMesh polygon interface 002221, 002223, 002225, 002227, 002229, 002231, 002217, 002219, 002249 | 649 | `TriangleMesh.cpp` span | L, M (slots 2/3/4/11); 002249 needs B |
| P-Emit | 000875 | 915 | `gap:SceneRaycast.cpp..CapsuleShape.cpp` | I, J, M, and 001909 |
| P-Plane | 001909, 001903, 001907 | 1,385 | `gap:ContactPlaneMesh.cpp..PenetrationMap.cpp` | L, M |
| P-Small | 002081, 002144 (276), 002188 (152, needs P-EdgeList), 002186 (143, needs A and P-EdgeList), 001461 (194, needs 001651) | 765 | TriangleMesh spans, SphereShape..ConvexHull gap | D (001651), I, M, N |
| P-Dispatch | 002348 (719, phase 2), 000529 (134, phase 7), 004153 (156, phase 6) | 1,009 | `gap:Controller.cpp..fluids\Fluid.cpp`, `Scene.cpp` area | K (A entries) |
| P-Sphere | 001351 (33 B) and its closure: 000517, 000887, 000903, 000915, 001323, 001945, 001955, 002354, 002413, 004157, 004859 | 1,315 | phases 3 and 5 | N (001874) |

## Task split

Ordered, dependencies first; bytes are the not-started bytes the task writes (in range, plus any
prerequisite adopted). The template is the plan's Task 2. Each task names the sub-unit sections
above for its rows, callers, structures and test route.

| task | rows | in-range bytes | prerequisite | test family (new) |
|---|---|---:|---|---|
| 2a | E box distance (001670, 001674..001688) + G box/capsule (001751, 001753) + J 001774 | 7,245 | none | point_box, line_box, segment_box, contact_box_capsule, overlap_box_capsule, overlap_capsule_capsule |
| 2b | E triangle distance (001672, 001692, 001694) + F (001708, 001730/001732) | 8,860 | none | point_triangle, line_line, segment_triangle, ray_inflated_tris, aabb_slab |
| 2c | A IceAdjacencies.cpp (incl. 001537) + D valencies (001667) | 2,072 | P-EdgeList (3,179) | ice_adjacencies, ice_valencies, edge_list |
| 2d | C MeshBuilder2 + D vertex reduction (001647) | 10,728 | none | ice_meshbuilder2, vertex_reduction |
| 2e | D remainder (001639, 001641/001643, 001651, 001653, 001661) | 3,664 | P-Small 002144 | pose_pair, unique_axis, edge_dedupe, mesh_normals |
| 2f | B support maps | 1,889 | P-Hull (3,107) for the product rows; the differential runs on filled hulls | support_maps |
| 2g | Mesh fixture + L convex/convex | 5,529 | P-Mesh, P-Plane, P-Emit | contact_convex_convex |
| 2h | M first half: 001822..001842 | 5,160 | P-Hull (001472), 002188 | (leaf families on the fixture) |
| 2i | M second half: 001844..001853 (incl. the ContactConvexHeightfield.cpp unit) | 7,447 | 001461, 001502, 002081 | contact_convex_heightfield |
| 2j | I ContactBoxMeshICE.cpp | 7,457 | (2g..2i done) | contact_box_mesh, overlap_box_mesh |
| 2k | J capsule/mesh (001777..001783) | 4,097 | - | contact_capsule_mesh, overlap_capsule_mesh |
| 2l | N (001855..001874) | 9,538 | 002186, P-Sphere | contact_mesh_heightfield, overlap_mesh_mesh |
| 2m | K compound | 2,429 | P-Dispatch (A entries only; B entries need none) | overlap_*_compound, contact_compound |

Every new file except `IceAdjacencies.cpp` holds float code and goes on the `/arch:IA32` list in
`CMakeLists.txt` (the existing `ContactGeneration.cpp`, `NarrowPhase.cpp` and `Geometry.cpp` are
already on it). No task exceeds 12 KB with the prerequisites it adopts: the largest are 2d
(10,728 B) and 2g (8,478 B with P-Mesh, P-Plane and P-Emit).

In-range total 76,115 B. Tasks 2a, 2b and 2d need nothing outside the range and can start at
once; 2c needs only P-EdgeList, and 2e only 002144. Everything mesh-shaped (2f..2l) is blocked on
the mesh fixture and on P-Hull/P-Mesh; if the controller defers those prerequisites, stop after
2e plus 2m's three B entries (001785, 001789, 001791), which is about 34 KB written. 2m's A
entries wait for P-Dispatch in every case.

## Open items

1. `tools/work_units.py` misses three translation units whose Ghidra string entries start early
   (ContactBoxMeshICE.cpp, ContactMeshHeightfield.cpp, core\Articulation.cpp). Fixing it
   renames `gap:IceAdjacencies.cpp..ContactConvexHeightfield.cpp` and
   `gap:ContactConvexHeightfield.cpp..ContactMeshMesh.cpp` in `work_units.json` and their
   bundles; Task 3 regenerates both, so the fix belongs before or in Task 3.
2. 001753's indirect call `[eax+0x14]` (0x0003b3da) and 001779's (0x0003e6c8): resolve the
   receiver from the listing.
3. TriangleMesh +0x84, +0x88, +0xa4, +0xa8: only the use sites above are known, not what they hold.
4. The pointers 0x000f8100 -> 001857+0x23 and 0x00101bdc -> 001874+0xd0.
5. Which file holds the support-map classes (IceAdjacencies.cpp or a neighbour) and whether
   001822..001846 open ContactConvexHeightfield.cpp or close the convex/convex file.
