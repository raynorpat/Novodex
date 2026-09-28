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
| 2b review | 2026-09-28T11:18:00 | 2026-09-28T11:32:34 | 0 | 0 | No row added; 001760 (x87 block), 001672 (first-edge leaf) and the Task 2b families (bit-written raw draws, mixed exponents, 001712 pre-flight) reworked after the review. Under 0x0f7f: 001672, 001692, 001708, 001730, 001760 exact; 001694 31 on these draws (all 001690's; its own interior leaf differed on 2 of the review's 1M draws, which these did not reach -- see the harness hardening), 001855 215, pinned. The 14 Task 2b lines re-registered; all 122 collision lines reproduce. 35 traced functions hit. See `## Task 2b`, **Review**. |
| H | 2026-09-28T11:36:00 | 2026-09-28T12:58:35 | 1 | 782 | Harness hardening. Every raw draw of the collision harness is written as bits (no float return, which quieted signalling NaNs depending on inlining); pre-Task-2b families draw their NaNs quiet explicitly (all their digests reproduce); handed signalling NaNs, 15 of their blocks' candidates differ (recorded, not registered). 58 input-digest lines registered. 001712 rewritten as a naked x87 transcription (its C++ loaded operands the listing uses from memory: 1,310 step_ray_tri words and 75 / 31 fans on signalling NaNs; now 0), step_ray_tri on signalling NaNs. 001694's interior leaf an x87 block (2 own 0x0f7f words on the review's draws; now 0); segment_triangle replays the review's 250,000 draws (0x0f7f 940, all 001690's). Compound raw bounds drawn directly. 11 lines re-registered once, 59 added; phase 3 floor 199. 12 traced functions hit. Follow-up (controller decision): the 15 blocks, and 3 kernel-fuzz exports replayed in-process, also run as `.snan` variants divergent under enforced ceilings (44 lines, floor 243); the fuzz harness writes bits and quiets explicitly (its lines unchanged); two unsequenced draws sequenced; the affected rows' notes say signalling-NaN propagation is not reproduced. See `## Harness hardening`. |
| H review | 2026-09-28T13:00:00 | 2026-09-28T13:45:53 | 0 | 0 | Task H review fixes: the separating-axis byte of contact_box_box and contact_box_capsule tallied into their `.snan` ceilings (346 / 346, 78 / 78, 236 / 236 and 99 / 93, 7 / 7, 40 / 40); every remaining unsequenced multi-draw expression in the collision, fuzz, third-party and tangent harnesses sequenced in the order the build used (all registered lines and the geometry transcript unchanged; one site was missed, segment_segment's `scale`, sequenced by Task 2c -- see the correction under `## Harness hardening`); float-returning raw-bit helpers removed from all harnesses and the tool test widened to every tests/*.cpp; ray_inflated_tris' divergent cause no longer names 001712. See `## Harness hardening`. |
| 2c | 2026-09-28T13:48:00 | 2026-09-28T14:38:42 | 12 | 5,564 | EdgeList (002054 with continuation 002056, 002058, 002061, 002063) in the new `EdgeList.cpp`, IceAdjacencies (001537..001548) in the new `IceAdjacencies.cpp`, 001667 in the new `IceMeshTools.cpp`; product forms of 002052, 002060, 001544, 001663, 001665. Allocations through the 004803 getter with the listing's cookies; reports through the SetIceError seam; 002061's plane side and angle as x87 blocks; the three files `/EHs-c-`. Families edge_list, ice_adjacencies, ice_valencies in NxPhysicsThirdPartyTests (object image, allocations and reports compared): exact on every gated run; the vertex runs of 13 meshes whose decisions follow the vendored Plane::Set / Triangle::Normal split into `.plane_divergent` under ceilings (465 / 124 words; 0 with the oracle's callees bound in). 14 lines registered, phase 4 floor 175. 15 traced functions hit. Also: segment_segment's scale draws sequenced (stdout identical) and the raw-bit float-return scan widened (.c files, casts, unions, conventions). See `## Task 2c`. |
| 2c review | 2026-09-28T14:38:42 | 2026-09-28T14:58:32 | 0 | 0 | Task 2c review: the edge_list / ice_adjacencies plane-divergent split frozen as a list of 13 meshes (the candidate pre-flight kept as a failing check, detail on stderr); every digest unchanged, the coverage line `pairs=1487 side=40 angle=9 meshes=13` replaced by `frozen_meshes=13`; the throwaway callee binding committed as `convex-mesh-gap-2c-bind-oracle-callees.patch` (re-run: 465/124, 306/100, 342/92, 0/0); the 002160 product shim recorded (evidence, contract, row notes); the 001539 errata extended to the no-face-array case. See `## Task 2c`. |
| 2d | 2026-09-28T15:01:00 | 2026-09-28T15:41:14 | 26 | 10,728 | MeshBuilder2 (sub-unit C, 001591..001637, 25 rows incl. seven continuations) in the new `IceMeshBuilder2.cpp` and the vertex reduction 001647 in `IceMeshTools.cpp`; product forms of 001645 / 001659. Heap checked first: 005700/005701 are `jmp`s to 005668/005702, the same static-CRT heap as 001514's pair (contract Open item 7 closed); the candidate uses its CRT's nothrow `operator new` / `free` with the listing's cookies; the reduction uses the 004803 getter. x87 sections as assembly blocks; the FPU sequences of 001597, 001603, 001607, 001627 equal the listing's (45/62/6/110). Families ice_meshbuilder2 (249 cases over 68 meshes, eight create-block configurations, both control words) and vertex_reduction: exact (178,377 and 24,313 words). 8 lines registered, phase 4 floor 183. 21 traced functions hit. Contract corrected: 001627 runs once per run of faces, not per face. See `## Task 2d`. |
| 2d review | 2026-09-28T15:44:00 | 2026-09-28T16:09:23 | 0 | 0 | Task 2d review: the uvw and colour streams written as bits (signalling and quiet NaNs, infinities, denormals; 510 / 347 signalling-NaN input words), so the fld/fstp pass-through of 001607 / 001627 is exercised -- still exact, and an integer copy there gives 276 mismatches (patch committed); the ice_meshbuilder2 input, exact, coverage lines and the totals pair re-registered (before/after under `## Task 2d`); 001591 given the oracle's register ABI (__fastcall, `ret 4`); 001647's /GS cookie recorded; trace re-recorded (21 hit). See `## Task 2d`. |
| 2e | 2026-09-28T16:14:00 | 2026-09-28T16:52:07 | 10 | 4,370 | The rest of sub-unit D (001639, 001641/001643, 001651, 001653, 001661 in `IceMeshTools.cpp`) and P-Small (002144 in `SmoothNormals.cpp`, 001461 in the new `ConvexHull.cpp`, 002186/002188 in the new `TriangleMeshTopology.cpp`); product forms of 001536 and 001649. 001651, 001653, 001661 and 002144 are naked listing transcriptions whose built code equals the listing instruction for instruction; `angleAtVertex` now calls 002144 (every collision line unchanged). Families pose_pair, unique_axis, edge_dedupe, mesh_normals, adjacency_owner in NxPhysicsThirdPartyTests, all exact with signalling NaNs in the inputs; two fixed-input splits under ceilings (944 NaN words that follow the vendored InvertPRMatrix's quieting of signalling NaNs; 155 on Task 2c's frozen meshes), both 0 with the oracle's callees bound in. One throwaway mutation per x87 row detected (the 001653 one only after cancelling pose kinds were added). 19 lines registered, phase 4 floor 202. 11 traced functions hit. Contract: 001639 is a 3x3 and a 4x4 identity; 001641 chains an outline; TriangleMesh +0x84/+0x88 are the Adjacencies and the EdgeList. See `## Task 2e`. |
| 2e review | 2026-09-28T16:58:00 | 2026-09-28T17:21:44 | 0 | 0 | Task 2e review cleanups: the four caught mutations re-measured in a `git archive` copy between un-mutated controls (oracle digest a1b4f9a6 unmoved) and cited in the ledger notes of 001661, 001651, 001653 and 002144, which stay reconstructed_not_falsified because a closure needs `dynamically_gated`; the listing comparison now checks absolute operands by their bytes (still 0 differing); the pose_pair split classified over all 944 words (276 quieted signalling NaNs, 668 quiet NaNs differing downstream); the UB of calling Init on a possibly null object noted at 002186/002188; stale contract states updated. See `## Task 2e`, **Review**. |
| 2f | 2026-09-28T17:22:00 | 2026-09-28T18:38:20 | 25 | 6,875 | P-Hull (001441, 001445, 001449, 001459, 001463, 001465, 001472, 001496 with continuations 001498/001500, 001502 with continuations 001504..001512 -- seven continuations the contract's list lacked) in `ConvexHull.cpp` and sub-unit B (001550, 001556, 001558/001560, 001567, 001569, 001573, 001579, 001581) in the new `IceSupportMaps.cpp`; product forms of the eleven small support-map rows, of 000925, 001391, 001439, 001657, and 000001 (not promoted). The twelve x87 rows are naked listing transcriptions, instruction-equal to the listing (vendored members through /alternatename, 001558's switch table row-relative in .rdata). The 001407 model now calls 001556 and 001472 (shape vtable 626/0 unchanged). Families hull_leaf, convex_hull, support_maps in NxPhysicsThirdPartyTests, all exact with signalling NaNs in the inputs; convex_hull.plane_divergent (a fixed-input rule: concave, nudged and 0x0f7f polycubes, drawn 001463 points) under a ceiling of 1,595 words, 0 with the oracle's 005155/005179 bound into 001463. All twelve x87 mutations caught. Two oracle out-of-bounds reads found and not driven (001641 on an empty Container, 001502 without polygons). 12 lines registered, phase 4 floor 214. 32 of 33 traced functions hit. See `## Task 2f`. |

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
control words, oracle digests registered). As registered after the Task 2b review (see **Review**
below): every family draws a third raw words (`nxPickBits`, written as bits), a third aimed and a
third of mixed exponents (`nxMixedBits`, magnitudes 2^-27..2^72).

| family | rows | checks | 0x027f | 0x0f7f |
|---|---|---:|---:|---:|
| point_triangle | 001672 | 2,160,000 | 0 | 0 |
| line_line | 001692 | 2,880,000 | 0 | 0 |
| segment_triangle | 001694 (and 001690, 001672) | 2,640,000 | 0 | 31 |
| ray_inflated_tris | 001708 (and 005185, 001712) | 600,000 | 0 | 0 |
| aabb_slab | 001730/001732 | 1,080,000 | 0 | 0 |
| triangle_plane | 001760 | 1,920,000 | 0 | 0 |
| segment_triangle_edges | 001855 | 2,040,000 | 0 | 215 |

Everything is exact under the CRT word. The 0x0f7f counts are registered in each family's coverage
line (so a change fails the gate), as for phys_fn_001690:
- *segment_triangle, 31:* all inherited from phys_fn_001690's own 0x0f7f divergence. Measured on the
  same draws by a throwaway build that bound the oracle's 001690 into the candidate's 001694: 0.
  001694's own code (and the 001672 it calls) is exact under both words ON THESE DRAWS. Not in
  general: the C++ interior leaf spilled s to an 8-byte slot, and on the review's 1M draws that
  differed on 2 words under 0x0f7f, none of them among these; fixed and re-measured in the harness
  hardening (below), where the family now replays those draws (940 words, all 001690's).
- *segment_triangle_edges, 215:* the wide intermediates themselves -- the normal's x and z and the
  direction's x and z, which the listing keeps in st(n) -- are cut to 53 bits twice: as the operands
  of each square root, which reach `X87Sqrt.h` through qwords, and in their reuse after the root,
  where MSVC reloads them from 8-byte slots to scale them by 1/root. (Written as leaves that
  recompute them after the call, the count rose, because MSVC folds the recomputation into the
  spilled copy.) The x87-block remedy used for 001760 would be two blocks of about 131 instructions
  here and is not judged proportionate; the count is pinned.
- *ray_inflated_tris:* 001708 calls Triangle::Inflate (phys_fn_005185), a vendored row held at
  `discovered` because its candidate differs from the oracle's in the last bits
  (evidence/vendored-correspondence.md), and NxRayTriIntersect (phys_fn_001712), whose candidate
  differs from the oracle's on some NaN inputs (the bit-written draws now reach signalling NaNs).
  Each side reaches its own, so the family pre-flights every fan through both callees under the word
  it is driven under: the fans on which both agree are compared exactly and gate (0 and 0); the others
  (28,725 + 75 under 0x027f, 41,198 + 31 under 0x0f7f) are counted apart, with 1,051 and 1,603
  differing words, under ceilings the harness enforces (`kCalleeDivergentFanCeiling`,
  `kCalleeDivergentWordCeiling`).

14 oracle-side lines were registered; the phase 3 coverage floor goes from 126 to 140.

**Coverage.** point_triangle reaches every leaf (vertex0/1/2, both edges, the open regions, and the
determinant-zero FLT_MAX interior, 3,998); segment_triangle reaches the parallel branch and r at the
start, the end and between (22,940 / 10,039 / 11,602); aabb_slab every face and the miss, with 15,029
direction components inside (-FLT_EPSILON, FLT_EPSILON) and 7,445 exactly on it;
segment_triangle_edges every exit (one side or parallel 16,188, t < 0 12,710, outside the edge
25,249, on the edge 5,853). About 20,000 draws of each family are mixed-exponent.

**Review.** The first registration of these families (commit 53ebe9c) overclaimed: its draws never
mixed exponents, and it drew raw words through nxPick, whose float return value quiets a signalling
NaN or not depending on how the compiler inlined the call site. It measured 001694 at 14 words of
0x0f7f divergence "all 001690's", 001760 at 51 and 001855 at 32. The review's mixed-exponent
differential found 001672's own setup divergent under 0x0f7f (MSVC held the first edge in three qword
slots; about 3 words per million calls, and nearly all of 001694's own), and 001760 reproducible in
x87 assembly. Commit 1c0431d moved 001672's first edge into a leaf of its own, wrote 001760's normal
and normalisation (0x0003c163..0x0003c21b) as one x87 `__asm` block transcribed from the listing
(recorded in `X87Sqrt.h` as the per-site precedent; d stays C++), made the families' draws as above,
and extended ray_inflated_tris's pre-flight to 001712. The counts above are that measurement; only
this task's own 14 lines were re-registered, and all 122 registered collision lines reproduce.

**Defects found.**
- *Candidate, register lifetime:* 001672 written as one function differed on 2,483 words under
  0x0f7f (the first edge, c and t spilled to qwords); written as the listing's leaves, forming c and t
  from the stored floats where they are used, 0 -- and, with mixed-exponent draws, 0 only once the
  first edge had a leaf of its own (the review). 001694 with its setup and interior in one function
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
functions and the 28 leaves 001672 and 001694 are split into, in `NxPhysicsCollisionTests.exe`
(sha256 796dc67c..., the build of commit 1c0431d, addresses from its linker map); all 35 hit in one
full run that ended `collision=pass`. 001732 has no address of its own (it is the loop of `NxRayAABBSlab`).

**Ledger.** Phase 2: 001672, 001730, 001732 leave `homeless_shared_code`; phase 3: 001692, 001708,
001760, 001855 leave `not_reconstructed_in_phase`; phase 4: 001694 leaves `not_reconstructed_in_phase`.
All eight defer `reconstructed_not_falsified` with the standard note (no mutation was aimed at them);
counts and reason texts updated.

**Verification.** Build (Release, Win32) clean; gates 2, 3, 4, 6 and 7 pass; phase 5 fails only on
`candidate CANDIDATE-MISSING family=vtables`; 753 tool tests pass; `validate_inventory.py` exits 0.

## Harness hardening (between Tasks 2b and 2c)

**Why.** The Phase 3 collision harness drew raw words through `float nxPick(unsigned*)`. Under the
x86 ABI a float return travels in st(0), even from the harness's SSE2 translation unit, and loading a
signalling NaN there quiets it; an inlined call keeps the value in an XMM register and hands it on
intact. So which draws reached the oracle quieted depended on the compiler's inlining (Opcode.h's
`#pragma inline_depth(255)` and wmain's size had both moved it), and registered oracle digests pinned
the harness's code generation (Task 2b, defect 4). The Task 2b families already drew through
`nxPickBits` (bits, never a float); every other family now does too.

**Generators** (`tests/PhysicsCollisionTests.cpp`, commit a8df0e8).
- `nxPickWordFrom` writes a word as drawn; `nxPickWord` (every family written before Task 2b) draws
  the old mixture with the old stream and then quiets a NaN explicitly (bit 22, sign and payload
  kept -- what an x87 load does); `nxPickRawWord` keeps signalling NaNs; `nxPickBits` shares the code.
  No generator returns a raw word as a float (a tool test pins that).
- Measured first with the old float-returning generator and the input digests below in place: all
  122 registered lines reproduced. With `nxPickWord` as written, every pre-Task-2b block's input
  digest and oracle digest is the same as the float-returning generator's -- every one of those call
  sites had been going through st(0) -- except the compound families, changed on purpose (below).
- *Signalling NaNs: a class, and the policy.* Handed the same draws with their signalling NaNs
  kept, 15 pre-Task-2b blocks' candidates differ from the oracle; quieting the same draws restores
  every digest and every count, so all of it is signalling-NaN driven. The mechanism is x87's NaN
  rule: a signalling memory operand loses to a quiet register operand whatever the significands, and
  `fld` quiets what it loads, so a candidate that loads an operand its listing uses from memory (or
  the reverse) propagates another NaN -- mostly a different payload, and where a sign-bit test or a
  compare reads the propagated NaN, a different branch (a different verdict, contact count or
  stream). Policy (controller decision, after the first report):
  - each block's registered family keeps drawing its NaNs quiet (`nxPickWord`), exact, its lines
    unchanged;
  - each also runs as `<block>.snan` on the same draws with the signalling NaNs kept, under both
    control words, divergent under ceilings the harness enforces (`kSnanCeilings`, the
    `kDivergentCeilings` pattern of `tests/PhysicsThirdPartyTests.cpp`): differing words, how many
    of them are discrete (a verdict, a count, a stream header) and how many are float words not NaN
    on both sides ("non-NaN": a branch, or a NaN against a number, rather than a payload), per
    control word; a count may fall, never rise. Only oracle-side lines are registered -- the oracle
    digest, the input digest, and coverage lines with no candidate count in them;
  - these rows are not rewritten for it: a signalling NaN is a pathological input, and the fix --
    listing-faithful memory operands -- is a transcription as large as the row. The naked copy of
    phys_fn_001712 (below) is the precedent if a row ever needs it. Each row's inventory `notes`
    says its signalling-NaN propagation is not reproduced and names its `.snan` block.

  The ceilings (0x027f / 0x0f7f), measured on the build of this commit:

| `.snan` block | row | words | discrete | non-NaN |
|---|---|---:|---:|---:|
| box_corner | 000943 | 268 / 268 | 0 / 0 | 0 / 0 |
| box_quad_depth | 001739 | 2 / 2 | 0 / 0 | 0 / 0 |
| box_clip.random | 001741 (001743) | 19,717 / 19,700 | 589 / 588 | 18,520 / 18,504 |
| box_axis.random | 001745 | 30,212 / 30,190 | 2,904 / 2,897 | 25,904 / 25,889 |
| box_shim | 001748 | 5,105 / 5,105 | 435 / 435 | 4,335 / 4,335 |
| contact_box_box | 001749 | 346 / 346 | 78 / 78 | 236 / 236 |
| step_smooth_normals | 002146 | 15 / 15 | 0 / 0 | 0 / 0 |
| contact_emit | 000873 | 3,738 / 3,738 | 0 / 0 | 0 / 0 |
| shape_raycast_plane | 001261 | 1,720 / 1,720 | 0 / 0 | 0 / 0 |
| contact_plane_capsule | 001891 | 470 / 470 | 0 / 0 | 0 / 0 |
| shape_raycast_sphere | 001377 | 2,171 / 2,171 | 0 / 0 | 0 / 0 |
| contact_sphere_capsule | 001923 | 804 / 804 | 0 / 0 | 0 / 0 |
| sphere_box_contact | 001917 | 3,181 / 3,181 | 0 / 0 | 0 / 0 |
| contact_sphere_box | 001919 | 2,022 / 2,022 | 0 / 0 | 0 / 0 |
| contact_box_capsule | 001753 | 99 / 93 | 7 / 7 | 40 / 40 |
| fuzz_ray_plane | 001704 | 15 / 15 | 0 / 0 | 0 / 0 |
| fuzz_ray_aabb | 001722 | 1 / 1 | 0 / 0 | 0 / 0 |
| fuzz_segment_box | 001714 | 2 / 2 | 0 / 0 | 0 / 0 |

  Ten of the fifteen differ in NaN payloads only; box_clip, box_axis, box_shim, contact_box_box and
  contact_box_capsule also branch (discrete and non-NaN words). Every comparison a `.snan` block
  makes feeds these counts (the sink's separating-axis byte in contact_box_box and
  contact_box_capsule too, a discrete word: 39 / 39 and 3 / 3 of theirs, which the first
  registration of the ceilings missed and the review caught). So `words` is each block's own
  per-word mismatch count, with one exception in unit: box_quad_depth's own count is per byte of
  its ten-byte spill (7 / 7), where `words` counts spills (2 / 2). The fifteen add 35 registered
  lines (15 digests, 15 input digests, 5 coverage lines without candidate counts).
- *The kernel fuzz harness* (`tests/PhysicsKernelFuzzTests.cpp`) had the same float-returning
  `nxPick`, and three unsequenced draws in one expression in two of its cases. Its generator now
  writes bits (the draws sequenced exponent, significand, sign) and quiets NaNs explicitly: every
  line it prints is unchanged on both pairs (compared against the float-returning build on the
  shipped and the rebuilt pair), so none of its 18 registered lines moved and none was
  re-registered. With the signalling NaNs kept, three of its exports' candidates differ
  (NxRayPlaneIntersect, NxRayAABBIntersect, NxSegmentBoxIntersect; NaN payloads). A staged-pair
  differential cannot carry a line on which its pairs differ, so those draws -- the fuzz harness's
  own stream, replayed word for word -- run in NxPhysicsCollisionTests (`nxDriveFuzzSnan`, the
  oracle in process, both control words) as `fuzz_ray_plane.snan`, `fuzz_ray_aabb.snan` and
  `fuzz_segment_box.snan`, in the table above (9 lines registered).
- *Unsequenced draws.* Expressions that call the generator more than once where C++ leaves the
  order to the compiler (function or constructor arguments, the operands of `*` or `|`) are
  sequenced into named locals, in the order the build evaluated them, so every registered digest
  stays identical. The compound families' flat-bounds index pair and `nxMixedBits` came first (the
  first had already moved when the `.snan` passes were added); after the review, the rest across
  the harnesses:
  - PhysicsCollisionTests.cpp: the triangle point's height draw and the segment/edge `up` draw
    (the xorshift draw before the unit draw, both);
  - PhysicsKernelFuzzTests.cpp: the three rotation angles of the aimed box block (x, y, z) and of
    the SAT block's two boxes (y, x, z and x, y, z -- the compiler chose differently for two
    identical statements; found by building all 36 combinations);
  - PhysicsThirdPartyTests.cpp: the grown box maxima (z, y, x), the mesh inside point (z, y, x),
    the ray segment offsets (x, y), the sweep-and-prune boxes (centre x, y, z; extents z, x, y),
    its moves (centre z, x, y; extents z, y, x), the ICE AABB pair (a: extent y, centre x, y, z,
    extent x, z; b: extents x, y, z, then centre x, y, z), plane/triangle points (x, y, z), OBBs
    (centre x, y, z; extents y, x, z; the other's offset and extents x, y, z) and the unit points
    (z, y, x). The sweep-and-prune, AABB and OBB orders were read off the run of the unmodified
    build: its oracle-call arguments dumped under cdb and matched against the draws replayed in
    Python;
  - FoundationTangentTests.cpp: the unit and scaled sweep vectors (z, y, x).
  Every registered line of the four harnesses, and the Phase 3 geometry transcript, is unchanged.
  Braced initialisers and comma-separated declarations are sequenced by the language and were left
  as they are.
  *Correction (convex-mesh gap Task 2c).* The review fixes above did not sequence every site: one
  was left in PhysicsCollisionTests.cpp, segment_segment's near-parallel `scale` (a unit draw
  times a sign drawn from `nxNext`, operands of one `*`). It is now two named locals in the
  order the build had evaluated them, the sign word first (the other order moves the
  segment_segment input, oracle and coverage lines); the harness's whole stdout is identical
  before and after. A statement scan of tests/*.cpp and *.c for two or more generator calls
  finds no other unsequenced site (the remaining ones are `?:` arms, `&&` operands, braced
  initialisers, comma declarations or a call whose argument draw precedes its body's).
- *Float-returning raw-bit helpers elsewhere.* FoundationTangentTests.cpp poisoned its outputs with
  the signalling NaN 0x7fa5a5a5 through `float nxFloat(NxU32)`; it now writes words into their
  slots (`nxSetBits`). PhysicsGeometryTests.cpp's `nxF` (its case words hold no signalling NaN) is
  replaced by the file's own pointer view, PhysicsJointTests.cpp's unused `nxF` is gone, and the
  fuzz harness's witnesses are copied into locals. Every line those harnesses print is unchanged.
  `test_gate_targets.py` now rejects, in every tests/*.cpp and *.h, a function that returns a float
  type built from raw bits (a memcpy or a pointer pun), not only the collision generators by name.
- The compound families' raw bounds were `c - h` and `c + h` over two raw draws; SSE propagates the
  first operand's NaN and the sum is commutative, so the operand order the compiler emitted chose the
  payload. They are now the two drawn words themselves (same stream).
- `tests/PhysicsThirdPartyTests.cpp`: its one raw-bit float return (`nxNextFloat`, the radix sort's
  float keys) now writes bits. Its 346 draws contain no signalling NaN (checked), so the words and
  every Phase 4 line are unchanged.

**Input digests.** Every block prints `collision input name=<block> words=<n> input=<digest>`: FNV-1a
over the exact words it hands the oracle, once per draw (shapes: rotation, translation, the
trigger-flag dword, type, geometry; no addresses). All 58 are registered, so an input change is told
apart from an oracle change.

**001712 (NxRayTriIntersect).** On signalling NaNs its C++ candidate differed from the oracle
(step_ray_tri: 1,310 words once its draws kept their signalling NaNs; ray_inflated_tris' pre-flight:
75 / 31 fans under 0x027f / 0x0f7f). Geometry.cpp is on the `/arch:IA32` list, so both sides were
x87; the difference was which operands are loaded. The listing (0x00036f50..0x0003725d) loads vert1,
vert2 and the origin with `fld` and uses vert0 and the direction only as memory operands of
`fsub`/`fmul`; MSVC's C++ loaded vert0 and the direction into registers first (quieting them) and
reordered operands, so the propagated payload -- and the sign bit the `test eax, eax; js` range
tests read -- differed. The row is now a naked x87 function transcribed instruction for instruction
(the X87Sqrt.h precedent, recorded there); its disassembly equals the listing's (constant addresses
and branch targets aside, branch offsets equal). step_ray_tri now draws signalling NaNs
(`nxPickRawWord`): 0 differing words under both words; ray_inflated_tris' pre-flight: 0 / 0 fans.
The row gains its stable-ID line and its inventory row its static proof, trace and notes (state
unchanged, `dynamically_gated`); `validate_inventory.py` drops it from `IMPLEMENTATION_MISMATCHES`.

**ray_inflated_tris.** The 001712 pre-flight compares outputs as the family folds words (a NaN is
any NaN) instead of `memcmp`; a new line prints the gating fans per word and draw kind (0x027f: raw
10,778, aimed 14,293, mixed 6,204; 0x0f7f: 3,727 / 13,581 / 1,494). What remains apart is
Inflate's: 28,725 / 41,198 fans, 963 / 1,515 words, the new ceilings.

**001694.** Its C++ interior leaf spilled s and partial products to 8-byte slots and summed in its
own order; on the Task 2b review's 1M draws (a quarter of the words of mixed exponent) that differed
on 2 words under 0x0f7f, which none of the Task 2b draws reached (so "001694's own code is exact",
said in Task 2b, held only for those draws). The interior (0x00035483..0x00035524), with s re-formed
from the stored floats as at 0x00034b1a..0x00034b4e, is now an x87 block from the listing: no 8-byte
access (disassembly), and on the review's draws 0 and 0 (the reviewer's scratch program rebuilt on
both versions: 2 before, 0 after). Aimed interior crossings -- scaled, skinny, grazing and
far-reaching -- never separated the two interiors (measured: identical candidate digests), so
segment_triangle instead replays the review's first 250,000 draws, which contain both cases: with
the oracle's 001690 bound in (a throwaway build) 0, and with the C++ interior 2. Its 0x0f7f count is
now 940 words, all 001690's own divergence reached through it.

**Re-registered once** (the controller's one-time exception to the append-only rule; values verbatim
from the oracle side; candidate agreement unchanged except where noted):

| block | input before -> after | oracle before -> after | mismatches before -> after |
|---|---|---|---|
| step_ray_tri | d03f1c26d2a5b2ca -> b3af9202902cf7ca | 2bb3aaedbd4ac9ed -> fdbc6163470513f9 | 0 -> 0 (now on signalling NaNs) |
| sphere_compound | 2c1c06d8d757445d -> ee0e412b089ffc8c | e3cf7959326e8e05 -> 306d6cb8c80ea48f | 0 -> 0 |
| box_compound | 7dc1610c243c3e69 -> 9a3dac417a7bf45c | 0f9d2946320876b4 -> 8ae3e6725ef7706e | 0 -> 0 |
| capsule_compound | 51bba59e7a1e7aa1 -> 54afd91ea4c41c1e | 5b6ab7e9e5695661 -> 97318ad61948798e | 0 -> 0 |
| segment_triangle | c6fc5d65972ab760 -> 0e8f31c39ee53daf | 601b861a1a0d4e66 -> c77590b1ba895124 | 0x027f 0 -> 0; 0x0f7f 31 -> 940, all 001690's (0 bound) |
| ray_inflated_tris | b96f8d70443d0ad3 (same) | 9604e3071aae4c41 (same) | 0 -> 0; coverage only (raytri_divergent 75 / 31 -> 0 / 0) |

("Before" is the old generator with the input digests added, which reproduced all 122 registered
lines.) The name and coverage lines of the first five and ray_inflated_tris' coverage line were
replaced (11 lines); 58 input lines and the gated line were added (59), then the 44 `.snan` lines
(35 + 9). Phase 3 floor 140 -> 243; `test_gate_targets.py` pins 243 and requires an input line for
every driven block, a digest and an input line (and no candidate count in a coverage line) for every
`.snan` block and a `kSnanCeilings` entry for it, and no float-returning raw generator (756 tool
tests).

**Trace.** `evidence/convex-mesh-gap-trace-H.txt`: 12 one-shot breakpoints (001712; 001694 and its
ten leaves, the interior block among them) on the build of a8df0e8 (exe sha256 7fc1fb88...); all hit,
`collision=pass`.

**Trace, `.snan` follow-up.** No product code changed after a8df0e8 (harness, registry, inventory
notes and evidence only), so the trace above stands.

## Task 2c: EdgeList, IceAdjacencies and the valencies

**Rows (12, 5,564 B), all `discovered` -> `reconstructed`.** Prerequisite P-EdgeList in the new
`Physics/src/EdgeList.cpp` (the oracle's own `__FILE__`): 002054 `CreateFacesToEdges` (554 B) with
its continuation 002056 (313 B, missing from the contract's row list until now), 002058
`CreateEdgesToFaces` (467 B), 002061 `ComputeActiveEdges` (1,933 B) and 002063 `Init` (225 B).
Sub-unit A in the new `Physics/src/IceAdjacencies.cpp`: 001537 AddTriangle, 001539 UpdateLink and
001541 CreateDatabase (register conventions, written as noinline functions with those registers as
parameters), 001542 ComputeNbBoundaryEdges and 001546 Init with its continuation 001548. From
sub-unit D, 001667 `Valencies::Compute` in the new `Physics/src/IceMeshTools.cpp`. The five
already-reconstructed rows these need -- 002052 and 002060 (EdgeList constructor and release), 001544
(Adjacencies release), 001663 and 001665 (Valencies constructor and release) -- have product forms in
the same files with their stable-ID lines; their ObjectModel.cpp models and proofs stand, their
states are unchanged and their notes say so. Nothing is vendored: OPCODE 1.3's `Ice/` has no
IceAdjacencies, EdgeList or Valencies (checked against `External/opcode/upstream/Opcode/Ice`), and no
candidate stand-in or caller existed for any of the rows (callers 001465, 001411, 002186, 002188,
002239 are not written), so there was nothing to wire.

- *Allocation.* Every allocation in the three files is the 004803 getter's (slot 0 with the
  listing's type, slot 3 to release; none through CRT new/free or the imported allocator), with the
  listing's count cookies: `new[]` of the adjacency faces, the temporary edge records, the edge
  links, the edge buffer and the edges (released at the pointer minus four), plain blocks for the
  reference lists, descriptors (zeroed by their constructor 0x1002a610), faces-by-edges, the mark
  buffers and the three valency arrays. The families record every call and compare them.
- *Reports.* Through the SetIceError seam (002160, `OpcodeNovodeXHost.h`) with the oracle's file
  string, line and message; the report's `false` is the row's return value, as in the listing.
- *Vendored callees,* called where the oracle calls them: RadixSort (005157, 005163 with hint 0,
  005159), IndexedTriangle::FindEdge (005189), Plane::Set (005155) and Triangle::Normal (005181).
- *x87.* 002061's two float sections are assembly blocks transcribed from the listing: the side of
  the plane (0x0005189f..0x000518d4) and the angle (0x000519c1..0x00051a3a), whose cross-product
  length is square-rooted and kept on the FPU stack into `fpatan`. EdgeList.cpp and IceMeshTools.cpp
  are on the `/arch:IA32` list; IceAdjacencies.cpp is integer code.
- *Frames.* The three files are built with `/EHs-c-`, as the joint files are: the listings are
  frameless, and under `/EHsc` the RadixSort and EdgeList locals and the noexcept destructors gave
  seven functions unwind frames. The `/GS` cookies on the functions with local arrays remain
  (002061, 001539, and 001541 for its alloca), as 001708's does.

**Listing findings.** 001539 reads both faces from DFaces and then from WFaces (two tests, not an
else), so its invalid-edge arm is reachable only when the two face arrays disagree -- a repeated
vertex cannot reach it (the contract said it could; corrected). The link word is
`(edge << 30) | face`. ADJACENCIESCREATE has a fifth field, the epsilon at +0x10, which 001546 hands
to the EdgeList create block; 002061 never reads it and compares the angle with the constant 0.1f
(0x10106954). 001546 returns CreateDatabase's result whatever its EdgeList does. 002054 starts both
last references at 0xffffffff (not at the first sorted pair, as ICE does) and leaks what it has
allocated on every failing path; 002061 sizes its vertex marks by the largest reference over the
`nb_faces` argument but walks `mNbFaces`, and reads vertex 0xffffffff (12 bytes before the array)
when the edge is not a side of the first face.

**Differential** (`NxPhysicsThirdPartyTests`; the oracle's entry rows by RVA on oracle-side objects,
the candidate's on its own; tapes compared word for word). Inputs: the six NxMesh fixtures and 59
meshes of the task's own -- height grids straddling the 0.1 rad threshold, folded pairs (convex,
concave, flat, both windings), three faces on one edge, a face (a, a, a), duplicated faces (same and
reversed winding), the box with isolated vertices, the height field with shuffled indices and faces,
a pair whose shared edge sorts last, 24 random soups and six meshes with raw words among the
coordinates (written as bits). Each run's tape: the return value, the whole object image (counts,
every table word, every count cookie), every allocation (type, size) and release (which block) through
the 004803 getter -- a recording allocator written into the oracle's singleton slot
.data:0x0012845c for the oracle pass and installed through `nxSetSdkAllocatorBridge` for the
candidate's, blocks filled with 0xcd -- and every report (line, file and message digests; the
oracle's import slot 0x001041b4 redirected to a recorder, the candidate's seam made capturable in
`tests/PhysicsThirdPartyHost.cpp`). Vertex arrays sit behind a fixed 12-byte guard.

| family | entry | runs | words | result |
|---|---|---:|---:|---|
| edge_list | 002063 (and 002054, 002061 direct) | 650 + direct | 160,261 | exact |
| edge_list.plane_divergent | 002063, vertex runs of 13 meshes | (in the 650) | 23,225 | 465 words, ceiling 465 |
| ice_adjacencies | 001546 (001542, 001544 after each) | 335 | 35,084 | exact |
| ice_adjacencies.plane_divergent | 001546, vertex runs of the same 13 | (in the 335) | 3,528 | 124 words, ceiling 124 |
| ice_valencies | 001667 | 261 | 32,709 | exact |

Coverage (oracle side, registered): edge_list 27,045 edges, 6,714 active-edge and 12,384
active-vertex link bits, every report of 002054 and 002061 (0x72 twice; 0x10a, 0x10b, 0x10e, 0x111,
0x114, 0x117); ice_adjacencies 182 true / 153 false, 12,821 links, 14,323 boundary words, 15,877
active bits, reports at 266 (2), 267 (5), 321 (146) and EdgeList's 0x72 (1); ice_valencies 260 true
/ 1 false (no faces), 12,020 adjacent entries.

*The divergent split.* 002061 decides an edge shared by two faces from two vendored callees:
Plane::Set for the second face's plane (the first face's opposite vertex is tested against it) and
Triangle::Normal for the angle, and both are measured divergences (`ice_plane_triangle`, up to 2,820
ulp; 005155 and 005181 are held `discovered`). A last-bit difference flips the decision for a vertex
in the plane or an angle at 0.1. Measured on a throwaway build that could bind the oracle's rows
into the candidate's 002061: edge_list 465 differing words and ice_adjacencies 124 (whose vertex
runs go through the same EdgeList); with 005155 bound 306 / 100, with 005181 bound 342 / 92, with
both bound 0 / 0 -- every differing word is the callees'. (Before the six raw-word meshes were added:
72 / 0, and 0 with 005155 alone.) Each mesh was pre-flighted edge by edge in 002061's order
(faces ascending, the six-way opposite-vertex rule, both callees' planes and normals; an angle within
1e-6 of 0.1 counts as undecided): 1,487 two-face edges, 40 with the side differing and 9 with the
angle decision, in 13 meshes (eight random soups, 40..53, and five raw-word meshes, 59..63; every
fixture and every designed mesh is gated). The vertex runs of those 13 meshes go to
`<family>.plane_divergent`, registered up to the oracle digest and held by `kDivergentCeilings`
(465/465, 124/124 discrete words; a change either way is reported, exceeding fails). Every other
run, including every vertex run of the fixtures, is exact.

*Frozen split (Task 2c review).* As first registered, the split was recomputed each run from that
pre-flight, which runs the candidate's vendored Plane::Set / Triangle::Normal -- so the registered
digests encoded candidate behaviour, and a later fix to 005155/005181 would have moved lines that
append-only registration then protects. The split is now the constant list
`kIcePlaneDivergentMeshes` = {40, 41, 42, 43, 44, 46, 50, 53, 59, 60, 61, 62, 63} in the harness; the
pre-flight remains as a check that fails the run when a mesh OUTSIDE the list could diverge
(checked: dropping 63 from the list fails with exit 1), and prints its detail to stderr only. The
list reproduces the same split, so every digest and count is unchanged; the one registered line
that carried candidate-dependent counts was replaced (a line this task added, not yet merged):

    before: thirdparty coverage name=edge_list.plane_divergent pairs=1487 side=40 angle=9 meshes=13
    after:  thirdparty coverage name=edge_list.plane_divergent frozen_meshes=13

*Reproducing the attribution.* `evidence/convex-mesh-gap-2c-bind-oracle-callees.patch` is the
throwaway change, applicable with `git apply` to the commit that froze the split: hooks in
EdgeList.cpp's 002061 that the harness fills from `NX_ICE_BIND` (1 binds the oracle's 005155 at
base+0x000e31c0, 2 its 005181 at base+0x000e3f50, 3 both). Rebuild NxPhysicsThirdPartyTests and run
it with NX_ICE_BIND=0..3: the `.plane_divergent` lines read 465/124, 306/100, 342/92 and 0/0 (re-run
for the review). Never commit it applied: product code must not reach the oracle.

*The 002160 seam.* The rows report through `opcNovodeXSetIceError`, the host seam for 002160. In the
product DLL that seam (Physics/src/ThirdPartyHost.cpp) is still the shim: it reports nothing and
returns false, where the oracle's 002160 forwards (2, file, line, 0, message) to NxFoundation's
variadic `FoundationSDK::error` (import slot [0x101041b4], message as format). The rows' return values are unaffected (both return the false), and the
families compare the (message, file, line) the rows pass; what a user's error stream would receive
is not reproduced until 002160 is written as product code.

Registered: 14 lines (per family an input digest, the exact line whole, the divergent line to the
oracle digest and the coverage lines, and the totals pair `driven=76 divergent=27 words=1755447` /
`4a282660`); phase 4 floor 161 -> 175 and `test_gate_targets.py` MINIMUM with it. The other
families' lines, including the totals pairs printed before these families, are unchanged. The
harness is deterministic run to run, and `--self` prints the same oracle digests.

**Also (separate commit).** tests/PhysicsCollisionTests.cpp: segment_segment's near-parallel
`scale` (a unit draw and a sign draw, operands of one `*`) sequenced into named locals, sign word
first -- the order the build had used (the other order moves the segment_segment lines); the whole
stdout is identical. The harness-hardening review's claim that every remaining site had been
sequenced is corrected under `## Harness hardening`. The tool test that rejects float-returning
raw-bit helpers now scans `tests/*.c` too, finds `reinterpret_cast` and union puns (local, named and
typedef'd) and sees calling conventions and `const` between the return type and the name, with a
probe test per shape; the widened scan found `nxStreamReadFloat` in PhysicsAssetTests.cpp, the
harness stream's `NxStream::readFloat` slot, whose float return is the interface's ABI on both sides
(exempted by name; the exemption is checked live).

**Trace.** `evidence/convex-mesh-gap-trace-2c.txt`: one-shot cdb breakpoints (from
`NxPhysicsThirdPartyTests.map`, written against `@$exentry` because this cdb names the 22 MB image
`image<base>`) on the 15 candidate functions -- the twelve rows' ten functions and the five product
forms; 001548 and 002056 are continuations with no address of their own -- all hit in one full run,
closing `thirdparty candidate mismatches=0` (exe sha256 e9b4c202edc33af5..., build of f325d1a).

**Inventory and ledgers.** The 12 rows: `reconstructed`, `source`/`implementation` their file,
static proofs citing the listing ranges, dynamic proofs citing the trace and the family. Ledgers,
with the standard note: phase 2 001546/001548 homeless_shared_code -> reconstructed_not_falsified
(51 -> 49 / 7 -> 9); phase 3 001542, 001667 not_reconstructed_in_phase -> reconstructed_not_falsified
(302 -> 300 / 11 -> 13); phase 4 the eight EdgeList and IceAdjacencies rows (296 -> 288 / 472 -> 480);
reason texts and the phase 3 note's count updated. `validate_inventory.py`: the EdgeList.cpp and
IceAdjacencies.cpp entries leave the unresolved-source allowlist (the check asked for it).

## Task 2d: MeshBuilder2 and the vertex reduction

**Rows (26, 10,728 B), all `discovered` -> `reconstructed`.** Sub-unit C, all 25 rows
(001591..001637, 10,236 B, seven of them continuations), in the new `Physics/src/IceMeshBuilder2.cpp`
with its header `Physics/src/include/IceMeshBuilder2.h`; from sub-unit D the vertex reduction 001647
(492 B) in `Physics/src/IceMeshTools.cpp`, with product forms of the already-reconstructed 001645
(its constructor) and 001659 (its destructor) and their stable-ID lines (their ObjectModel.cpp models
and proofs stand; states unchanged; notes say so). The shape is ICE's MeshBuilder2, which is not
vendored (OPCODE 1.3's `Ice/` has none). Nothing is wired: the rows' only caller, 002087
(0x000523c0, the EdgeList.cpp..InternalTriangleMesh.cpp gap), is not written, and the candidate has
no stand-in for any row.

- *Heap (the contract's open item, checked first).* MeshBuilder2 allocates through 005701, which is
  `jmp 005702` (`operator new`: `push 1; push size; call __nh_malloc` 005690), and frees through
  005700, which is `jmp 005668` (`_free`). 001514's pair 005702/005668 is the same two functions
  reached without the thunks: one heap, the DLL's static CRT (listing 0x000f48bb..0x000f48d2,
  0x000f41f0). The candidate uses its own CRT's pair: nothrow `operator new` (so a failure returns
  null, as `__nh_malloc` does) and `free`, which pair (MSVC's `operator new` is `malloc`). The
  `new[]` blocks -- the three stream copies (001595, 001599, 001611), the face array (001623, 001611)
  -- carry the listing's count cookie and are released at the pointer minus four; the references,
  per-vertex tables, face remap, marks, remaps and sort keys are plain blocks. The vertex reduction
  allocates through the 004803 getter (cross-reference and reduced array type 0, key buffer type 1),
  as its listing does; the Containers grow through the vendored Resize (the 004803 getter).
- *x87.* The float sections are assembly blocks transcribed from the listing: 001597's zero-area
  test ((p0-p1)^(p0-p2), x and y spilled to floats, (z*z+y*y)+x*x against 0.0f, 0x0002ee29..
  0x0002eece), 001603's face normal and its normalisation (0x0002f661..0x0002f734), 001627's
  angle-weighted corner normal (the cross product's length square-rooted and rounded to float before
  `fpatan`, unlike 002061's), its plain sum and the vertex normal's normalisation (0x0003097c..
  0x00030b5d; the weighted and plain paths share the z store as in the listing), and the `fld`/`fstp`
  through which 001607 and 001627 pass each uvw word (so a signalling NaN comes out quiet, as in the
  oracle). The FPU instruction sequence of each of the four built functions equals the listing's --
  register forms byte for byte, memory forms by operation and size (the operands' addressing differs
  by construction): 45, 62, 6 and 110 instructions (`evidence/convex-mesh-gap-2d-fpu-opcodes.py`,
  which reads the committed Capstone manifest and a `dumpbin /disasm` of the object; usage in its
  docstring). IceMeshBuilder2.cpp is on the `/arch:IA32` list.
- *Frames and functions.* The file is built `/EHs-c-` with the other ICE-shaped files (the listings
  are frameless; the Container, reducer and RadixSort locals would otherwise get unwind frames). Every
  row the oracle has as a function of its own is `noinline` (001595, 001602, 001617 and 001625 had
  been inlined into Init, Build and OutputRun), so each has an address to trace. 001591, thiscall
  on the Container with `ret 4` in the oracle, is a `__fastcall` function with the Container in ecx,
  an unused edx and the point on the stack (the built function ends in `ret 4`): the same registers
  and stack cleanup, without changing the vendored Container header (Task 2d review). 001647 carries
  a `/GS` cookie (its three-word "previous" array `Junk[3]` on the stack, IceMeshTools.cpp) that the
  oracle's frameless listing lacks, as 002061, 001539 and 001541 do (Task 2c); no other function of
  the two files has one (checked in the objects' disassembly).

**Listing findings** (reproduced; the contract is corrected where it said otherwise).
- 001627 is called once per *run* of faces of equal (material, smoothing groups), not once per
  sorted face: 001631 cuts the radix-sorted faces (smoothing groups, then material, both unsigned)
  into runs and hands each to 001627 as it closes. Each run appends five words to +0xb0 (material,
  smoothing groups, face count, new-vertex count, 0) and its face count to +0x10.
- 001593 never writes +0x123 (only Init copies it). Init (001623) does not reset the face and
  reference counts (+0xe0, +0xe4): a second Init on a built object keeps them, so AddFace continues
  at the old index or rejects every face when the old count equals the new maximum. 002087 always
  builds a fresh object.
- 001597's zero-area test reads the vertex copies at the face's references before they are clamped
  (the clamp to 0 at or past each stream's count comes after); the face index may equal +0xd0.
- 001635 gathers the runs per material with 0xffffffff meaning "none yet": runs of material
  0xffffffff followed by another material are counted into that material's entry, not flushed
  (002087 passes 0xffffffff for every face).
- 001611 marks the entry of the previous reference for a `which` other than 1, 2 or 4 (unreachable:
  001625 passes only those); on a failed packed-stream allocation it frees the remap and the marks.
  With duplicates, and for vertices only, the faces whose three vertices no longer differ are dropped
  into a new face array allocated with the *old* count as its cookie.
- 001617's remap and 001647's reduced array are not null-checked (the listing stores through them).
- 001647 compares each sorted vertex with the previous one by bits, starting from three 0xffffffff
  words on the stack: a smallest vertex with those bits is not kept and its cross-reference is
  0xffffffff (driven; reproduced).

**Differential** (`NxPhysicsThirdPartyTests`, through the Task 2c `nxIceFamily`: the recording 004803
allocator in the oracle's singleton slot and through `nxSetSdkAllocatorBridge`, the report recorder
installed per pass; entry rows by RVA on oracle-side objects, the candidate's on its own).

| family | entries | runs | words | result |
|---|---|---:|---:|---|
| ice_meshbuilder2 | 001593, 001623, 001597, 001633, 001629 | 249 cases | 178,141 | exact |
| vertex_reduction | 001645, 001647, 001659 | 178 reductions | 24,313 | exact |

*ice_meshbuilder2* is driven as 002087 drives it: constructor, Init with a create block, AddFace per
triangle, Build, then (odd configurations) a second Init over the built object -- FreeUsedRam over
full Containers -- and the destructor. Once per mesh also: AddFace before Init, Init with no faces
(false, after copying the streams), Build before any face; and after every case AddFace with an
index above +0xd0. Inputs: the 65 meshes of Task 2c and three of this family's own (the box with
every face's corners unshared, which the vertex pass welds back; a face whose two corners are
different vertices at one position, which the vertex pass drops -- `killed=3`; a face with no
distinct corners). Eight create-block configurations (002087's own among them) vary the twelve
flags, the uvw stream (per vertex, a palette of five, a null source with a count -- the zeroed copy
of 001595), the colour stream (a palette of four, per vertex), all written as bits (see below),
smoothing groups (all 1, all 0 --
every face unshared --, drawn from {0, 1, 2, 4, 3}), materials (all 0xffffffff, {0, 1, 2},
{0xffffffff, 5, 3}), flips and out-of-range uvw/colour references (clamped). The six fixtures and
the three own meshes run all eight, the other meshes a rotating three; cases alternate between the
x87 control words 0x027f and 0x0f7f (154 of 249 under 0x0f7f). When Init succeeded (the side's own
return value, taped first and compared), Build is skipped by a rule on the fixed inputs for a case
none of whose faces has three corner positions of distinct bits (the vertex
pass would drop every face and 001631 would then read the rank of face 0 of an empty sort): the
eight cases of the third own mesh.

Each tape holds every return value; after Build (or its failure) the thirteen Containers (maximum,
count, growth factor, the first 24 entries word for word and a digest of all), the counts, the flag
bytes, digests of the owned arrays (the three stream copies with their cookies, the references, the
face remap, the per-vertex face tables, and the face records -- output corners only after a
successful Build, normals only when a normal flag computed them, since the CRT leaves the rest
unwritten); the result block's counts and each of its pointers as *which* array it points at (the
two sides allocate from different CRT heaps, so no pointer is compared); and every allocation and
release through the 004803 getter in order. After a second Init the face and reference records are
not digested (Init allocates them unwritten). The CRT blocks MeshBuilder2 takes itself come from the
oracle's own heap and are not recorded on either side.

*Raw-word streams (Task 2d review).* As first registered, the uvw and colour streams were built
with float arithmetic (`x * 0.5f`, `(float) (draw % 3) * 0.5f`), which quiets every signalling NaN
before it reaches the rows, so the `fld`/`fstp` pass-through of 001607 and 001627 was never handed
one, and an integer copy there would have produced the same tapes. The streams are now written as
bits: the per-vertex uvw is the vertex words copied; the palettes are fixed words (the uvw palette's
fifth entry a signalling NaN 0x7fa00005 and a denormal, the colour palette's fourth a signalling NaN
0xffa00007, +infinity and a quiet NaN); the per-vertex colours are drawn level words; and
`nxMb2RawWords` turns one word in six (drawn) into a signalling NaN of either sign, a quiet NaN, an
infinity, a denormal, -0 or a raw drawn word, each draw a named local. The input streams carry 510
(uvw) and 347 (colour) signalling-NaN words (counted from the fixed inputs, registered). Both the
indexed path (001607, +0x11e) and the per-vertex path (001627) are reached. ice_meshbuilder2 stays
exact, so the rows reproduce the oracle's quieting and no `.snan` variant is needed. Replacing the
pass-through with an integer copy (`evidence/convex-mesh-gap-2d-fldfstp-mutation.patch`, applied
with `git apply` to 2973938, NxPhysicsThirdPartyTests rebuilt and run, then `git checkout` the file)
gives `mismatches=276`: the pass-through is now tested. Re-registered (this task's own unmerged
lines, replaced once; values from the oracle side):

| line | before | after |
|---|---|---|
| input | `words=125178 input=1200bbc6` | `words=125178 input=333baf0f` |
| exact | `words=178377 oracle=5e33a49e` | `words=178141 oracle=ae71cdd4` |
| coverage | `out_verts=18056 submeshes=1043 materials=520 norm_info=34816 remapped=174` | `out_verts=18031 submeshes=1040 materials=515 norm_info=34694 remapped=177`, plus `uvw_snan=510 colour_snan=347` |
| totals | `words=1958137` / `d54a58fc` | `words=1957901` / `c2e747eb` |

The vertex_reduction lines are unchanged.

*vertex_reduction*: per mesh its vertices with a third again inserted as bit copies at drawn slots
(welded duplicates), with and without the result block; every fifth mesh twice on one object (the
release of both outputs at 001647's start); every fourth mesh's plain vertices twice; an empty set;
three copies of the 0xffffffff-bits vertex (nothing kept) and the same with a larger vertex. Tapes:
the object before and after, the cross-reference and reduced vertices word for word, the result
block against the object's pointers, and every 004803 allocation and release.

Coverage (oracle side, registered): 249 cases, 249 Init true (and 68 no-face Inits false among the
453 probes, all false); 6,695 faces added, 445 dropped by the zero-area test, 241 builds true, 8
skipped; 6,689 output faces, 18,031 output vertices, 1,040 runs, 515 material entries, 3 faces
dropped by the vertex pass, 34,694 normal-info words, 177 non-identity face remaps; 510 uvw and 347
colour signalling-NaN input words; vertex_reduction
178 reductions of 5,457 vertices to 4,331. No report is made by these rows (reports=0).

*Sensitivity (throwaway, not committed).* Each edit below was made on the working tree, rebuilt and
run, then reverted: (A) `jnp SkipNormalize` -> `jp` in 001627's normalisation: ice_meshbuilder2
2,655 mismatches; (C) the `fxch st(1)` before 001627's `fpatan` removed: 1,183; (B) 001597's third
uvw clamp `>=` -> `>` and (D) 001647 comparing only x and y: both end the run in an access violation
(an unclamped reference, a wrong cross-reference), which fails the gate as well; (E, review) the
uvw pass-through as an integer copy: 276 (the patch above). Reproduce A..D with, for
example, `sed -i 's/\t\t\t\tjnp\t\tSkipNormalize/\t\t\t\tjp\t\tSkipNormalize/'
Physics/src/IceMeshBuilder2.cpp`, rebuild NxPhysicsThirdPartyTests, run, `git checkout` the file.

Registered: 8 lines (per family the input digest, the exact line whole and the coverage line, and
the totals pair `driven=78 divergent=27 words=1957901` / `c2e747eb`), every one copied from the
oracle side of a run (the name lines' agreement fields are the gate's assertion; the coverage lines
count oracle-side values and fixed inputs only). Phase 4 floor 175 -> 183 and `test_gate_targets.py`
MINIMUM with it. Every line registered before is unchanged (the families run after all others and
restore the generator state). Deterministic run to run; `--self` prints the same oracle digests.

**Trace.** `evidence/convex-mesh-gap-trace-2d.txt`: one-shot cdb breakpoints (from the map, against
`@$exentry` as in Task 2c) on the 21 candidate functions -- the 26 rows' 19 functions (the seven
continuations have no address of their own) and the product forms of 001645 and 001659 -- all hit in
one full run closing `thirdparty candidate mismatches=0` (exe sha256 0f5bdda539b6c409..., build of
2973938, re-recorded after the review fixes; the first recording, on e8fe4e3, sha256
69e19c5619e2299c..., hit the same 21).

**Inventory and ledgers.** The 26 rows: `reconstructed`, `source`/`implementation` their file,
static proofs citing the listing ranges (and, for 001597, 001603, 001607 and 001627, the FPU
sequence check), dynamic proofs citing the trace and the family. 001645 and 001659: notes record the
product forms. Ledgers, with the standard note: phase 2 001647 homeless_shared_code ->
reconstructed_not_falsified (49 -> 48 / 9 -> 10); phase 4 the 25 MeshBuilder2 rows
not_reconstructed_in_phase -> reconstructed_not_falsified (288 -> 263 / 480 -> 505); reason texts
updated. `validate_inventory.py` asked for nothing else.

**Verification.** Build (Release, Win32) clean; gates 2, 3, 4, 6 and 7 pass; phase 5 fails only on
`candidate CANDIDATE-MISSING family=vtables`; tool tests 763 OK; `validate_inventory.py`
inventory=pass.

## Task 2e: the rest of sub-unit D and P-Small

**Rows (10, 4,370 B), all `discovered` -> `reconstructed`.**

| row | bytes | candidate | file | form |
|---|---:|---|---|---|
| 001639 | 157 | `nxIceIdentityPoses` | IceMeshTools.cpp | C++ |
| 001641 + 001643 | 61 + 433 | `nxIceEdgeLoop` | IceMeshTools.cpp | C++ |
| 001651 | 1,240 | `nxMeshNormalsCompute` | IceMeshTools.cpp | naked listing |
| 001653 | 1,618 | `nxIcePosePair` | IceMeshTools.cpp | naked listing |
| 001661 | 155 | `nxIceAddUniqueAxis` | IceMeshTools.cpp | naked listing |
| 002144 | 217 | `nxSmoothNormalsAngleAtVertex` | SmoothNormals.cpp | naked listing |
| 001461 | 194 | `ConvexHull::ComputeVertexNormals` | ConvexHull.cpp (new) | C++ |
| 002186 | 143 | `TriangleMesh::createAdjacencies` | TriangleMeshTopology.cpp (new) | C++ |
| 002188 | 152 | `TriangleMesh::createEdgeList` | TriangleMeshTopology.cpp (new) | C++ |

Also written: the product forms of two rows that were already `reconstructed` as ObjectModel.cpp
models. 001536 is `Adjacencies::Adjacencies` in IceAdjacencies.cpp; the oracle's linker folded it
with MeshNormals' constructor, which is inline in IceMeshTools.h. 001649 is
`MeshNormals::~MeshNormals`. Their states are unchanged; their notes record the product forms.

*Wiring.* The only candidate stand-in was SmoothNormals.cpp's C++ `angleAtVertex`, which modelled
002144 inside the reconstructed NxBuildSmoothNormals. It now calls the product row with the listing's
registers:
- the collision harness prints the same lines as before, step_smooth_normals included;
- its `.snan` variant still differs on the same 15 words under its ceiling, so they come from
  NxBuildSmoothNormals' own C++ and not from the angle.

No other row has a candidate caller:
- 001844, 001818, 001849, 002264, 002296, 001465, 001514, 001812, 001834, 001836 and 001859 are
  not written.
- Scene.cpp's inline store for the oracle's call of 0x0002dae0 on the scene's +0x50 member belongs
  to another class and was left as it is.

*Fidelity.*
- **Naked rows.** 001651, 001653, 001661 and 002144 are the listing's instructions, naked, as
  001712 is. `evidence/convex-mesh-gap-2e-listing-compare.py` shows that each built function in
  NxPhysics.dll equals its listing instruction for instruction (85, 445, 454 and 55 instructions):
  - branch targets are compared by index;
  - absolute operands (the float constants) are compared by the 4 bytes each image holds at that
    address, the oracle's in the pinned DLL and the candidate's in the built one (Task 2e review;
    first written masking them as "[abs]");
  - the two alignment `lea`s are emitted as their bytes.

  Each call reaches the candidate row of the same stable ID: the 004803 getter, 002144, the vendored
  005191 and 001591.
- **Calling conventions.** These are the oracle's:
  - 001651 and 001661 are thiscall with `ret 4` in the oracle. Naked code cannot be a member, and
    the vendored Container header is not changed, so both are `__fastcall` with an unused edx.
  - 002144 keeps its register convention: eax, edx and esi in, st(0) out, ebx and edi preserved.
  - 001653, 001641 and 001639 are cdecl.
  - 001461, 002186, 002188 and 001536 are thiscall members.
- **Frames.** No new function carries a /GS cookie or an unwind frame. ConvexHull.cpp and
  TriangleMeshTopology.cpp join the /EHs-c- list, as the MeshNormals local and the placement-new
  objects would otherwise get frames. This was checked in the DLL's disassembly.
- **Allocators.** Every allocation these rows make goes through the 004803 getter:
  - 001651's arrays, 001461's normals and 002186/002188's objects are type 0;
  - 001641's copy goes through the vendored Container, which also uses the getter.

  None goes through the CRT or the imported allocator, as in the listings.

*Listing findings* (reproduced; the contract is corrected):
- **001639** returns a 3x3 identity followed by a 4x4 identity (0x10123c7c, 100 bytes). It is not
  a 4x4 with six zero words.
- **001641 builds an outline, not only a dedupe.** After the pairs that occur twice are removed,
  it chains the rest into an outline and returns false when the chain breaks. It reads the first
  remaining pair without testing the count, so an input whose pairs all cancel reads stale words.
- **001651.** A failed vertex-normal allocation leaks the face normals it has just allocated. The
  weighted pass visits the corners in the order r0, r2, r1 and hands 002144 the triangle
  {r0, r2, r1}. A NaN sum is normalised, because the listing tests `jnp` after `test ah, 0x44`.
- **001653** has a dead store at 0x00031dc2.
- **001661** negates the direction when the sign bit of x is set, so -0 and negative NaNs are
  negated too. A NaN dot product never rejects.
- **002186 and 002188** call Init on their allocation without testing it. Each returns only what
  eax happens to hold.
- **TriangleMesh.** +0x84 is the Adjacencies and +0x88 is the EdgeList. The "three-way state" at
  +0x84 is 001859 caching 1 when 002186 fails.

**Differential** (`NxPhysicsThirdPartyTests`, through nxIceFamily: the recording 004803 allocator and
the report recorder; oracle rows by RVA, candidates on the same inputs; every float input written as
bits):

| family | rows | runs | words | result |
|---|---|---:|---:|---|
| pose_pair | 001653, 001639 | 4,608 calls | 127,828 | exact |
| pose_pair.inverse_divergent | 001653 | 736 calls | 24,288 | 944 NaN words, ceiling 944 |
| unique_axis | 001661 | 320 sequences | 13,830 | exact |
| edge_dedupe | 001641/001643 | 216 cases | 30,416 | exact |
| mesh_normals | 001651 (001536, 001649, 002144), 001461 | 381 + 139 cases | 36,260 | exact |
| adjacency_owner | 002186 (001536), 002188 | 66 meshes | 28,728 | exact |
| adjacency_owner.plane_divergent | 002188 | 13 meshes | 6,300 | 155 discrete words, ceiling 155 |

- **pose_pair.**
  - Poses are null, clean, raw, a raw rotation at the origin, and rows (x, x, y) or (x, -x, y) at
    the origin. A clean pose is a signed permutation scaled by drawn finite words, with a finite or
    infinite translation.
  - Either output is requested, under both control words.
  - The vendored InvertPRMatrix inverts every non-raw kind exactly: one product, a transposed copy,
    or a sum of zero products.
  - The last two kinds make each element's two large products cancel, so the listing's
    per-element term order is visible.
  - A call that uses the inverse of a raw pose goes to `.inverse_divergent` by a rule on the fixed
    inputs. Every word that differs there is a NaN on both sides, and all of them trace to the
    vendored 005191, which copies through the FPU (quieting signalling NaNs) where the oracle copies
    as integers (0 with the oracle's bound in, below). 276 are that signalling NaN itself, quieted on
    the candidate side; the other 668 are quiet NaNs on both sides that differ only in sign (252),
    payload (140) or both (276): the quieted inverse meets other NaNs downstream in 001653's
    products, and x87 propagation picks a different operand. (Classified over all 944 words from
    the harness's ULP_BEYOND detail with its 160-line cap lifted for one throwaway run; Task 2e
    review.)
  - 001639 is compared on its block, on the same pointer from a second call, and on a third call
    over a poisoned block, which must leave the poison (the guard).
- **unique_axis.**
  - Pairs straddle 0.9999 by margins of 1e-11..1e-8. They were computed offline over float words
    and are fixed in the harness, together with the exactly-equal pair and one word above it.
  - Their negations, doublings and last-bit nudges are made by bit operations.
  - Raw words: 320 signalling-NaN input words.
- **edge_dedupe.**
  - Triangle fans over 3..16 drawn vertices, shuffled, flipped, with a stray edge, a doubled
    boundary edge or an internal edge three times.
  - The face edges of every mesh of at most 100 faces (O(n^3) past that), in order and shuffled.
  - An input whose pairs all cancel is not driven (4 cases). This is a rule on the fixed input.
- **mesh_normals.**
  - Each mesh runs under rotating create blocks: 32-bit, 16-bit or no faces; weighted or not;
    caller or object arrays. The six fixtures run under all 24.
  - The vertex words are raw-mixed (nxMb2RawWords) for half the cases: 1,089 signalling-NaN words.
  - Cases alternate between the control words.
  - 001461 runs over hull images, with and without a previous array, and with no vertices.
- **adjacency_owner.**
  - TriangleMesh images of every mesh, and one with no triangles.
  - The object built, the release path on failure, and every report: 29 non-manifold at line 321,
    and EdgeList's 0x72.
  - 002188's EdgeList computes active edges from the vertices, so its runs on Task 2c's 13 frozen
    meshes go to `.plane_divergent`.

*Attribution of the two splits.* `evidence/convex-mesh-gap-2e-bind-oracle-callees.patch` applies
with `git apply` to 5aad0c8. It extends Task 2c's patch with a hook in 001653 for the oracle's 005191
at base+0x000e4200 (NX_ICE_BIND bit 4). With NX_ICE_BIND unset, rebuild NxPhysicsThirdPartyTests
and run it, then set the variable to 3, 4 and 7 in turn. The four `.plane_divergent` /
`.inverse_divergent` lines read:

| NX_ICE_BIND | edge_list | ice_adjacencies | pose_pair | adjacency_owner |
|---|---:|---:|---:|---:|
| unset (0) | 465 | 124 | 944 | 155 |
| 3 | 0 | 0 | 944 | 0 |
| 4 | 465 | 124 | 0 | 155 |
| 7 | 0 | 0 | 0 | 0 |

So every word in the two new splits belongs to the vendored callees. Never commit the patch applied:
product code must not reach the oracle.

*Sensitivity* (throwaway mutations, one per x87 row, each committed as
`evidence/convex-mesh-gap-2e-<row>-mutation.patch`). To reproduce: `git apply` the patch, rebuild
NxPhysicsThirdPartyTests (and NxPhysicsCollisionTests for 002144), run, then `git checkout` the file.

| row | mutation | result |
|---|---|---|
| 002144 | `faddp` -> `fsubp` at 0x000533a5 (the dot product) | mesh_normals 4,311 mismatches; step_smooth_normals 53,653 |
| 001651 | `jnp` -> `jp` at 0x00031ab0 (the face-normal normalisation) | mesh_normals 15,539 |
| 001653 | first and last terms of relative0[0][0] swapped (0x00031e6e/72 <-> 0x00031e87/8b) | pose_pair 32 |
| 001661 | the `fchs` of y at 0x0003251b removed | unique_axis FAILED: its tape is 14,140 words against 13,830, as different directions are kept |

- The 001653 mutation first moved nothing (a0622c2): with the earlier inputs, an element's order of
  terms could not change a float. The cancelling pose kinds were added for that (5aad0c8).
- The 002144, 001651 and 001661 counts were measured on a0622c2. The families they touch did not
  change afterwards.

*Registered lines.* 19 lines, all copied from the oracle side of the run on 5aad0c8:
- per family, the input digest, the exact line whole and the coverage line;
- the two divergent lines, up to the oracle digest;
- the totals pair `driven=85 divergent=29 words=2225551` / `a1b4f9a6`.

The name lines' agreement fields are the gate's assertion. The coverage lines count oracle-side
values and fixed-input rules only. The pose_pair lines and the totals pair were first registered
from a0622c2 (`words=77668 oracle=56567247`, `2174863` / `d9a2accc`) and re-registered once, from
5aad0c8, after the cancelling kinds were added. The phase 4 floor goes from 183 to 202, and the
MINIMUM pin with it. Every line registered before this task is unchanged: the families run last,
and their seeds are their own. The runs are deterministic, and `--self` gives the same oracle
digests.

**Trace.** `evidence/convex-mesh-gap-trace-2e.txt` records one-shot cdb breakpoints on 11 candidate
functions: the nine rows with an address of their own, and the product forms of 001536 and 001649.
All 11 were hit in one full run of the build of 5aad0c8, which closed
`thirdparty candidate mismatches=0` (exe sha256 1a3476b96506cb0e...).

**Inventory and ledgers.**
- The 10 rows are now `reconstructed`, with static proofs that cite the listing ranges (and, for
  the naked rows, the comparison script) and dynamic proofs that cite the trace and the family.
- 001536 and 001649 get notes.
- Ledgers, with the standard note:
  - phase 2: 001661 moves from homeless_shared_code to reconstructed_not_falsified (48 -> 47,
    10 -> 11);
  - phase 3: 001461, 001641, 001643 and 001651 move from not_reconstructed_in_phase, and 001653
    from blocked_on_later_phase, to reconstructed_not_falsified (300 -> 296, 18 -> 17, 13 -> 18);
  - phase 4: 002144, 002186 and 002188 (260, 508);
  - phase 7: 001639 (361, 196).
- The reason texts are updated. `validate_inventory.py` asked only that the ConvexHull.cpp
  unresolved-source allowlist entry be removed.
- The four mutated rows keep the standard note: the mutations above are evidence, not the
  ledgers' closure procedure.

**Review (cleanups).**
- *Ledger notes of the mutated rows.* 001661 (phase 2), 001651 and 001653 (phase 3) and 002144
  (phase 4) had a mutation aimed at them and caught, so the standard note ("no mutation has been
  aimed at this row") was wrong for them. `oracle_differential_falsified` is not available: the
  validator rejects a differential closure on a row the inventory leaves below `dynamically_gated`
  (validate_inventory.py, `validate_closure`), and this plan promotes no row above `reconstructed`.
  Earlier tasks of this plan left their caught mutations (2d's fld/fstp, 276) out of the ledgers
  altogether. Decision: the four rows keep `reconstructed_not_falsified` and their notes now say
  the mutation was caught, cite the patch and the count, and say why the row stays deferred; the
  phase 2, 3 and 4 reason texts name them as the exception. For the notes the four mutations were
  re-measured in a throwaway `git archive` copy of 14c8ec7 (D:/nx2e), NxPhysicsThirdPartyTests
  rebuilt after each `git apply` and after each `git apply -R`, with an un-mutated control before
  the first and after each: every control `thirdparty candidate mismatches=0`, and
  `thirdparty oracle digest=a1b4f9a6` in all nine runs. Counts: 002144 mesh_normals 4,311; 001651
  mesh_normals 15,539; 001653 pose_pair 32; 001661 unique_axis 1 (the tape length). 001651's
  phase 3 deferral named no later phase (`driving_phases: []`), so it could not have been
  discharged by phase 4 even at `dynamically_gated`; 001661 ([3, 4]) and 001653 ([4, 5, 7]) could.
- *Listing comparison.* `convex-mesh-gap-2e-listing-compare.py` compares each absolute memory
  operand by the bytes it reads in its own image (the oracle's pinned DLL, the candidate's built
  DLL), not as a mask. Re-run on the build of 14c8ec7: 85 / 445 / 454 / 55 instructions, 0 differing
  (0.0f, 1.0f and 0.9999f's 0x3f7ff972 on both sides).
- *pose_pair.inverse_divergent wording.* See the corrected bullet above: 276 of the 944 words are the
  quieted signalling NaN itself and 668 are quiet NaNs on both sides differing in sign or payload
  downstream (the first wording said every word was a quieted NaN).
- *002186 / 002188.* Calling Init on the unchecked allocation, as the listing does, is undefined
  behaviour in C++ when the allocation fails, which would let a compiler drop the null test on the
  release path. The build of 14c8ec7 keeps it (`test ecx, ecx` before the release in both functions
  of NxPhysics.dll, 0x10037a13 and 0x10037aaa); TriangleMeshTopology.cpp says so at both calls. The
  families cannot see it: the recording allocator never fails.

## Task 2f: P-Hull and the support maps

**Rows (25, 6,875 B), all `discovered` -> `reconstructed`.** P-Hull is 16 rows, 4,986 B: the
contract's nine, plus seven continuations its row list lacked (001498, 001500 of 001496; 001504,
001506, 001508, 001510, 001512 of 001502; 1,879 B). Sub-unit B is 9 rows, 1,889 B.

| row | bytes | candidate | file | form |
|---|---:|---|---|---|
| 001441 | 178 | `nxHullTriangleArea` | ConvexHull.cpp | naked listing |
| 001445 | 146 | `nxHullTriangleCenter` | ConvexHull.cpp | naked listing |
| 001449 | 156 | `nxHullGatherFaces` | ConvexHull.cpp | C++ |
| 001459 | 241 | `nxHullComputeCentroid` | ConvexHull.cpp | naked listing |
| 001463 | 337 | `nxHullPolygonPlane` | ConvexHull.cpp | naked listing |
| 001465 | 791 | `nxHullExtractPolygons` | ConvexHull.cpp | C++ |
| 001472 | 664 | `nxHullComputePolygons` | ConvexHull.cpp | naked listing |
| 001496 + 001498, 001500 | 296 + 217 + 66 | `nxHullSupportPolygon` | ConvexHull.cpp | naked listing |
| 001502 + 001504..001512 | 298 + 1,596 | `nxHullComputeEdges` | ConvexHull.cpp | naked listing |
| 001550 | 131 | `nxSupportMapCubeFace` | IceSupportMaps.cpp (new) | naked listing |
| 001556 | 196 | `nxSupportMapLookup` | IceSupportMaps.cpp | naked listing |
| 001558 + 001560 | 122 + 478 | `nxSupportMapInit` | IceSupportMaps.cpp | naked listing |
| 001567 | 63 | `nxSupportMapHullAllocate` | IceSupportMaps.cpp | C++ |
| 001569 | 34 | `nxSupportMapHullCompute` | IceSupportMaps.cpp | C++ |
| 001573 | 323 | `nxSupportMapPlaneCompute` | IceSupportMaps.cpp | naked listing |
| 001579 | 70 | `nxSupportMapVertexAllocate` | IceSupportMaps.cpp | C++ |
| 001581 | 472 | `nxSupportMapVertexCompute` | IceSupportMaps.cpp | naked listing |

Also written, states unchanged (notes record them):
- the element constructors the naked `new[]` sites push, 000925 (`nxHullPolygonConstruct`),
  001391 (`nxIceIdentityConstruct`) and 001439 (`nxEdgeDescConstruct`), naked; 001657
  (`nxIceReverseArray`) in IceMeshTools.cpp, C++;
- 000001, MSVC's `vector constructor iterator` (`nxIceVectorConstruct`, naked), which 001472 and
  001502 call where the listing does. It stays `discovered`: the contract names it a compiler
  helper, not a prerequisite, and it is outside this task's rows;
- the product forms of the eleven support-map rows the census had closed as ObjectModel.cpp
  models: 001552, 001554, 001563, 001565, 001571, 001575, 001577, 001583 (a naked `ret`), 001585,
  001587, 001589. The four tables (gIceSupportMapBaseTable, ...HullTable, ...PlaneTable,
  ...VertexTable) are arrays of their slots, as the image's are.

*Wiring.* The one candidate stand-in was the model of 001407 (`MeshShape::nxMeshSweepPrepared`,
ObjectModel.cpp), which inlined the lookup and required the hull's polygons to be built. It now
calls the product 001556 and, when +0x28 is null, the product 001472, where the listing does
(0x000296a7, 0x000296c8). Every target that compiles ObjectModel.cpp now links the ICE rows
these reach (NxPhysicsShapeVtableTests in 20e37e4; NxPhysicsInternalTests, NxPhysicsObjectLayoutTests
and NxPhysicsCollisionTests in 45a4d16, after the first gate run found them unlinked), and
NxPhysicsShapeVtableTests prints `shape vtable oracle_digest=ed1294b6 cases=626 failures=0`, as
before. 002255 and 002249 (which
construct and read the maps) and the other hull callers (001822, 001844, the polygon interface)
are not written. The layout models of 001565/001571/001575 stay in ObjectModel.cpp: they write
the oracle's table addresses for the object-layout tests and cannot point at product tables.

*Fidelity.*
- **Naked rows.** 17 functions (the 12 x87 rows, 000001, 000925, 001391, 001439, 001583) were
  generated from the listing. `evidence/convex-mesh-gap-2f-listing-compare.py` shows each built
  function in NxPhysics.dll equal to its listing extent instruction for instruction (continuations
  and alignment fillers included): 22, 6, 2, 6, 63, 45, 82, 115, 240, 196, 645, 41, 63, 183, 108,
  169 and 1 instructions, 0 differing. Branch targets are compared by index; absolute operands by
  the bytes they read in each image (0.0f, 1.0f, 0.5f, 1/3's 0x3eaaaaab, 2^32, FLT_MAX, and the
  doubles 1e-7 / -1e-7); pushed constructor addresses and calls by the candidate function they
  name (each is the row of the same stable ID, or the vendored member).
  - *Why naked:* every float row keeps values on the x87 stack across stores that narrow, and
    001556/001558 convert through `fild` with the 2^32 fix-up and `fistp qword`; 001502 is mostly
    integer but interleaves six allocations, two sorts and an x87 normalisation, so it is one
    transcription.
  - *Vendored members from naked code:* MSVC's inline assembler cannot name a member function, so
    Container's constructor and destructor, RadixSort's constructor, destructor and Sort, and
    Triangle::Area and Plane::Set are called through `/alternatename` aliases of their decorated
    names; the call is the same direct call to the same function.
  - *001558's switch table:* the image's table is at 0x1002e550, just after the row. MSVC's inline
    assembler cannot emit code addresses as data, so gIceSupportMapFaceCases in .rdata holds the
    row's address plus the offsets of its three case labels (0x90, 0xef, 0x14c, the oracle's own;
    `--offsets` prints them); the script compares the six entries by the instruction each lands on.
  - *001583:* the inline assembler writes `ret` there as `ret 0`; it is emitted as its byte.
- **Calling conventions.** The oracle's: the thiscall rows are `__fastcall` with an unused edx
  (naked code cannot be a member; `ret 4` / `ret 8` as the listing's), which is thiscall's
  convention for the callee, so the tables' slots are called by 001558 exactly as the oracle's are;
  001449, 001463, 001465, 001550 and 001657 are cdecl; 000001 stdcall.
- **Frames.** No new function has a /GS cookie or an unwind frame (checked in the DLL: none of the
  16 C++ functions loads `__security_cookie` or touches fs:); the naked ones have neither by
  construction. IceSupportMaps.cpp joins ConvexHull.cpp on the `/EHs-c-` list; both are on the
  `/arch:IA32` list (ConvexHull.cpp was not before).
- **Allocators.** Every allocation is the 004803 getter's, as in the listings: 001472's polygons
  (`new[]`, cookie) and references (type 0); 001502's four temporaries (type 1), edges (`new[]`,
  cookie, twice), edge numbers, EdgeDescs (no cookie), polygon table and normals (type 0); the maps'
  bytes (type 0). None goes through the CRT or the imported allocator.

*Listing findings* (the contract is corrected):
- 001496's stack arguments are (direction, pose) and it returns a polygon index; 001569 calls it
  with no pose. 001472 computes the centre into a local and never stores the hull's +0x18.
- 001465 reports a broken outline through 002160 with no message (0, file, 318) and returns the
  report's result (false on both sides under the harness's report recorder).
- 001502 calls itself when +0x3c, +0x44 or +0x48 is still null after it built them (unreachable).
- UB mirrored from the oracle, not driven:
  - 001641 (called by 001465) reads the first pair of an empty edge Container, so a face group
    with no active edge (an inner shell whose edges are all concave, a mesh wound inward, a NaN
    coordinate) is a null read in the oracle (0x00031537) as here. The harness never drives such a
    mesh: drawn 3 x 3 x 3 fills keep the centre voxel, the lattice tables all increase, and the
    split meshes are nudged, never given raw words.
  - 001502 over a hull whose polygons failed writes 8 bytes into the zero-byte block it allocates
    for the EdgeDescs and reads 6 bytes before it (0x0002d067, 0x0002d0a1). The harness calls
    001502 only after the side's own 001472 built polygons, and calls 001502 alone only on main
    convex sets whose 001472 cannot fail.

**Differential** (`NxPhysicsThirdPartyTests`, through nxIceFamily: the recording 004803 allocator
and the report recorder; oracle rows by RVA, candidates on the same inputs; every float input
written as bits):

| family | rows | words | result |
|---|---|---:|---|
| hull_leaf | 001441, 001445, 001459, 001496 (hand-built polygons), 001449, 001657, 000001 with 000925 / 001391 / 001439 | 21,813 | exact |
| convex_hull | 001472, 001502, 001496 (built hulls), 001463; through them 001465, 001449, 001459, 001441, 001445, 001657 | 20,691 | exact |
| convex_hull.plane_divergent | the same rows on the split inputs | 56,808 | 1,595 words (52 discrete), ceiling 1,595 / 52 |
| support_maps | 001550, 001556, 001558/001560, 001565..001589 through the tables | 26,162 | exact |

- **hull_leaf.** 900 triangles of eight drawn vertex words (one in four raw: signalling and quiet
  NaNs, infinities, denormals, -0), null vertices every 45th; 400 hull images for 001459 (no
  vertex count, no vertex array, no faces); 1,500 polygon arrays for 001496 with drawn plane words
  and tied normals, half with a drawn pose; 300 link-word sets for 001449; 001657 with counts 0..9
  and a null array; 000001 with each constructor, counts 0..4 and -1. 7,706 signalling-NaN input
  words; both control words.
- **convex_hull.** 90 polycube meshes: the boundary squares of voxel sets, each split on a drawn
  diagonal, the lattice mapped through tables of increasing short-mantissa words (every pairwise
  difference has at most six significant bits, so every product the vendored rows form is exact).
  Shapes: boxes, separate cubes (the centre lies outside their facing planes: 001472's flip), a ring
  (001465's line-318 report), two voxels on an edge (the non-manifold report, line 321), open meshes
  (a boundary: false), concave sets. 001502 after 001472, or alone on every fifth buildable mesh;
  a second 001472 and 001502 on every third (the releases); 24 directions through 001496 on every
  built hull. 700 direct 001463 cases: n references (0..12) into 16 points, in the main set the
  corners of a rectangle a power of two wide on each in-plane axis.
  - *The split is a rule on the fixed inputs:* a mesh goes to `convex_hull.plane_divergent` when
    its voxel set is concave (its concave edges are inactive, so faces across them join a polygon
    that is not planar, whose plane 001463 takes through a skew triangle), when its words were
    nudged (rounds 2 and 3), or when it runs under 0x0f7f (round 4, the same lattice words: under
    chop rounding the vendored Plane::Set normalises (0, c, 0) to 0x3f7ffffe where the oracle's
    005155 gives 0x3f7fffff whenever 1/c is inexact); a 001463 case goes there when its points are
    drawn words. Main meshes run under 0x027f; the direct 001463 cases under both.
- **support_maps.** 2,400 drawn directions through 001550 and 001556 (n = 0..20, ties of magnitude
  and axis directions); A, B and C built by each side's constructors over the main meshes' hulls
  (built by each side's own 001472) and over 30 hand-built hull images of drawn plane, centre and
  vertex words; Init for n = 0, 1, 2, 3, 5, 8 (and 1..6 on the images), the bytes, 12 lookups
  through each map, slot 0 through the object's own table with the free bit for odd n; the
  allocation failures (256 polygons for A and B, 256 vertices for C).
- **Stability.** The runs are deterministic, `--self` gives the same oracle digests, and every
  earlier thirdparty line is unchanged (the families run last, with seeds of their own).

*Attribution of the split.* `evidence/convex-mesh-gap-2f-bind-oracle-callees.patch` applies with
`git apply` to 0701abc. It is Task 2e's patch (the 002061 hooks for 005155 / 005181, bits 1 and 2;
001653's for 005191, bit 4) plus hooks in 001463 for the oracle's 005179 and 005155 (bit 8). With
it applied, rebuild NxPhysicsThirdPartyTests and run it with NX_ICE_BIND unset, 3, 8 and 11; then
`git checkout` the four files:

| NX_ICE_BIND | convex_hull.plane_divergent | edge_list / ice_adjacencies / adjacency_owner .plane_divergent |
|---:|---:|---|
| 0 | 1,595 | 465 / 124 / 155 |
| 3 | 1,595 | 0 / 0 / 0 |
| 8 | 0 | 465 / 124 / 155 |
| 11 | 0 | 0 / 0 / 0 |

Every split word follows 001463's two vendored callees; the EdgeList decisions (bits 1 and 2) do not
move it. Never commit the patch applied: product code must not reach the oracle.

*Sensitivity* (one throwaway mutation per x87 row, each committed as
`evidence/convex-mesh-gap-2f-<row>-mutation.patch`). To reproduce: `git apply` the patch to 0701abc,
rebuild NxPhysicsThirdPartyTests, run, then `git checkout` the file. Measured on the build of
0701abc between un-mutated controls before the first and after the last (both `thirdparty candidate
mismatches=0`, every oracle-side line unmoved):

| row (patch) | mutation | hull_leaf | convex_hull | support_maps |
|---|---|---:|---:|---:|
| 001441 | `faddp` -> `fsubp` at 0x0002a6b8 | 607 | 219 | 246 |
| 001445 | `fadd` -> `fsub` at 0x0002a7d3 | 635 | 320 | 237 |
| 001459 | `fadd` -> `fsub` at 0x0002adf7 | 42 | 448 | 262 |
| 001463 | `jne` -> `je` at 0x0002b006 | 0 | 432 | 0 |
| 001472 | the `fchs` at 0x0002b898 removed | 0 | 87 | 230 |
| 001496 | `jne` -> `je` at 0x0002ca43 | 898 | 266 | 1,154 |
| 001502 (in 001512) | `jnp` -> `jp` at 0x0002d26b | 0 | 638 | 0 |
| 001550 | `fdivr` -> `fdiv` at 0x0002e1ac | 0 | 0 | 5,723 |
| 001556 | `jne` -> `je` at 0x0002e2bf | 0 | 0 | 4,261 |
| 001558 (in 001560) | `fsubr` -> `fsub` at 0x0002e3b8 | 0 | 0 | 853 |
| 001573 | the `fchs` at 0x0002e770 removed | 0 | 0 | 983 |
| 001581 | the `fchs` at 0x0002e8fd removed | 0 | 0 | 437 |

Every mutation is caught by an exact family on first measurement; no input had to be strengthened.
The ledger notes of the twelve rows (001512 and 001560 for the two continuations) cite them.

*Registered lines.* 12 lines, all copied from the oracle side of the run on 0701abc:
- `thirdparty input name=hull_leaf words=181380 input=9279a52e`
- `thirdparty name=hull_leaf ... words=21813 oracle=106d7907 mismatches=0 worst_ulp=0 verdict=exact`
- `thirdparty coverage name=hull_leaf area=900 center=900 centroid=400 centroid_true=379 support=1500 support_posed=752 gather=300 gather_faces=1639 reverse=24 vector=18 input_snan=7706 x87_0f7f=450 reports=0`
- `thirdparty input name=convex_hull words=70348 input=2ebe9920`
- `thirdparty name=convex_hull ... words=20691 oracle=64a1d9bd mismatches=0 worst_ulp=0 verdict=exact`
- `thirdparty name=convex_hull.plane_divergent ... words=56808 oracle=87dd3578`
- `thirdparty coverage name=convex_hull meshes=90 split_meshes=68 polygons_true=53 polygons_false=34 edges_alone=3 polygons=469 edges_true=56 edges_false=0 edges=1495 rebuilt=22 support=1344 plane=700 plane_split=175 reports=24 line318=9 line321=15`
- `thirdparty input name=support_maps words=26184 input=caa5f9eb`
- `thirdparty name=support_maps ... words=26162 oracle=79d50caf mismatches=0 worst_ulp=0 verdict=exact`
- `thirdparty coverage name=support_maps faces=2400 lookups=2400 maps=138 init_true=135 init_false=3 samples=11844 map_lookups=1620 freed=66 input_snan=625 reports=4`
- `thirdparty coverage driven=89 divergent=30 words=2351025 layout_checks=47`
- `thirdparty oracle digest=cfa5d10e`

The coverage lines count oracle-side values and fixed-input rules only. The phase 4 floor goes from
202 to 214, and the MINIMUM pin with it. `kDivergentCeilings` holds
`convex_hull.plane_divergent` at 1,595 words, 52 discrete.

**Trace.** `evidence/convex-mesh-gap-trace-2f.txt` records one-shot cdb breakpoints on the 33
candidate functions carrying this task's stable IDs. 32 were hit in one full run of the build of
0701abc, which closed `thirdparty candidate mismatches=0` (exe sha256 af76122099e40130...). The one
not hit is 001563, the base table's deleting destructor: no map is destroyed while it holds the
base table.

**Gates** (on 45a4d16 with this commit's inventory): phases 2, 3 (243/243), 4 (214/214), 6 (403)
and 7 (276) pass; phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables` through
NxPhysicsObjectLayoutTests (871/871; shape vtable 626/0). Tool tests 763 OK; validate_inventory.py
passes.

**Inventory and ledgers.**
- The 25 rows are now `reconstructed`, with static proofs that cite the listing ranges (and, for
  the naked rows, the comparison script) and dynamic proofs that cite the trace and the family.
- Notes: 000001, 000925, 001391, 001439, 001657, the eleven support-map product forms, and 001407
  (its model now calls 001556 and 001472).
- Ledgers, every moved row to reconstructed_not_falsified (the standard note, or the caught
  mutation's): phase 2 homeless_shared_code 47 -> 39 and 11 -> 19 (001472, 001502..001512 with its
  continuations, 001556); phase 3 not_reconstructed_in_phase 296 -> 290 and 18 -> 24 (001441,
  001445, 001449, 001459, 001463, 001465); phase 4 not_reconstructed_in_phase 260 -> 249 and 508 ->
  519 (001496, 001498, 001500, 001550, 001558, 001560, 001567, 001569, 001573, 001579, 001581).
  The reason stays reconstructed_not_falsified for the mutated rows too: a closure needs the
  inventory at `dynamically_gated`, and this plan promotes no row above `reconstructed`.
