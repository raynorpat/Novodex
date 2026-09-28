# Convex/mesh gap

Measurement note for the IceAdjacencies..ContactConvexHeightfield gap plan
(`docs/superpowers/plans/2026-09-28-convex-mesh-gap.md`), which reconstructs the not-started code
of `gap:IceAdjacencies.cpp..ContactConvexHeightfield.cpp`, `IceAdjacencies.cpp`,
`ContactConvexHeightfield.cpp` and `gap:ContactConvexHeightfield.cpp..ContactMeshMesh.cpp`, split
into the sub-units of `units/convex-mesh-gap-contract.md`. Each task appends one row to the timing
table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T08:04:40 | 2026-09-28T08:35:41 | 0 | 0 | Survey and contract (`units/convex-mesh-gap-contract.md`); no row written or moved. Bundles generated for the four units; `DecompileSupplement.java` run headless over the union of the 34 earlier RVAs and the 43 rows Ghidra never made functions (77 requested, 77 `ok`, the 34 earlier entries byte-identical, a second run byte-identical), so every one of the 174 rows has a decompile. 127 not-started rows, 76,115 B (with 001537, AddTriangle, adopted from the ConvexHull.cpp gap), split into sub-units A..N and tasks 2a..2m, none above 12 KB. Found: two translation units the census missed (`ContactBoxMeshICE.cpp`, 001772 line 1706; `ContactMeshHeightfield.cpp`, 001865 line 328; `core\Articulation.cpp` elsewhere) because `work_units.py` keys file strings by Ghidra entries that start early; the per-unit NxMath pi block in .rdata as a translation-unit boundary marker; TriangleMesh +0x04 is a second vtable (0x101085d4, 12-slot polygon interface, slot 11 = 002249); no row has vendored ICE source (Adjacencies, MeshBuilder2, Valencies, support maps are full-ICE classes), so `vendored_match.py` does not apply; about 12 KB of out-of-range prerequisites (EdgeList.cpp, the ConvexHull.cpp hull, the TriangleMesh polygon interface, 000875, 001909, the dispatcher 002348) gate the mesh-shaped sub-units. Tool tests (753) pass; validator exits 0. No product, test or gate file changed, so gates 2-7 are unaffected and were not run. |
| 2a | 2026-09-28T08:51:30 | 2026-09-28T09:40:40 | 15 | 8,468 | Box distance kernels (001670, 001674..001688 incl. 001686) in the new `Physics/src/Distance.cpp`; matrix B 001751, 001774, 001785, 001789, 001791 and matrix A 001753. All exact against the oracle under both control words (point_box, line_box, segment_box, box_capsule, capsule_capsule, contact_box_capsule, three compound families; 23 lines registered, phase 3 floor 126). Defects: Face as one function spilled wide values (16 segment_box words under 0x0f7f; fixed by splitting its leaves); the contract's compound entries test the compound's own world bounds, not children (corrected); 001753's indirect call is the box's slot 5 (000949); Case00's pnt[i1] typo reproduced; the oracle's 16-slot manifold overflow in 001753 pre-flighted (21 pairs skipped). All 24 traced functions hit. See `## Task 2a`. |
| 2b | 2026-09-28T09:53:44 | 2026-09-28T11:01:37 | 8 | 9,919 | Triangle distance kernels 001672, 001692, 001694 in `Physics/src/Distance.cpp`; 001708 and 001730/001732 in `Geometry.cpp`; 001760 in the new `ContactBoxMeshICE.cpp` and 001855 in the new `ContactMeshHeightfield.cpp`. BOX slot 7 (000951) written from its listing and wired to 001730 (still `discovered`). All exact under 0x027f (point_triangle, line_line, segment_triangle, ray_inflated_tris, aabb_slab, triangle_plane, segment_triangle_edges; 14 lines registered, phase 3 floor 140); under 0x0f7f 001694 differs on 14 words inherited from 001690, 001760 on 51 and 001855 on 32 from the square root's qword operand. ray_inflated_tris gates the fans whose two Triangle::Inflates (005185) agree, the rest under ceilings. Defects: 001672 and 001694 spilled wide values until split into leaves; a Triangle object gave 001708 an unwind frame; contract roles of 001692/001708/001855 corrected; the harness's generators shift oracle digests when its inlining changes. All 34 traced functions hit. 2a review minors fixed in their own commit. See `## Task 2b`. |

## Task 2a: box distance kernels and the entries that reach them

