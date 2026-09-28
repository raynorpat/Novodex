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
| H review | 2026-09-28T13:00:00 | 2026-09-28T13:45:53 | 0 | 0 | Task H review fixes: the separating-axis byte of contact_box_box and contact_box_capsule tallied into their `.snan` ceilings (346 / 346, 78 / 78, 236 / 236 and 99 / 93, 7 / 7, 40 / 40); every remaining unsequenced multi-draw expression in the collision, fuzz, third-party and tangent harnesses sequenced in the order the build used (all registered lines and the geometry transcript unchanged); float-returning raw-bit helpers removed from all harnesses and the tool test widened to every tests/*.cpp; ray_inflated_tris' divergent cause no longer names 001712. See `## Harness hardening`. |

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
