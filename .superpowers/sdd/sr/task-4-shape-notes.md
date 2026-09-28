# Task 4, shape and mass-property rows: notes

Branch `claude/sr-t4-shape` from `claude/scene-raycast-block` at 6bd2529. Work done in the isolated worktree
`agent-a0c67ace943242768`, own build dir and pair roots (`build/pairs`, `build/pairs-shape`).
Per the dispatch, inventory.json, the ledgers, the contract, gate_targets.ps1 and the timing table are NOT
edited; this file carries the records the integrator needs.

Oracle listing: Capstone manifest (every range below walked instruction by instruction). Constants read from the
pinned image: [0x101043cc] 0.5f, [0x101041f0] 0.0f, [0x101041ec] 1.0f, [0x1010687c] -1.0f,
[0x10107a10] -2^-23, [0x10107a0c] +2^-23, 0x10106bd4 "CapsuleShape::setDimensions: radius should be positive!",
0x10106b6c "\Epic\Novodex\SDKs\Physics\src\CapsuleShape.cpp".

Control words: 000933, 000935, 000989, 000993, 000995, 000829, 000833, 000849 are API-time rows (0x027f: 53-bit,
round to nearest), where `double` register lifetimes reproduce the x87 exactly. 000867 (pair constructor 000893,
step) and 000951 (BOX slot 7, called only from the CCD sweep 002264 at 0x100562c3/0x100564b9) are step-only
(0x0f7f: 64-bit, round toward zero). They moved from ObjectModel.cpp (SSE2 in the DLL) into a new file
`Physics/src/StepOnlyRows.cpp`, added to the /arch:IA32 list with a reason comment; ObjectModel.cpp itself was
not moved. 000873 is already in ContactGeneration.cpp, which is on the IA32 list.

## Rows

### 000933 (0x204e0, 188 B) BoxShape::getWorldOBB -- written, claimed
- Candidate: `BoxShape::nxBoxGetWorldOBB(float* out) const` (ObjectModel.cpp), stable-ID line added. The public
  handle `nxBoxHandleGetWorldOBB` (NpActor.cpp, ours) now calls it (001073 calls the row at 0x10023566); it used to
  copy the cached +0x30/+0x0c pose.
- Listing 0x100204e0-0x10020599: identity 3x4 on the stack, call 001309 (0x1002054d), center = pose t (x via
  fld/fstp, y/z dwords), rep movsd 9 rotation dwords to out+0x18, extents = +0xe4..+0xec to out+0xc.
- ABI: thiscall ret 4, same as the row.
- 001309 (0x25f60, 797 B, discovered, not ours) is written from its listing as an unclaimed helper
  `ShapeBase::nxShapeGlobalPose` (ObjectModel.cpp, comment line begins `// Row 001309`): body quaternion expansion
  with its m32 spills (0x10025f9f 2yy, 0x10025fd3 2xz, 0x10025fdd 2yw, 0x10025ffd 1-2xx, 0x1002600d 2yz), body
  position +0x50, static owner pose +0x20; translation row x unrounded, y/z spilled (0x100260d3/0x100260f9);
  rotation (a+b)+c per the load order (0x10026122-0x1002624d). Walked 0x10025f60-0x1002627a. Its owners may claim
  it as is.