**Rows (15, 8,468 B), all `discovered` -> `reconstructed`.** Sub-unit E's box half in the new
`Physics/src/Distance.cpp` (on the `/arch:IA32` list): 001670 point/box, 001684 line/box with its
continuation 001686, the five register-convention helpers 001674 Face, 001676 CaseNoZeros, 001678
Case0, 001680 Case00, 001682 Case000, and 001688 segment/box (4,288 B). Matrix B [BOX][CAPSULE]
001751, [CAPSULE][CAPSULE] 001774 and the compound entries 001785, 001789, 001791 in
`NarrowPhase.cpp` (1,913 B); matrix A [BOX][CAPSULE] 001753 in `ContactGeneration.cpp` (2,267 B).
No candidate stand-in existed for any of them, and no candidate caller: the entries are reached only
through the matrices, which this plan does not wire (contract, sub-unit G), so the only wiring is
entry -> kernel inside the new code. Static proofs cite the listing ranges; each row's
`dynamic_proof` cites the trace below and its family.

**Differential** (`NxPhysicsCollisionTests`, oracle rows by RVA against the linked candidate, both
control words, oracle digests registered):

| family | rows | checks | 0x027f | 0x0f7f |
|---|---|---:|---:|---:|
| box_capsule.random / .aimed | 001751 (and 001688, 001913) | 120,000 + 120,000 | 0 | 0 |
| capsule_capsule.random / .aimed | 001774 (and 001690) | 120,000 + 120,000 | 0 | 0 |
| point_box | 001670 | 2,640,000 | 0 | 0 |
| line_box | 001684/001686, 001674..001682 | 3,120,000 | 0 | 0 |
| segment_box | 001688 | 3,120,000 | 0 | 0 |
| contact_box_capsule | 001753 | 6,532,012 | 0 | 0 |
| sphere_compound / box_compound / capsule_compound | 001789 / 001791 / 001785 | 600,000 each | 0 | 0 |

Every family is exact under both words, so nothing is registered as divergent and no ceiling was
needed. 23 oracle-side lines were registered (the phase 3 coverage floor goes from 103 to 126).

**Defects found.**
- *Candidate, register lifetime:* Face written as one function differed on 16 segment_box words
  under 0x0f7f (none under 0x027f): MSVC spilled the reciprocal `-1/(d^2 + lSqr)` and the sum under
  it to 8-byte slots across Face's branches, and in the edge and corner leaves the squared distance
  is a sum that cancels to ~1e-14 when the segment grazes an edge, so the 53-bit spill moved its last
  float bits. The phys_fn_001690 class -- but here removable: Face's ten leaves are now small
  functions entered only with values the listing has already narrowed, each forming its wide values
  locally (the single-slab arms recompute the wide `tmp` from the same floats rather than receive it),
  and the count went to 0. line_box had not shown it because its aimed lines rarely lie in a face
  plane; segment_box's endpoints on faces and edges do.
- *Contract:* the three matrix B compound entries do not walk children. Each tests its primitive
  against the compound shape's own world bounds through the `Prunable` at Shape+0xa4 (flags +0xac,
  pruner +0xc4, handle +0xcc; `GetUpdatedWorldAABB` inlined with phys_fn_004886 called), and 001791
  is false unless `[box+0xde] & 7`. The contract's K section, `## Shared structures` and open item 2
  are corrected in this commit; 001753's `[eax+0x14]` (open item 2) is the BOX's vtable slot 5,
  phys_fn_000949.
- *Oracle, reproduced:* 001680 (Case00) stores the low clamp of axis i2 into pnt[i1]
  (0x0003393a), the Magic Software typo, and the source keeps it.
- *Oracle, not driven:* 001753's crossing branch writes the phys_fn_001748 manifold into the same
  sixteen slots phys_fn_001749 uses, the seventeenth being its return address. The first run of the
  family returned the oracle into a contact coordinate and took the harness down; it now uses
  contact_box_box's pre-flight (the oracle's own 001748 with 80 slots and a copy of the cache byte),
  which measured `probe_max=18` and skipped 21 pairs (`overflow_skipped=21`, registered).
  For a pair whose manifold has more than sixteen contacts the candidate does not reproduce the
  overrun: `NxContactBoxCapsule` gives the manifold 80 slots and emits every contact, where the
  oracle writes the seventeenth over its own return address. That is a deliberate, documented
  divergence, and the pre-flight is what keeps those pairs out of the comparison (the count is the
  same, 21, under both control words; Task 2b made the harness count and check both).

**Harness notes.** The box's slot 5 is phys_fn_000949, a Phase 5 row this task does not own and whose
candidate is provisional, so both worlds of contact_box_capsule carry the ORACLE's 000949 in the box
vtable: the dispatch is data the shape carries, and what is compared is 001753. The compound
families require the oracle's owner callback at .data 0x00128478 to be null (registered), which is
also the candidate's state. `tools/tests/test_gate_targets.py` read the collision, fuzz, asset and
third-party harness sources from absolute paths into the main checkout, so in a worktree it checked
this tree's registry against another tree's harness; it now reads the harness of its own tree.

