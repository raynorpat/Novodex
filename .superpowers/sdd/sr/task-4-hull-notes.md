# Task 4 (hull facade rows): notes

Branch `claude/sr-t4-hull` from `claude/scene-raycast-block` bb2e485. Files: `Physics/src/ObjectModel.cpp`,
`Physics/src/include/ObjectModel.h`, `tests/PhysicsShapeVtableTests.cpp`. Not touched (per the assignment):
inventory.json, ledgers, contract, gate_targets.ps1, timing table.

Listings were walked with Capstone 5.0.6 over the pinned DLL (sha256 4b7db3e1...; the manifest's image), Ghidra
decompiles and the unit bundle `units/gap__SceneRaycast.cpp__to__CapsuleShape.cpp.md` as cross-checks only.

## Rows

| Row | RVA, B | Candidate (ObjectModel.cpp) | Listing walked | ABI (row = candidate) |
|---|---|---|---|---|
| 000973 | 0x21420, 913 | `BoxShape::nxBoxRebuildHull` | 0x10021420-0x100217b0 | ecx = the box, no stack arguments, plain `ret` (Ghidra's __fastcall(int)); member with no arguments |
| 000957 | 0x20d40, 578 | `BoxHullFacade::supportFace` (facade slot 9) | 0x10020d40-0x10020f7f | __thiscall on the facade (box+0xe0), arg1 direction, arg2 optional pose, `ret 8`, eax = face index |
| 000959 | 0x20f90, 1062 | `BoxHullFacade::supportFeature` (facade slot 10) | 0x10020f90-0x100213b3 | __thiscall on the facade, arg1 direction, arg2 optional pose, arg3 optional NxU32 out, `ret 0xc`, eax = face index |
| 000981 | 0x21990, 55 | `BoxShape::nxBoxLoadFromDesc` (BOX slot 12) | 0x10021990-0x100219c4 | __thiscall, arg1 record, `ret 4`, al = 0x27740's al (now `bool`, was `void`) |

`nxBoxHullFacadeVtable` slots 9 and 10 now hold `supportFace`/`supportFeature` (were null). The candidate's 000981 in
the DLL disassembles to the image's shape: three dword copies, `call` 000973, `push record; mov ecx,this; call`
0x27740's reproduction, `ret 4`.

New data (the image's words, dumped from the pinned DLL): the face index lists .rdata 0x10106998..0x101069f7
(`gHullFaceCorners[6][4]`) and 0x101069f8..0x10106a57 (`gHullFaceEdges[6][4]`); the twelve edge directions
.rdata 0x101220f0..0x1012217f (`gHullEdgeDirections`, +-0x3f3504f3 and 0). `BoxFaceRecord`'s six anonymous float
words are now named: normal[3] (+0x0c), distance (+0x18), min (+0x1c) and max (+0x20) projection.

### 000973 (what is reproduced)
- AABB about a zero centre with half-extents +0xe4 (000923 call at 0x10021451), eight corners to +0xf0 (005147
  AABB::ComputePoints at 0x10021461; the candidate calls the vendored IceAABB.cpp function out of line).
- List pointers (0x10021466-0x100214d4), corner counts 4 (0x100214e3-0x10021501).
- Normals in store order: face 1 (+1,0,0), 3 (-1,0,0), 4 (0,+1,0), 5 (0,-1,0), 2 (0,0,+1), 0 (0,0,-1).
- Distances -((n_a + n_b) * 0.0f + s * n_c) per face with the listed operand pairs and the x87 lifetimes of s: dx,
  -dx and dy are float spills (0x10021623/33/43), -dy, dz, -dz stay in registers (0x10021507-0x10021725).
- Loop through the facade's own table (+0xe0): slot 3 before the loop and after each face, slots 1/2 per face,
  FLT_MAX/-FLT_MAX seeds, per corner (vz*nz + vy*ny) + vx*nx unrounded; min on strictly less (test ah,5/jp), max on
  strictly greater (test ah,0x41) (0x1002172b-0x100217a7).

### 000957 / 000959 (what is reproduced)
- Pose words 0-2, 4-6, 8-10 copied to a local 3x3; direction rows spilled to float. 957 groups each row
  (m0*d0 + m1*d1) + m2*d2 (0x10020dad-0x10020e01); 959 groups (m0*d0 + m2*d2) + m1*d1 (0x10020ffc-0x10021050).
- Face count through the facade's slot 3 (0x10020e23 / 0x10021072).
- Face 0 seeds the best as (d2*nz + d1*ny) + d0*nx, unrounded in the register; replacement only on strictly greater
  (fcomp, test ah,0x41); the replacing value is the float spill reloaded (fst [arg slot]; fld).
- Unrolled x4 when (int)(count-1) >= 4, repeated while i < count-3 (unsigned). 957: all four
  (d0*nx + d2*nz) + d1*ny; 959: first (d2*nz + d0*nx) + d1*ny, the other three (d2*nz + d1*ny) + d0*nx. Tail loop
  (both) (d2*nz + d0*nx) + d1*ny.
- 959 only: the best continues over the twelve edge directions, six per pass, (d0*x + d2*z) + d1*y
  (0x100211d2-0x100212f6). No edge: out = 0, return the face. An edge: out = 1, slots 6 (result unused), 7 and 8
  called in that order; faces A = adj[w], B = adj[w+1] with w = slot7table[2*edge+1]; projections
  (d2*nz + d1*ny) + d0*nx kept unrounded; return A iff B < A (fcompp, test ah,5, jp), else B (unordered -> B;
  Ghidra's decompile returns A on NaN, the listing does not).

### Divergences (code shape, none observable in the differential)
- 000973: the candidate inlines IceAABB.h `SetCenterExtents` (SSE2 subss/addss, exact for 0 -/+ e) where the image
  calls the ICF-folded out-of-line copy 000923; a candidate trace will show no 000923 hit from 000973. The image's
  NxVec3 temporaries on the stack for the normal stores (L10..L18) are not reproduced (same stored words).
- 957/959: the dead zero stores to a local (0x10020d4e, 0x10020fa1) are not reproduced; the x4 unroll is written as
  an inner loop with the same per-position grouping.
- Precision: ObjectModel.cpp keeps the default architecture in the DLL (SSE2). All register lifetimes are `double`,
  spills `float`; at API time (0x027f, 53-bit) that is the x87's arithmetic. The only NaN-payload risk (two-NaN
  operations) reaches only compares, never a stored word. No step caller found: a scan of every inventory code row for
  `lea/add reg,+0xe0` followed by a call through the table or a push of the register finds only 000973 itself (the
  other +0xe0 pushes are plane/mesh/stack uses: 000651, 001247, 001779, 001861, 001891, 001893, 001895, 004206).
  957/959 have no direct callers and no found indirect callers in the image: reached only through the table.

## Ready-to-paste static_proof sentences
- 000973: "Listing 0x10021420-0x100217b0 walked (Capstone): BoxShape::nxBoxRebuildHull (ObjectModel.cpp), ecx = the box, no stack arguments, plain ret. Reproduced: the zero-centre AABB of half-extents +0xe4 and its eight corners to +0xf0 (000923 at 0x10021451, inlined in the candidate as IceAABB.h SetCenterExtents; 005147 AABB::ComputePoints called at 0x10021461), the twelve list pointers to the image's index lists (.rdata 0x10106998/0x101069f8), the corner counts, the axis normals in store order, each plane distance -((n_a + n_b) * 0.0f + s * n_c) with the listing's operand pairs and float spills (dx, -dx, dy spilled; -dy, dz, -dz in registers), and the per-face min/max projection loop through the facade's own slots 3, 1 and 2 with FLT_MAX seeds and the fcom predicates (strictly less / strictly greater). Not reproduced: the out-of-line call to 000923 (inlined) and the NxVec3 stack temporaries of the normal stores. Runs at API time (shape creation through 000981 and setDimensions through 000983) under 0x027f."
- 000957: "Listing 0x10020d40-0x10020f7f walked (Capstone): BoxHullFacade::supportFace (ObjectModel.cpp), facade slot 9, __thiscall ret 8 (direction, optional pose), installed in nxBoxHullFacadeVtable slot 9. Reproduced: the pose rows at words 0-2/4-6/8-10 applied as (m0*d0 + m1*d1) + m2*d2 and spilled to float, the face count through the facade's slot 3, the unrounded face-0 seed (d2*nz + d1*ny) + d0*nx, replacement on strictly greater by the float spill of the new value, the x4 unrolled run (count-1 >= 4 signed, while i < count-3) summing (d0*nx + d2*nz) + d1*ny and the tail (d2*nz + d0*nx) + d1*ny. Not reproduced: a dead local zero store (0x10020d4e). The image has no direct caller; no indirect caller was found."
- 000959: "Listing 0x10020f90-0x100213b3 walked (Capstone): BoxHullFacade::supportFeature (ObjectModel.cpp), facade slot 10, __thiscall ret 0xc (direction, optional pose, optional out word), installed in nxBoxHullFacadeVtable slot 10. Reproduced: the pose rows as (m0*d0 + m2*d2) + m1*d1 spilled to float, the face search of slot 9 with this row's groupings (unrolled first (d2*nz + d0*nx) + d1*ny, the other three (d2*nz + d1*ny) + d0*nx), the continuation over the twelve edge directions of .rdata 0x101220f0 as (d0*x + d2*z) + d1*y, the out word (0 face / 1 edge), the slot 6, 7, 8 calls and the adjacent-face choice A iff B's unrounded projection is strictly less than A's (unordered gives B). Not reproduced: a dead local zero store (0x10020fa1). The image has no direct caller; no indirect caller was found."
- 000981: "Listing 0x10021990-0x100219c4 walked (Capstone): BoxShape::nxBoxLoadFromDesc (ObjectModel.cpp), BOX slot 12, __thiscall ret 4: the dims words from record+0x4c/0x50/0x54 to +0xe4..+0xec as dwords, the hull rebuild 000973 (call at 0x100219b5), then BASE slot 1 0x27740 (call at 0x100219bd) whose al is returned. The candidate's compiled row has the image's instruction shape. The product's shape factory does not run it (nxShapeFactory writes the dims directly), so only the internal-slot differential reaches it."

## Dynamic path
`NxPhysicsShapeVtableTests` (in-process oracle differential, Phase 5; the oracle's box is built by its constructor
0x21870 and carries its facade table 0x10106a88 at +0xe0, the candidate's by `BoxShape::BoxShape`, which now carries
`nxBoxHullFacadeVtable` with slots 9/10). New function `runBoxHullCases`, own digest and line, after the existing cases
(existing cases and their line untouched: `shape vtable oracle_digest=ed1294b6 cases=626 failures=0` reproduced).
The exit code is non-zero when either block fails. Cases (314):
- 000973 direct (oracle `base+0x21420` as __fastcall(box), candidate member): 9 dims sets incl. +-0, negative,
  1e-3/3e7, 1-ulp, inf, denormal and NaN payloads; the hull's 117 words compared (list pointers compared by the four
  words they point at).
- 000973 through a table copy whose slots 1/3 the harness sets: face counts 0..6 x vertex counts 0,1,5,8 (28).
- 000981 through each side's BOX slot 12: 9 records (dims above, a rotated pose, groups 0-3): return byte, +0x6c pose,
  +0xd8..+0xdf, and the hull words.
- Slots 9/10 through each side's +0xe0 table (slot 10 with an out word and with null): 20 directions x 5 poses on a
  rebuilt box, 2 NaN directions, 36 crafted grouping cases (face j = (A,4,-A) permutations, A = 1e17f, where the sum's
  grouping decides the winner), 7 crafted spill cases (2 + 2^-29 vs 2 + 2^-30), 18 crafted pose-grouping cases, 96
  random normal/direction/pose samples, half of them with face counts 0..6 through a replaced slot 3.

Results (build in this worktree, oracle D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll 4b7db3e1...):
```
shape vtable oracle_digest=ed1294b6 cases=626 failures=0
box hull oracle_digest=e0477220 cases=314 failures=0
```
exit 0. The oracle digest folds only the pinned DLL's outputs (hull words, slot returns, out words, the load's return
byte, pose and halfwords).

Mutations (each built and run, then reverted): 957 unrolled grouping -> 16 failures; 959 first-unrolled grouping ->
4; 957 spill dropped (best = value) -> 6; 957 pose grouping -> 4; 959 tie arm (<=) -> 36; 973 plane d sign -> 46;
981 without the rebuild call -> 9; 973 max on >= -> 0 (only a signed-zero order could show it; not covered).

**Proposed registered line** (NxRequiredCoverageLines, target NxPhysicsShapeVtableTests, beside the existing one):
`box hull oracle_digest=e0477220 cases=314 failures=0`

Other checks in this worktree: full `cmake --build build --config Release` (no new warnings: the only ObjectModel
warnings are the pre-existing ARRAYSIZE C4005 and LNK4217 lines); NxPhysicsObjectLayoutTests transcript keeps
`layout candidate mismatches=1 mode=differential candidate_fold=4492c8c1`, all 53 registered lines present, `boxload
candidate ok=1`, and the expected `candidate CANDIDATE-MISSING family=vtables` / `batch3268 candidate failures=3`;
NxPhysicsInternalTests `static_proof checks=51 status=pass`; validate_inventory unexplained=0; tool tests 753 OK;
stable-ID lines exact, unique, RVA/size equal to inventory. The phase gate scripts were not run (this agent's sandbox
refuses powershell); the integrator should run them.

Trace (for `dynamic_proof`, not done here): in this target the candidate rows are linked into the exe
(ObjectModel.cpp), not the candidate DLL; breakpoints go on the exe's `?nxBoxRebuildHull@BoxShape@@QAEXXZ`,
`?supportFace@BoxHullFacade@@QBEIPBM0@Z`, `?supportFeature@BoxHullFacade@@QBEIPBM0PAI@Z`,
`?nxBoxLoadFromDesc@BoxShape@@QAE_NPBX@Z` and on the oracle module at 0x21420/0x20d40/0x20f90/0x21990. Expected equal
counts per row by construction of the harness (each case calls both sides once), except 000923: 0 candidate hits from
000973 (inlined).

## Public path (product vs image)
- Image: NxScene::createActor -> 000032 constructs the box (000977, facade at +0xe0) and calls BOX slot 12 (000981 ->
  000973); NxBoxShape::setDimensions -> 001069 -> 000983 -> 000973.
- Product: `nxShapeFactory` (Scene.cpp) allocates 0x228 bytes, `memset`s them to zero, installs the BOX table, the
  prunable and the dims directly; it never runs 000977 nor slot 12, so +0xe0 is 0 (no facade) and the face records
  are zero. `nxBoxHandleSetDimensions` (NpActor.cpp, 000983's candidate) writes the dims, owner update and dirty 0x20;
  it does not call 000973. Wiring 000973 into 000983 or the factory needs the facade table installed on factory
  shapes first (000973 calls through +0xe0; a factory shape would call through a null table). That is the factory /
  handle-row work (000983 is another sub-area's row; the factory is not a row of this block), left untouched here.
  So no public path reaches the four rows in the candidate today; only the internal-slot differential does.

## Concerns
- 000923 stays uncalled from the candidate's 000973 (inlined vendored header function).
- The `973 max on >=` mutation is invisible to the harness (signed-zero ordering only).
- Records for inventory (state, source/implementation, static_proof above, dynamic_proof when traced), the ledger
  reason for rows moving to `reconstructed` (000973 phase 2, 000957/000959/000981 phase 5), the contract results
  section and the coverage line registration are left to the integrator.

# Follow-up (branch `claude/sr-t4-hull2`): the public path

Branch `claude/sr-t4-hull2` from 052f1c0 (`claude/scene-raycast-block`) with ed67cfe cherry-picked (clean, no
conflicts; committed as is, 2186f38). Files: `Physics/src/Scene.cpp` (nxShapeFactory, localized),
`Physics/src/ObjectModel.cpp` (new `nxShapeFactoryLoadBox`), `Physics/src/NpActor.cpp` (000983, localized),
`tests/PhysicsSceneRaycastTests.cpp` (new block at the end). Not touched: inventory.json, ledgers, contract,
gate_targets.ps1, timing table.

## Oracle creation path of a box (Capstone, pinned DLL)
`NxScene::createActor` -> ... -> 000032 (0x1de0, the shape builder; jump table at 0x10001ffc on desc+4):
the BOX arm (0x10001eaa) allocates 0x228 through `[0x101041bc]` (Foundation allocator) and calls 000977
(0x10021870 at 0x10001ec6, args (actor, shape id)); 000977 stores the facade table 0x10106a88 at +0xe0
(0x10021895), zeroes the face-record words, builds the 0x1c handle (001075) at +0x9c, +0xd0 = 2, dims 1.0.
Back in 000032 the shared tail (0x10001eee) calls BOX slot 12 through the table (`call [eax+0x30]` at
0x10001efd) = 000981 -> dims from desc+0x4c.., 000973, 0x27740; al 0 -> slot 0 (delete, flag 1) and fail
(0x10001f02 -> 0x10001e78); else +0x9c tested and the handle's +0x10/+0x14 set. So 000981 and 000973 run
once per box created, and a public box carries the facade table at +0xe0 and the hull 000973 builds from
the descriptor's dims.

## What the factory now does
`nxShapeFactory` (Scene.cpp), BOX only: instead of memcpy'ing the dims to +0xe4 it calls
`nxShapeFactoryLoadBox(shape, descriptor)` (ObjectModel.cpp): stores `nxBoxHullFacadeVtable()` at +0xe0
(000977's 0x10021895 store; the zero fill already gives 000977's face-word zeroes) and calls BOX slot 12
through the table the factory installed (0x10001efd), i.e. 000981: dims, 000973, `ShapeBase::nxApplyDescriptor`
(re-applies pose +0x6c, flags +0xde, material +0xda, group through 001329 -- the same values the factory
wrote; +0x9c is still 0 at that point so its userData arm is skipped, as before). On al 0 the factory frees
the shape and returns 0 (unreachable: the candidate's 0x27740 returns true). Sphere/capsule/plane unchanged.
Not changed: the factory's box allocation stays on the SDK allocator while BOX slot 0 (000979) frees through
the Foundation allocator (Task 3's latent pairing note; still not live -- release never calls slot 0).
Group children (nxShapeGroupConstruct -> nxShapeFactory) get the same path.

## 000983 (0x219d0, 62 B), listing 0x100219d0-0x10021a0d
`nxBoxSetDimensions` (NpActor.cpp, stable-ID line `// phys_fn_000983 (0x000219d0, 62 B)`), __fastcall with
edx unused = the row's __thiscall ret 4 on the internal box, `__declspec(noinline)` so the handle calls it as
001069 does (0x10023503): the three dims dwords to +0xe4/+0xe8/+0xec (0x100219d7-0x100219ed), 000973 (call
at 0x100219f3), BASE slot 6 through the shape's own table with 1 (`call [edx+0x18]` at 0x100219fe), 001325
(`nxSceneMarkShapeDirty`) with 0x40 (0x10021a01; was 0x20). The contract's defects (no 000973, 0x20) are
fixed. `nxBoxHandleSetDimensions` (public slot 31) is now only 001069's role: handle +0x18 -> 000983; the
image's scene-lock check (0x1005b730/0x1005b790) and its report (0x100234cf-0x100234f4) are not reproduced
(001069 not claimed). The contract's candidate name for 000983 (NPA `nxBoxHandleSetDimensions`) should become
`nxBoxSetDimensions`.

## Test cases (NxPhysicsSceneRaycastTests, new lines only: 909 new, the 1811 existing lines unchanged)
`nxResizeCases`, after every existing query and the late actor's release: a static box s_resize (group 13),
a dynamic d_resize (14) and a dynamic rotated (0.6/0.8 about z) r_resize (15) at x=40; each printed after
creation, after `setDimensions(grown)` and after `setDimensions(shrunk)`: getDimensions and getWorldBounds
words; the internal shape's facade presence, dims, 24 vertex words, per face corners / the four words each
list pointer names / plane / range; facade slots 1 and 3; slots 9 and 10 (with out word and with null) for
12 directions x (no pose, a rotated pose) -- both slot-10 arms occur (feature 0 and 1); the scene's
flags[shape id] word (001325's). Then all six raycasts for 7 rays (x through each box, down onto each, along
z) x 3 shapes types on groups 13-15, plus a max-distance 2.0 query, at each of the three stages. Resized
shapes are named with `nxSymbolSet` (a new shape can reuse a released shape's address and inherit its name).
Observability: the dirty word is 0xffffffff on both sides at every stage (the registration path leaves it
all ones), so 0x20 vs 0x40 is not observable here; everything else is.
Before the product change the block gave stdout_delta=908 (no facade, zero hull words, old dims in the
hull). Mutations (built, run, reverted): 000983 without the 000973 call -> 40 deltas; without the slot-6
call -> 433 deltas.

## Results (this worktree's build, own pair dir D:\FlamingEnt__\novodex-analysis\pairs-hull2)
Staged by a python equivalent of run_differential.ps1 (powershell is refused in this agent's sandbox):
same staging, same dropped prefixes, stdout/stderr/exit compared. Candidate NxPhysics.dll sha256
94d8eb585e05a2e9903428759fd86f2b5c91e31e75f330a8ec0bcae703a7515b.
- Phase 5 staged targets (13), NxPhysicsSceneRaycastTests (2720 lines), the three other phase 7 targets:
  all stdout_delta=0, stderr exact, exit 0 both sides.
- NxPhysicsShapeVtableTests: `shape vtable oracle_digest=ed1294b6 cases=626 failures=0` and
  `box hull oracle_digest=e0477220 cases=314 failures=0`, exit 0.
- NxPhysicsObjectLayoutTests: `layout candidate mismatches=1 mode=differential candidate_fold=4492c8c1`,
  `batch3268 candidate failures=3 provisional=1`, `candidate CANDIDATE-MISSING family=vtables` (unchanged).
- NxPhysicsInternalTests `static_proof checks=51 status=pass`; validate_inventory unexplained=0; tool tests
  753 OK; the only new `// phys_fn_` line is 000983's (exact form, unique, RVA/size = inventory).
- The phase gate scripts themselves were not run (powershell refused); the integrator should run them.

## Hit counts (cdb 10.0.10586, `bp NxPhysics+<off> ".echo HIT <label>; gc"`, same pairs)
Candidate offsets from build\Release\NxPhysics.map of the build above: 000973 0x25230, 000957 0x2dfb0,
000959 0x2e2a0, 000981 0x24d00, 000983 0x19ae0; context: 001069's role 0x199d0, nxShapeFactoryLoadBox 0x2c3b0.
Oracle: the inventory RVAs; context 001069 0x234c0, 000977 0x21870.

| target | side | 000973 | 000957 | 000959 | 000981 | 000983 | 001069 | 000977 / LoadBox |
|---|---|---|---|---|---|---|---|---|
| NxPhysicsSceneRaycastTests | oracle | 17 | 216 | 432 | 11 | 6 | 6 | 11 |
| NxPhysicsSceneRaycastTests | candidate | 17 | 216 | 432 | 11 | 6 | 6 | 11 |
| NxPhysicsActorShapeMutationTests | oracle | 2 | 0 | 0 | 2 | 0 | 0 | 2 |
| NxPhysicsActorShapeMutationTests | candidate | 2 | 0 | 0 | 2 | 0 | 0 | 2 |

Ordered label sequences (LoadBox mapped to 000977) identical: 699 hits (raycast), 6 (mutation); the raycast
counts re-taken with the final harness are the same. The logs were not committed (no evidence file written); an integrator promoting on dynamic evidence should re-trace.

## Oracle-side lines to register (NxPhysicsSceneRaycastTests; verbatim from the oracle run)
```
box_resize s_resize grown is_box=1 get_dims=40000000.3f000000.40400000 world_bounds=42180000.bf000000.c0400000.42280000.3f000000.40400000
box_resize d_resize grown is_box=1 get_dims=3f800000.40000000.3f400000 world_bounds=421c0000.c0000000.40880000.42240000.40000000.40b80000
box_resize r_resize grown is_box=1 get_dims=40400000.3f000000.3fc00000 world_bounds=42173333.c02ccccc.41080000.4228cccd.402ccccc.41380000
box_resize r_resize grown face=1 corners=00000004 list_a=00000001.00000005.00000006.00000002 list_b=00000001.00000008.00000005.00000009 plane=3f800000.00000000.00000000.c0400000 range=c0400000.40400000
box_resize d_resize shrunk is_box=1 get_dims=3dcccccd.3e4ccccd.3e99999a world_bounds=421f999a.be4ccccd.40966666.42206666.3e4ccccd.40a9999a
box_resize r_resize shrunk support dir=6 pose=1 face=4 feature_face=4 feature=00000000 bare=4
raycast box_resize grown ray=x_resize_s type=3 groups=0000e000 max=7f7fffff hint=ffffffff closest_shape result=s_resize flags=00000017 shape=s_resize impact=42180000.00000000.00000000 normal=bf800000.00000000.00000000 distance=40400000
raycast box_resize shrunk ray=x_resize_r type=3 groups=0000e000 max=7f7fffff hint=ffffffff closest_shape result=r_resize flags=00000017 shape=r_resize impact=421b5555.3e7ffff7.41200000 normal=bf19999a.bf4ccccc.00000000 distance=40755550
```

## Open / concerns
- 0x20 vs 0x40 in 000983 is not observable on any staged target (the flags word is all ones).
- 001069's lock/report arm not reproduced; 000977 itself still not run by the factory (the factory emulates
  its +0xe0 store; the +0x9c handle, +0xd0 and allocator differences of the factory remain as Task 3 lists).
- Records (inventory state/proofs for 000983, dynamic_proof of 000973/000957/000959/000981), the contract
  results, the registration of the lines above and a committed trace evidence file are the integrator's.

# Integration (onto f65b65c)
Cherry-picked 2186f38 and 79069b7; the only conflict was tests/PhysicsShapeVtableTests.cpp's tail (the shape
sub-area's massframe/boxsweep blocks kept, runBoxHullCases and its line added after them). Reviewed 000973,
000959's slot-10 arms, 000981 and 000983 against the Capstone listing: faithful, no fix. Registered the 9 proposed
lines verbatim (all present once in the oracle side's output). Traces: evidence/scene-raycast-trace-task4-hull.txt
(every claimed row equal per target over 18 staged targets and the two in-process differentials; sequences
identical). Promoted all five rows; records in inventory.json, the ledgers, the contract (## Task 4 results: box
hull) and the timing table.