- Reproduced: everything. Divergence: none known.
- static_proof (ready to paste): "Walked 0x100204e0-0x10020599 (thiscall ret 4): identity pose on the stack, 001309
  at 0x1002054d composes the shape's global pose, then center (fld/fstp x, dword y/z), rep movsd of the 9 rotation
  dwords to out+0x18 and the dims +0xe4..+0xec to out+0xc; BoxShape::nxBoxGetWorldOBB reproduces it, with 001309
  written from its listing (0x10025f60-0x1002627a: quaternion expansion with the m32 spills at 0x10025f9f,
  0x10025fd3, 0x10025fdd, 0x10025ffd, 0x1002600d, static owner pose at +0x20, translation x unrounded and y/z
  spilled) as the unclaimed helper ShapeBase::nxShapeGlobalPose; the handle (001073's product copy) calls it."
- Evidence: NxPhysicsActorDynamicSetterTests, new lines geometry_obb (dynamic owner at two quaternions, static
  owner) match the oracle bit for bit while the candidate's CACHED pose differs from the oracle's in those cases
  (see Findings), so the lines discriminate the old cache-copying code from the row. The existing
  `setter box_world_obb` line is unchanged.
- Reaching targets: DSet (4 calls: box_world_obb + three geometry_obb).

### 000951 (0x20b20, 507 B) BoxShape slot 7 (sweep) -- rewritten, claimed
- Candidate: `BoxShape::nxBoxSweep(void* out, const float* direction) const`, now in StepOnlyRows.cpp (IA32),
  stable-ID line added. The fitted model is gone.
- Listing 0x10020b20-0x10020d18 walked: max = dims, min = -dims (fchs); -t via the -1.0 literal (x spilled, y/z in
  registers); R^T(-t) rows 1/2 spilled, row 0 kept; R^T t row 0 kept, rows 1/2 spilled; origin sums spilled; R^T d
  each spilled; 001730(min, max, origin, dir, &tnear, &tfar) cdecl; -1 returns false, else *out = |tfar|, true.
- 001730 (0x38050, 61 B) + 001732 (0x38090, 296 B) are one function split by the splitter (001731 is the 3-byte
  gap); written from 0x10038050-0x100381b7 as the unclaimed static helper `nxSegmentSlabs` (StepOnlyRows.cpp): the
  +-2^-23 parallel band (strict), m32 inverse, t1 unrounded / t2 spilled, swap on t1 > t2 (face i+3), the four
  exit tests inside the loop and the two after it. Neither 001730 nor 001732 is ours to claim.
- ABI: thiscall ret 8, same as the row.
- Step-only: "reachable only from the simulation step; no public path while NpScene::simulate is a stub"
  (CCD sweep 002264, NX_CONTINUOUS_CD off by default).
- static_proof: "Walked 0x10020b20-0x10020d18 (thiscall ret 8; out, direction): dims/-dims slabs, -t by the -1.0
  literal, R^T(-t) + R^T t with the listing's spills (rows 1-2 of each spilled, row 0 kept) and association,
  R^T d spilled, then 001730/001732 (0x10038050-0x100381b7, one function, written as the unclaimed helper
  nxSegmentSlabs: +-2^-23 band, m32 inverse, t1 unrounded and t2 spilled, swap to face i+3, the in-loop and final
  exit tests); -1 returns false, otherwise *out = |tfar|. Reachable only from the simulation step (CCD sweep 002264);
  no public path while NpScene::simulate is a stub. Built /arch:IA32 (StepOnlyRows.cpp) for the step's 0x0f7f."
- Evidence (API-time drives, harness CW): ObjectLayout `boxsweep-diff candidate run=48 failures=0` (existing,
  unchanged, now against the listing code instead of the fitted model) and the new ShapeVtable line
  `shape vtable boxsweep ... cases=84 failures=0` (rotated/translated boxes, parallel-band and non-finite dirs).
- Reaching targets: ObjectLayout (48), ShapeVtable (84); DLL: none (step).

### 000993 (0x21b70, 108 B) CapsuleShape::setDimensions -- written, claimed
- Candidate: `CapsuleShape::nxCapsuleSetDimensions(float, float)`, defined in NpActor.cpp beside its only caller
  (it calls 001325 in Scene.cpp, which the ObjectModel-only test targets do not link); stable-ID line there.
  `nxCapsuleHandleSetDimensions` calls it (001113 at 0x10023b78).
- Listing 0x10021b70-0x10021bd9: +0xe0 = r (dword), +0xe4 = h*0.5, fcomp r vs 0.0 with `test ah,0x41; jp` (report
  line 0x4f on r <= 0, NaN not reported), vtable slot 6 with 1 (call [edx+0x18]), 001325(0x100).
- Fixed: the report arm and the 0x100 dirty flag (was 0x20); slot 6 now goes through the shape's table.
- The report goes through the reconstruction's nxReport, whose sink the DLL never installs (candidate-wide, same
  as 000989 and the sphere rows): the arm is silent in the DLL. See Findings.