**Trace.** `evidence/convex-mesh-gap-trace-2a.txt`: one-shot cdb breakpoints on the 14 candidate
functions and Face's 10 leaves in `NxPhysicsCollisionTests.exe` (sha256 4442c476..., addresses from
its linker map, `/MAP` added for this), all 24 hit in one full run that ended `collision=pass`.
001686 has no address of its own (it is the second half of `NxLineBoxSquareDistance`).

**Ledger.** The six phase 3 rows leave `blocked_on_later_phase` (001751, 001753) and
`not_reconstructed_in_phase` (001774, 001785, 001789, 001791), the nine phase 4 rows
`not_reconstructed_in_phase`; all fifteen defer `reconstructed_not_falsified` with the standard note
(no mutation was aimed at them).

**Verification.** Build (Release, Win32) clean; gates 2, 3, 4, 6 and 7 pass; phase 5 fails only on
`candidate CANDIDATE-MISSING family=vtables`; 753 tool tests pass; `validate_inventory.py` exits 0.

## Task 2b: triangle distance kernels and the shared helpers

**Rows (8, 9,919 B), all `discovered` -> `reconstructed`.** Sub-unit E's triangle half in
`Physics/src/Distance.cpp`: 001672 point/triangle (1,326 B), 001692 line/line closest points (684 B)
and 001694 segment/triangle (6,266 B). Sub-unit F in `Physics/src/Geometry.cpp`: 001708, a ray against
an inflated triangle fan (227 B), and the slab test 001730 with its continuation 001732 (357 B).
001760, the out-of-line `NxPlane::set` (222 B), in the new `Physics/src/ContactBoxMeshICE.cpp`
(sub-unit I), and 001855, a segment against one triangle edge (837 B), in the new
`Physics/src/ContactMeshHeightfield.cpp` (sub-unit N; the contract's placement choice, open item 5).
Both new files are on the `/arch:IA32` list and in `NxPhysicsCollisionTests`. Static proofs cite the
listing ranges; each row's `dynamic_proof` cites the trace below and its family.

**Wiring.** The only existing candidate caller is BOX slot 7, phys_fn_000951, whose provisional
model (min over axes of |col_k . H| / |swept[k]|) is not what the listing computes. Its body is now
written from the listing (0x00020b20..0x00020d18): the box's own AABB, the translation taken into the
box frame and back as the ray origin (a rounding residue), R^T swept as the direction, 001730, and
|tFar| written on a hit. `NxPhysicsShapeVtableTests` (whose slot 7 cases compare it against the
oracle's 000951) passes. 000951 stays `discovered`: it is a Phase 5 row, compiled without
`/arch:IA32`, and this task does not own it.

**Differential** (`NxPhysicsCollisionTests`, oracle rows by RVA against the linked candidate, both
control words, oracle digests registered):

| family | rows | checks | 0x027f | 0x0f7f |
|---|---|---:|---:|---:|
| point_triangle | 001672 | 2,160,000 | 0 | 0 |
| line_line | 001692 | 2,880,000 | 0 | 0 |
| segment_triangle | 001694 (and 001690, 001672) | 2,640,000 | 0 | 14 |
| ray_inflated_tris | 001708 (and 005185, 001712) | 600,000 | 0 | 0 |
| aabb_slab | 001730/001732 | 1,080,000 | 0 | 0 |
| triangle_plane | 001760 | 1,920,000 | 0 | 51 |
| segment_triangle_edges | 001855 | 2,040,000 | 0 | 32 |

Everything is exact under the CRT word. The 0x0f7f counts are registered in each family's coverage
line (so a change fails the gate), as for phys_fn_001690:
- *segment_triangle, 14:* all inherited from phys_fn_001690's own 0x0f7f divergence. Measured by a
  throwaway build that bound the oracle's 001690 into the candidate's 001694: 0. 001694's own code is
  exact under both words.
- *triangle_plane, 51, and segment_triangle_edges, 32:* the square roots. Each row takes `fsqrt` of a
  sum over a wide operand inline; the reconstruction reaches `fsqrt` through `X87Sqrt.h`, whose
  `double` arguments travel as qwords, so the wide operand is cut from 64 to 53 bits on the way in, and
  MSVC keeps the same values in 8-byte slots across the call. Only raw draws of extreme magnitude move.
  (001855 written as leaves that recompute the wide values after the call measured 99-100 instead of
  32, because MSVC folds the recomputation into the spilled copy; the single function is kept.)
- *ray_inflated_tris:* 001708 calls Triangle::Inflate (phys_fn_005185), a vendored row held at
  `discovered` because its candidate differs from the oracle's in the last bits
  (evidence/vendored-correspondence.md). Each side reaches its own, so the family pre-flights every
  fan through both Inflates: the 38,982 fans on which they agree are compared exactly and gate (0 and
  0); the 21,018 on which they differ are counted apart (914 and 981 differing words) under ceilings
  the harness enforces (`kInflateDivergentFanCeiling`, `kInflateDivergentWordCeiling`).

14 oracle-side lines were registered; the phase 3 coverage floor goes from 126 to 140.

**Coverage.** point_triangle reaches every leaf (vertex0/1/2, both edges, the open regions, and the
determinant-zero FLT_MAX interior, 4,990); segment_triangle reaches the parallel branch and r at the
start, the end and between (26,553 / 8,442 / 16,914); aabb_slab every face and the miss, with 22,419
direction components inside (-FLT_EPSILON, FLT_EPSILON) and 11,103 exactly on it;
segment_triangle_edges every exit (one side or parallel 7,574, t < 0 13,139, outside the edge 32,830,
on the edge 6,457).

**Defects found.**
- *Candidate, register lifetime:* 001672 written as one function differed on 2,483 words under
  0x0f7f (the first edge, c and t spilled to qwords); written as the listing's leaves, forming c and t
  from the stored floats where they are used, 0. 001694 with its setup and interior in one function
  each differed on 84 words of its own; with the determinant, the solve and s/t split into leaves and
  the interior reading local copies, 0 of its own.
- *Candidate, unwind frame:* 001708 first held its stack triangle as an `IceMaths::Triangle`, whose
  declared destructor gave the function an EH frame the oracle's does not have; it now inflates a plain
  36-byte block through a cast, as the oracle calls Inflate on its stack block.
- *Contract:* 001708 is a triangle FAN (hub `indices[0]`, count - 2 triangles; a count below 2 is an
  unsigned loop in the oracle), 001692 clamps both parameters to [0, 1], and 001855 tests one edge
  (bool, t and a point out), not "edges, closest points". The contract rows are corrected in this
  commit, with the 000951 wiring under sub-unit F's test route.
- *Oracle, reproduced:* 001694 re-forms the segment's far end as p0 + (p1 - p0) rather than reading
  p1, and builds its third edge segment with z's difference narrowed first and x's and y's kept wide;
  001692 tests t wide against 0 and narrowed against 1; 001855 writes t and the point on some false
  returns.

**Harness notes.** 001760 is `__thiscall` (`ret 0xc`); both sides are called as `__fastcall` with an
unused edx, the contract ContactGeneration.h already uses for 000873. Two things about the harness
itself were found on the way. Including Opcode.h in PhysicsCollisionTests.cpp (for the candidate's
Triangle::Inflate) moved the registered oracle digests of box_corner and box_quad_depth, and adding
this task's families to wmain moved those of the three compound families: in both cases the
compiler inlined the harness's generators differently, and whether nxPick's float comes back
through st(0) -- quieting a signalling NaN it drew -- decides what the oracle is handed. The
candidate's Inflate is therefore reached through tests/PhysicsCollisionInflate.cpp, and the families
run in a function of their own (nxDriveTask2b); every pre-existing registered line is reproduced.
The fragility is the generator's, not this task's, and is recorded in the report. The 2a review minors were fixed
in their own commit: contact_box_capsule now counts `overflow_skipped` under both words (21 and 21,
the registered line unchanged), and the note above says what the candidate does for a >16-contact
manifold.

**Trace.** `evidence/convex-mesh-gap-trace-2b.txt`: one-shot cdb breakpoints on the seven candidate
functions and the 27 leaves 001672 and 001694 are split into, in `NxPhysicsCollisionTests.exe`
(sha256 af838ebe..., addresses from its linker map); all 34 hit in one full run that ended
`collision=pass`. 001732 has no address of its own (it is the loop of `NxRayAABBSlab`).

**Ledger.** Phase 2: 001672, 001730, 001732 leave `homeless_shared_code`; phase 3: 001692, 001708,
001760, 001855 leave `not_reconstructed_in_phase`; phase 4: 001694 leaves `not_reconstructed_in_phase`.
All eight defer `reconstructed_not_falsified` with the standard note (no mutation was aimed at them);
counts and reason texts updated.

**Verification.** Build (Release, Win32) clean; gates 2, 3, 4, 6 and 7 pass; phase 5 fails only on
`candidate CANDIDATE-MISSING family=vtables`; 753 tool tests pass; `validate_inventory.py` exits 0.
