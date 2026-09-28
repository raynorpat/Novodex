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