- static_proof: "Walked 0x10021b70-0x10021bd9 (thiscall ret 8): radius dword to +0xe0, height*0.5f to +0xe4, the
  radius <= 0 report (CapsuleShape.cpp line 0x4f; `test ah,0x41; jp` skips greater and unordered), slot 6 through
  the shape's table with 1, then 001325 with 0x100; CapsuleShape::nxCapsuleSetDimensions reproduces every step and
  NpCapsuleShape::setDimensions' product copy calls it."
- Evidence: DSet `setter capsule_set_dimensions` (existing) and new `geometry_capsule_dimensions` match.
- Reaching targets: DSet (2).

### 000995 (0x21be0, 23 B) CAPSULE slot 14 setRadius -- defect fixed
- `CapsuleShape::nxCapsuleSetRadius` reads the vptr, stores +0xe0, then calls slot 6 through the table with 1
  (the tail jump at 0x10021bf4). `nxCapsuleHandleSetRadius` now calls the internal table's slot 14 as
  NpCapsuleShape::setRadius does ([edx+0x38] at 0x10023bd5) instead of inlining the row.
- static_proof: "Walked 0x10021be0-0x10021bf4: vptr read, +0xe0 = radius, argument slot forced to 1, tail jump
  through slot 6 (+0x18); CapsuleShape::nxCapsuleSetRadius reproduces it as a call through the table, and the
  public handle reaches it through slot 14 as 0x10023bd5 does."
- Reaching targets: DSet (2, through slot 14), ShapeVtable (4, unchanged digest), ObjectLayout (1).

### 000989 (0x21ad0, 110 B) CAPSULE slot 12 loadFromDesc -- defects fixed
- Report condition now `radius <= 0` (NaN not reported; 0x10021b07), and the row returns the bool of the BASE
  apply (0x10021b34); the header declaration is `bool`.
- static_proof: "Walked 0x10021ad0-0x10021b3b (thiscall ret 4): radius, height*0.5f, +0xe8 stores, the
  radius <= 0 report (line 0x37; NaN not reported), then 001347's al returned; reproduced."
- Evidence: new ShapeVtable line `shape vtable capsule_load_return oracle=1 candidate=1`; the slot-12 case inside
  the registered digest is unchanged.
- Reaching targets: ShapeVtable (1), ObjectLayout (2); DLL: none (createActor does not use slot 12).

### 000935 (0x205a0, 198 B) BOX slot 9 world AABB -- defect fixed
- (|dz r2| + |dy r1|) + |dx r0| per row in double; rows 0/1 extents spilled to m32 (0x100205de, 0x10020604), the
  row-2 extent unrounded (fsub st(4) 0x10020646, fadd st(1) 0x10020659).
- static_proof: "Walked 0x100205a0-0x10020663 (thiscall ret 4): per-row extents summed (|dz r2| + |dy r1|) +
  |dx r0| (faddp at 0x100205ce, 0x100205dc and their row copies), rows 0 and 1 spilled to m32 and re-read, row 2 kept
  in st(4) and used unrounded (0x10020646, 0x10020659), dead fst spills ignored; reproduced."
- Evidence: new DSet line `setter geometry_bounds=0.0...` (rotated local pose on a rotated dynamic body) matches;
  emulating the old candidate formula on the same cached pose gives a different word (checked offline), and the
  new formula reproduces the oracle's bounds from the oracle's cached pose in all three probed poses.
- Reaching targets: DSet (existing 2 + new 1), ObjectLayout (1 direct), ShapeVtable (slot 9 cases, digest
  unchanged).

### 000829 (0x1bd00, 187 B) MassFrame build box -- defect fixed
- F = 1/3 * m spilled to m32 (0x1001bd39); pairwise sums spilled (0x1001bd67/6c/72); diagonals = sum_f32 * F_f32.
- static_proof: "Walked 0x1001bd00-0x1001bdb8: volume accumulator over non-zero half-extent words, *8, F spilled
  to m32 at 0x1001bd39, the three pairwise square sums spilled at 0x1001bd67/0x1001bd6c/0x1001bd72, each diagonal
  their product with F, off-diagonals and offset zeroed; reproduced."
- Evidence: new ShapeVtable `massframe` line includes 9 box builds, three of them at extents where the spills change
  a diagonal. Reaching targets: ShapeVtable (9), ObjectLayout (boxmass); DLL: none (000008 not reproduced).

### 000833 (0x1c040, 1371 B) MassFrame translate -- rewritten from the listing
- Both paths walked: all-zero d early out; c = d + o spilled; centered path (all c words zero) adds m * X*X,
  displaced path adds m * (X*X - N) with N = -Q(c); every off-diagonal carries its `x * [0x101041f0]` addend; the
  (-ox)ox, (-cy)cy, (-cx)cx spills; offset += d.
- static_proof: "Walked 0x1001c040-0x1001c598 (thiscall ret 4): zero-d early out, c = d + o spilled, the centered
  path (0x1001c0d7-0x1001c26a) and the displaced path (0x1001c26f-0x1001c569) with every product, x*0.0 addend, m32
  spill ([esp+8], [esp+0xc], [esp+0x7c]), difference, mass product and inertia sum in the listing's order, then
  offset += d (0x1001c578-0x1001c58f); reproduced."
- Evidence: ObjectLayout `mftranslate candidate run=54 failures=0` and `negtrans` unchanged; new ShapeVtable
  `massframe` line: 192 translates over inf/-inf/NaN/-0/+0/finite offsets and steps on both paths. Mutation check:
  dropping two of the zero addends gives `failures=20` on that line.
- Reaching targets: ShapeVtable (192), ObjectLayout (54 + 5 + boxmass); DLL: none.

### 000849 (0x1c8c0, 101 B) box compute-mass helper -- defect fixed
- Calls 000833 (`nxMassFrameTranslate(extra + 0x24)`) as 0x1001c8eb does instead of the inline centered formula.
- ABI unchanged and recorded (thiscall on the destination in the row; shape member with destination first).
- static_proof: "Walked 0x1001c8c0-0x1001c922: build (000829), fold (000831) and translate by extra+0x24 (000833)
  when extra is non-null, scale unless density == 1.0f (fucompp; unordered scales), merge (000839); reproduced
  with the calls in that order (convention recorded: the row is thiscall on the destination)."
- Evidence: ObjectLayout `boxmass candidate ok=1 digest=d82de90e` unchanged. Reaching: ObjectLayout; DLL: none.

### 000867 (0x1d260, 103 B) kind accumulator -- defect fixed, moved to StepOnlyRows.cpp
- v = a / b kept unrounded; v*c2 spilled to m32 (0x1001d299) before its add. ABI recorded (cdecl free function;
  the row is thiscall ret 0xc; only direct caller 000893).
- The census `implementation` still names ObjectModel.cpp; ObjectModel.cpp keeps a comment naming phys_fn_000867
  so validate_inventory passes. Integrator: set `implementation`/`source` to Physics/src/StepOnlyRows.cpp, then the
  mention can go.
- static_proof: "Walked 0x1001d260-0x1001d2c4: v = a/b unrounded (0x1001d26b), kinds 4/5 add v*desc[0], v*desc[1]
  from the stack and v*desc[2] after its m32 spill (0x1001d299), bit 6 sets +0x75, other kinds add v to +0x64;
  reproduced. Reachable only from the simulation step; no public path while NpScene::simulate is a stub. Built
  /arch:IA32 (StepOnlyRows.cpp) for the step's 0x0f7f."
- Evidence: ObjectLayout `accum0867 candidate failures=0` unchanged.

### 000873 (0x1d610, 706 B) contact-stream emitter -- defect fixed, claimed
- The eight stream-growth sites now call `SdkContainer::resize` (004840, Containers.cpp) on the sink's stream at
  +0x38 (`count == capacity` -> resize(1), `count + 3 > capacity` -> resize(3)); the write follows regardless, as
  in the row. Stable-ID line added (the row had none).
- static_proof: "Walked 0x1001d610-0x1001d8cf (thiscall ret 0x1c): orientation swap/negate, header on object change
  (feature-pair flag, ids, material<<24|flag<<16, pair count), normal block on raw-word change, contact record
  (separation & 0x7fffffff, optional feature word); the eight growth sites 0x1001d6e7, 0x1001d711, 0x1001d752,
  0x1001d7c7, 0x1001d7fc, 0x1001d83f, 0x1001d87b, 0x1001d8a8 call 004840 (SdkContainer::resize) with 1 or 3 and
  the write follows regardless; reproduced. Step-only (narrow phase)."
- Evidence: CollisionTests `contact_emit ... oracle=deaa1fc557071411 candidate=deaa1fc557071411 mismatches=0`
  and contact_plane_box unchanged (the harness pre-sizes the stream: growth unexercised on both sides).

## Findings for other owners (not changed here)
- Cached world pose +0x0c: after `setGlobalOrientationQuat` on a dynamic body, and on a static actor created with
  a rotated pose and local pose, the candidate's cached pose differs from the oracle's in the last bits (e.g.
  t.y be99c436 vs be99c438 on the static owner; rot[0], t.x, t.y after the quaternion set). 001315
  (`ShapeBase::nxApplyOwnerUpdate`) expands the quaternion with the unstaged `nxQuatToMatrix9` and reads the body
  translation at record+0x18 where 001309's listing stages the spills and reads +0x50. World bounds and capsule
  world bounds after setDimensions/setRadius on a rotated owner inherit this; the new DSet cases print bounds only
  where the cache matches.
- The reconstruction's report sink (`nxInstallReportSink`) is never installed in the DLL, so every
  radius-should-be-positive arm (000989, 000993, sphere) is silent where the oracle reports through the SDK's
  error stream; no case with radius <= 0 was added for that reason.
- 001309 and 001730/001732 are written (unclaimed) and can be claimed by their units' owners.

## Test results (own build, own pairs)
- Build Release Win32: clean (no new warnings).
- Gates (run_phase_gate.ps1, RepoRoot = this worktree, final code): phases 2, 3, 4, 6, 7 pass; phase 5 fails only
  on `candidate CANDIDATE-MISSING family=vtables` (ObjectLayout exit 1), every staged target stdout_delta=0.
- validate_inventory: unexplained=0. Tool tests: 753 OK. Stable-ID lines: 418 in Physics/src, no duplicates, every
  RVA/size equal to the inventory; my rows' lines: 000829/000833/000849/000933/000935/000989/000995 ObjectModel.cpp,
  000867/000951 StepOnlyRows.cpp, 000993 NpActor.cpp, 000873 ContactGeneration.cpp.
- NxPhysicsShapeVtableTests: `shape vtable oracle_digest=ed1294b6 cases=626 failures=0` unchanged.
- NxPhysicsObjectLayoutTests: `layout candidate mismatches=1 mode=differential candidate_fold=4492c8c1` and
  `batch3268 candidate failures=3 provisional=1` unchanged.
- NxPhysicsActorDynamicSetterTests: stdout_delta=0 with the new lines.

## Proposed registered lines (copied from the oracle side)
NxPhysicsActorDynamicSetterTests (staged pair):
```
setter geometry_actor=0.1
setter geometry_obb=0.0.4094cccd.bfd99999.40b80000.3f333333.3fa66666.40066666.3f2e3abc.beeb6ef9.3f11d0d9.3f24a368.3ba1120f.bf439d28.3eb22871.3f63410c.3e98f887
setter geometry_bounds=0.0.40184c72.c070b7cc.406e6615.40dd7361.3eb8f194.40f8ccf5
setter geometry_obb=0.1.40408db8.be99c434.40b6a61f.3f333333.3fa66666.40066666.3e4d9d93.bf634975.bed43861.3f79b0c5.3e095e90.3e3a9cb7.bdd7f68b.bee30a90.3f642177
setter geometry_actor=1.1
setter geometry_obb=1.0.40408db9.be99c438.40b6a61f.3f333333.3fa66666.40066666.3e4d9d94.bf634976.bed43860.3f79b0c5.3e095e92.3e3a9cb6.bdd7f687.bee30a90.3f642177
setter geometry_capsule_actor=1
setter geometry_capsule_dimensions=3e99999a.3fd9999a.3e99999a.3f59999a.80002
setter geometry_capsule_radius=3ee66666.3ee66666.80002
```
NxPhysicsShapeVtableTests (oracle differential):
```
shape vtable capsule_load_return oracle=1 candidate=1
shape vtable massframe oracle_digest=7c450cef cases=201 failures=0
shape vtable boxsweep oracle_digest=2c5d5c09 cases=84 failures=0
```
Phase 5 floor: +9 staged-pair lines, +3 oracle-differential lines (update the Python pins accordingly).
