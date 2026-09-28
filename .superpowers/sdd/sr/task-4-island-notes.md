# Task 4, island / body-step / CCD sub-area: source notes

Branch `claude/sr-t4-island` (from `claude/scene-raycast-block` at bb2e485). Source only: no inventory,
ledger, contract, gate_targets or timing edits (the integrator does records).

## Files

- `Physics/src/Island.cpp` (new): 000708 000714 000716 000718 000720 000724 000728 000730 000762 000764, plus
  three placeholder bodies for callees outside the sub-area (below).
- `Physics/src/BodyStep.cpp` (new): 000710 000726 000732 000734 000736 000770 (body math), 000740 000772
  000774 (CCD). 000738 stays `Row000738Fixture::row000738` in `core/JointSupport.cpp` (unchanged).
- `Physics/src/include/BodyStep.h` (new): one fixture struct per row (the `core/JointSupport.h` convention:
  thiscall rows as members of never-constructed structs), `Row000740Target` (slot 10 of the actor body's
  +0x10 object), and the placeholder declarations.
- `Physics/src/include/X87Sqrt.h`: new naked helpers `x87FsqrtQuotDot3`, `x87FsinHalfOverNorm3`,
  `x87FcosHalfNorm3`, `x87RateOverRoot`, `x87CIacos`, `x87AcosRateOverRoot` (+ non-MSVC fallbacks).
- `CMakeLists.txt`: BodyStep.cpp and Island.cpp added to the `/arch:IA32` list with a reason comment.

Build: `cmake -S . -B build -A Win32; cmake --build build --config Release` (all targets) exits 0; no
warning names BodyStep.cpp, Island.cpp, BodyStep.h or X87Sqrt.h (the remaining warnings are the
pre-existing C4005/C4291/D9025 ones). Stable-ID check: all 19 new lines fullmatch
`\s*// phys_fn_\d{6} \(0x[0-9a-f]{8}, \d+ B\)`, RVA/size equal inventory.json, no duplicate ID across
Physics/src; BodyStep.h uses `// Row NNNNNN (...)` so no other line begins `// phys_fn_`. Gates,
validator and tool tests were not run (no records changed; integrator's step).

Keeping unreferenced rows: nothing in the candidate calls these rows (the step and the CCD sweep 002264 are
not reconstructed). They are out-of-line member functions, so the compiler emits them, and NxPhysics links
with `/OPT:NOREF /OPT:NOICF`, which keeps them, as it keeps `Row000738Fixture::row000738`. Rows called within
their own file (000716, 000718, 000728, 000734, 000736, 000740, 000772) are `__declspec(noinline)` so they
stay real calls, as the oracle's are.

Wiring: none. `NpScene::simulate` and `NpScene::fetchResults` (NpScene.cpp:716, "unimplemented") are stubs;
no existing candidate code inlines or calls any of these rows (grep for the rows, their callers 000608 000610
000611 000613 000615 000619 000635 000636 and 002264, and the record offsets), so nothing was wired.

## Callees outside the sub-area (open; the integrator connects names)

| callee | used by | declared as | placeholder |
| --- | --- | --- | --- |
| 000722 (0x16130, 127 B), body-creation sub-area (also written as `Row000722Fixture::row000722` in `core/JointSupport.cpp` by the effector session, worktree nostalgic-hamilton-a71fa5) | 000764 (tail jump 0x1798f) | `struct Row000722Fixture { void row000722(); }` in BodyStep.h | Island.cpp, NX_ASSERT(0) |
| 000897 (0x1f320, 554 B), contact-pair manager sub-area | 000728 (0x16816, ecx = pair+0x14, args scene, dt, invDt, `ret 0xc`) | `struct Row000897Fixture { void row000897(void* scene, NxReal dt, NxReal invDt); }` | Island.cpp, NX_ASSERT(0) |
| 004172 (0x9b0d0, 69 B), owner gap Joint.cpp..D6Joint.cpp, no candidate anywhere | 000720 (0x16127), 000764 (0x17982), cdecl | `void __cdecl nxBodyIslandRebuild004172(void* root)` | Island.cpp, NX_ASSERT(0) |
| 004167 (0x9ad10) | 000762, 000764 | existing `Row004167Fixture::row004167` (deferred stub, core/JointSupport.cpp) | existing |

On merge: if the owner's struct/function has the same name, delete the BodyStep.h declaration and the Island.cpp
placeholder (a duplicate definition fails the link, LNK2005, which flags it); if the name differs, retarget
the one call site (000764 for 000722; 000728 for 000897; 000720/000764 for 004172) and delete the placeholder.
000748 is not called by any row here.

Existing callees used: 000712 `Row000712Fixture::row000712` (core/JointSupport.cpp), 000713
`nxBodyRecordFixRoot` (ObjectModel.cpp, cdecl; the oracle row is thiscall, the recorded code-shape defect),
004133 `Joint::row_slot6` called qualified (`joint->Joint::row_slot6(dt)`, no table dispatch, as 0x167ea),
000758 `Row000758Fixture::row000758`, 000746 `nxNpActorWorldTensorRDRt` and 000756
`nxNpActorUpdateCMassQuaternion` (NpActorDynamicMath.h, cdecl statics; oracle calls 000756 with ecx), 000429
`PhysicsSDK::instance->getParameter(NX_CONTINUOUS_CD)`, 000022 `Row000022Fixture::row000022`, the Foundation
allocator `nxFoundationSDKAllocator->free` for `[0x101041bc]` slot +0x14 (as 000760 does).

Note for the 000756 owner (not changed here): `nxNpActorBodyQuaternionFromMatrix` uses CRT `sqrt`; in an
/arch:IA32 TU (BodyStep.cpp, via 000772) that is `__CIsqrt`, which rounds to nearest where the oracle's
inline fsqrt at 0x0f7f rounds toward zero.

## Control word / reachability (for the proofs)

Callers from the Capstone listing (every direct call/jmp to each row):
708 <- 000613 (0x11378); 710 <- 000619 (0x114e8); 714 <- 000732; 716 <- 000762; 718 <- 000724; 720 <- 000635
(0x12842); 724 <- 000608 (0x111fd); 726 <- 000610 (0x11240); 728 <- 000730; 730 <- 000611 (0x11344);
732 <- 000636 (0x128c3); 734/736 <- 000770, 000772; 740 <- 000770; 762 <- 000635 (0x12816); 764 <- 000635
(0x12885); 770 <- 000615 (0x113f5); 772 <- 000774; 774 <- 002264 (0x565b2).

- All but 000710 and 000772/000774 are step-only (NxScene::simulate -> 002400 -> 000659 -> 000655 -> the
  phase rows above), control word 0x0f7f.
- 000710: only 000619, reached from NxScene::fetchResults (000398 @0x1000d777), i.e. API time, not the step;
  it has no floating point (dword moves), so the control word is irrelevant. The candidate's fetchResults is
  a stub: no public path.
- 000772/000774: not step-only in the audit's sense, but the only caller of 000774 is the CCD sweep 002264,
  whose only caller is 002266 (NxContinuousCdPair), called from the seven contact-matrix entries 001749,
  001772, 001820, 001851, 001919, 001929, 001933 (0x3ae13, 0x3d543, 0x411e3, 0x4420e, 0x4a309, 0x4b220,
  0x4b896). In the shipped runtime those entries run in the step's narrow phase (0x0f7f), and 002266 returns
  before 002264 unless NX_CONTINUOUS_CD != 0 (default 0). In the candidate the entries are reached directly
  by staged harness targets, but its NxContinuousCdPair (ContactGeneration.cpp:1040) stops before 002264,
  which is not reconstructed, so no candidate path reaches 000772/000774.

## Per row

Common ABI note: each row is a thiscall member of its fixture; MSVC emits `ret N` matching the oracle's (checked
in the dumpbin of the objects: 000726/000732/000770/000728/000730/000724/000736 `ret 8`, the one-argument rows
`ret 4`, the no-argument rows plain `ret`). Where the oracle relies on a callee preserving ecx/edx across a call
(000770/000772 reuse ecx/edx after 000734; LTCG register knowledge), the candidate reloads: code shape only.

### 000708 `Row000708Fixture::row000708` (Island.cpp), 0x15c20-0x15ca1
thiscall, no args, `ret`. Dword copies from the +0x204 JointSupportBody (+0x00, +0x10, +0x44, +0x50 via the
`JointSupportBody` struct) to +0x34, +0x40, +0x1a0, +0x1ac; +0x1e4 |= 0x20 between the second and third
group; +0x204 re-read per group. Faithful.
static_proof: "Walked 0x15c20-0x15ca1: thiscall on the body record, plain ret; the four dword triples from the +0x204 JointSupportBody (+0x00/+0x10/+0x44/+0x50) to +0x34/+0x40/+0x1a0/+0x1ac with +0x1e4 |= 0x20 after the second, +0x204 re-read per group, all reproduced by Island.cpp Row000708Fixture::row000708. Reachable only from the simulation step (000613); no public path while NpScene::simulate is a stub."

### 000714 `Row000714Fixture::row000714(NxReal dt)` (Island.cpp), 0x15d70-0x15eb5
thiscall, `ret 4`. G = +0x1e8 read directly (no find); +0x1b8 vs G+0x1f4 word compare, then the wake raise
(+0x114 bit 8, ordered-below 0.39999998f -> store 0x3ecccccc); +0x1b8 = G+0x1f4; saved (+0x1e4 bit 5) or live
velocities; |v|^2 = (x x + y y) + z z kept (double), |w|^2 spilled to float (0x15e30); both ordered below
+0xd0/+0xd4; +0x4c == 0 (fucompp/jnp) left alone, else +0x4c -= dt stored and the unrounded difference
ordered below 0 stores 0; otherwise the wake raise (reloading +0x114); +0x114 &= ~0x100. Checked in the obj:
grouping, the float spill, the unrounded compare after `fst`.
static_proof: "Walked 0x15d70-0x15eb5 (x87): thiscall ret 4; the +0x1b8/G+0x1f4 check and 0.4f wake raise (test ah,5; jp: ordered below 0x101053d4), the saved/live velocity choice on +0x1e4 bit 5, |v|^2 kept and |w|^2 spilled to float as the listing does, both ordered below +0xd0/+0xd4, the wake decrement (fst, then the register value against 0) and the final +0x114 bit-8 clear are reproduced by Island.cpp Row000714Fixture::row000714. Reachable only from the simulation step (000636 -> 000732 -> row); no public path while NpScene::simulate is a stub."

### 000716 `Row000716Fixture::row000716(void* other)` (Island.cpp), 0x15ec0-0x15fd2
thiscall, `ret 4`. Find of other then this (000712 on each parent, result stored back), other's root read
before this record's find. Rank (+0x1c0) strictly greater (unsigned, jbe) keeps this root; else other's root
survives and its rank is incremented unconditionally (0x15f85; the contract's "bumps rank on a tie" is
imprecise: there is no tie test). List splice via tail +0x1d4 / next +0x1d0; +0x1c4 and +0x1c8 summed; +0x1e4
bit 1 set on the survivor, cleared on the absorbed.
static_proof: "Walked 0x15ec0-0x15fd2: thiscall ret 4; both finds as the inlined 000712 pattern (other first, its root read before this record's find), the unsigned rank comparison (jbe), the absorbed root's parent, the +0x1d4/+0x1d0 list splice, the +0x1c4/+0x1c8 sums, the +0x1e4 bit-1 transfer and the unconditional rank increment of the else arm are reproduced by Island.cpp Row000716Fixture::row000716. Reachable only from the simulation step (000635 -> 000762 -> row); no public path while NpScene::simulate is a stub."

### 000718 `Row000718Fixture::row000718(void* other)` (Island.cpp), 0x15fe0-0x160fa
thiscall, `ret 4`. As 000716 on +0x1e8 (000713 = cdecl `nxBodyRecordFixRoot` on the parent), rank +0x1ec,
list next +0x1fc / tail +0x200, word sums +0x1f0/+0x1f4, float sum +0x1f8 (`fld absorbed; fadd survivor;
fstp`), no +0x1e4 flag; unconditional rank increment in the else arm (0x160a1).
static_proof: "Walked 0x15fe0-0x160fa: thiscall ret 4; the two sleep-group finds (000713 on each parent, other first), unsigned rank compare on +0x1ec, the +0x200/+0x1fc splice, the +0x1f0/+0x1f4 word sums and the +0x1f8 float sum in the listing's operand order, and the unconditional else-arm rank increment are reproduced by Island.cpp Row000718Fixture::row000718 (000713 is called through the cdecl nxBodyRecordFixRoot, a recorded code-shape difference). Reachable only from the simulation step (000608 -> 000724 -> row); no public path while NpScene::simulate is a stub."

### 000720 `Row000720Fixture::row000720()` (Island.cpp), 0x16100-0x1612d
thiscall, `ret`. Find (000712 on the parent); byte root+0x1e4 & 2 -> 004172(root) cdecl. Open callee 004172
(placeholder).
static_proof: "Walked 0x16100-0x1612d: thiscall, plain ret; the inlined 000712 find and the +0x1e4 bit-1 test calling 004172 cdecl on the root are reproduced by Island.cpp Row000720Fixture::row000720; 004172 itself is not reconstructed (placeholder nxBodyIslandRebuild004172). Reachable only from the simulation step (000635); no public path while NpScene::simulate is a stub."

### 000724 `Row000724Fixture::row000724(void* other, void* pair)` (Island.cpp), 0x161b0-0x1625c
thiscall, `ret 8`. ++this+0x25c, ++other+0x25c; sleep-group find of this (000713), ++root+0x1f0; with other:
its find, pair pushed onto the +0x208 list (link pair+0x100) of other when other+0x11c < this+0x11c
(unsigned `jb`) else this, then this->000718(other); without: onto this record's list.
static_proof: "Walked 0x161b0-0x1625c: thiscall ret 8; the +0x25c increments, the 000713 finds, root+0x1f0 increment, the +0x11c (jb) owner choice for the +0x208/+0x100 push and the 000718 call are reproduced by Island.cpp Row000724Fixture::row000724. Reachable only from the simulation step (000608); no public path while NpScene::simulate is a stub."

### 000728 `Row000728Fixture::row000728(NxReal dt, NxReal invDt)` (Island.cpp), 0x167c0-0x16829
thiscall, `ret 8`. Joints on +0x1d8 (link +0x34): +0x160 = -1, +0x164 = 0, `Joint::row_slot6(dt)` called
non-virtually (004133); then scene = [[+0x19c]+4] (read before the list test), contact pairs on +0x208
(link +0x100): 000897 on pair+0x14 with (scene, dt, invDt). Open callee 000897 (placeholder).
static_proof: "Walked 0x167c0-0x16829: thiscall ret 8; the +0x1d8 joint walk (+0x160=-1, +0x164=0, direct call of 004133 = Joint::row_slot6 with the first argument) and the +0x208 pair walk calling 000897 on pair+0x14 with ([[+0x19c]+4], a1, a2) are reproduced by Island.cpp Row000728Fixture::row000728; 000897 is not reconstructed here (placeholder Row000897Fixture). Reachable only from the simulation step (000611 -> 000730 -> row); no public path while NpScene::simulate is a stub."

### 000730 `Row000730Fixture::row000730(NxReal dt, NxReal invDt)` (Island.cpp), 0x16830-0x16857
thiscall (null this allowed), `ret 8`; 000728 over the +0x1fc chain.
static_proof: "Walked 0x16830-0x16857: thiscall ret 8 with a null-this test; 000728(a1, a2) on each record of the +0x1fc chain, reproduced by Island.cpp Row000730Fixture::row000730. Reachable only from the simulation step (000611); no public path while NpScene::simulate is a stub."

### 000762 `Row000762Fixture::row000762(void* joint)` (Island.cpp), 0x177c0-0x17922
thiscall, `ret 4`. joint+8 set: wake raise on **this** (esi) and b1 = joint+0xc, else b1 = 0; this record's
find, its root's island object released (004167 + allocator free + 0), ++root+0x1c4. With b1: wake raise on
**this** again (0x17856 reads esi; the contract row says it wakes b1: it does not), b1's find and object
release, joint pushed onto the +0x1d8 list (link +0x34) of the record with the smaller +0x11c (b1 when below,
`jae`) and onto the other's +0x1dc list (link +0x38, with the listing's null test), then this->000716(b1).
Without b1: onto this record's +0x1d8, root+0x1e4 |= 2.
static_proof: "Walked 0x177c0-0x17922: thiscall ret 4; the joint+8 test with the wake raise on the record itself (twice on the two-body path, both reading esi), the 000712 finds, the island-object release through 004167 and the Foundation allocator's free, root+0x1c4 increment, the +0x11c (jae) choice of the +0x1d8/+0x34 and +0x1dc/+0x38 lists, the 000716 merge and the single-body +0x1e4 |= 2 are reproduced by Island.cpp Row000762Fixture::row000762 (004167 is the existing deferred stub). Reachable only from the simulation step (000635); no public path while NpScene::simulate is a stub."

### 000764 `Row000764Fixture::row000764()` (Island.cpp), 0x17930-0x1798f
ecx record, plain `ret` (fastcall == thiscall-no-args). Find; root+0x1e4 bit 1: release the island object
(004167, free, +0x1e0 = 0), 004172(root) cdecl; then 000722(record) (the oracle's tail jmp becomes a call).
Open callees 000722, 004172 (placeholders).
static_proof: "Walked 0x17930-0x1798f: record in ecx, plain ret; the 000712 find, the +0x1e4 bit-1 test releasing the island object (004167, allocator free, +0x1e0=0) and calling 004172 cdecl, then 000722 on the record (a tail jmp in the oracle, a call here) are reproduced by Island.cpp Row000764Fixture::row000764; 000722 and 004172 are placeholders until their owners' code lands. Reachable only from the simulation step (000635); no public path while NpScene::simulate is a stub."

### 000710 `Row000710Fixture::row000710(const NxVec3* gravity)` (BodyStep.cpp), 0x15cb0-0x15d27
thiscall, `ret 4`. Byte +0x10c bit 0 clear: +0x88..+0x90 = *gravity (dwords); set: 0; +0x94..+0xb4 = 0.
static_proof: "Walked 0x15cb0-0x15d27: thiscall ret 4, no floating point; the NX_BF_DISABLE_GRAVITY (+0x10c bit 0) choice between *gravity and zero for +0x88..+0x90 and the nine zeroed words +0x94..+0xb4 are reproduced by BodyStep.cpp Row000710Fixture::row000710. Its only caller is 000619, reached from NxScene::fetchResults (000398); the candidate's fetchResults is a stub, so there is no public path."

### 000726 `Row000726Fixture::row000726(NxReal dt, NxReal invDt)` (BodyStep.cpp), 0x16260-0x167b8
thiscall, `ret 8`. Kinematic arm (0x16274-0x1653e): linear from the target (+0x118, flag bit 0) with the
x/y differences in registers, scaled by S = Scene+0x554 and stored, the z difference stored and its S product
kept, then * invDt; angular (flag bit 1): r = t * conj(q) with a = -qx stored, -qy/-qz kept, the four-term
sums in the listing's order stored; negation when r.w ordered below 0; normalise unless |r| == 0 exactly;
when |r.w - 1| is ordered above 1e-6f: angle 0 (r.w >= 1) / pi float (r.w <= -1) / _CIacos(r.w);
k = (angle invDt + angle invDt) / sqrt(1 - r.w r.w) (formed inside x87RateOverRoot / x87AcosRateOverRoot so
the dividend and the arc cosine do not cross a call); w = (r k) S with x/y of r k stored, z kept; then
T+0xc = 0 when Scene+0x550 == Scene+0x558 + 1. Dynamic arm (0x16541-0x167b8): scale = 1/(unsigned)+0x25c
when > 1 and the .data float 0x10123c00 (`gBodyStepPairDivide`, 0 in the image, never written) is not 0; the
linear and angular sums with the listing's register/spill pattern; damping (factor ordered below 1 scales by
1 - factor, else zero; angular factor stored to float first); clamp |w|^2 ordered above +0xd8 by
sqrt(+0xd8/|w|^2) (x87FsqrtQuotDot3). The obj was checked: grouping and spills follow the source, no SSE.
Divergence: _CIacos's error reporting (_87except, inexact/domain) is not reproduced (no value change); the
oracle's `fild`+2^32 unsigned conversion is the compiler's own `(double)NxU32` conversion (same value).
static_proof: "Walked 0x16260-0x167b8 (x87): thiscall ret 8; the kinematic arm (target position and quaternion delta, hemisphere flip, normalisation, the 0/pi/_CIacos angle through the x87CIacos stand-in for CRT row 005697, the (a invDt + a invDt)/sqrt(1 - w w) rate kept in registers, the Scene+0x550/+0x558 target reset) and the dynamic arm (pair-count scale gated by the .data float 0x10123c00, force/gravity and torque integration with the listing's spills, linear and angular damping, the +0xd8 angular clamp) are reproduced by BodyStep.cpp Row000726Fixture::row000726; _CIacos's _87except error reporting is not reproduced (no returned value changes). Reachable only from the simulation step (000610); no public path while NpScene::simulate is a stub."

### 000732 `Row000732Fixture::row000732(NxReal dt, NxReal unused)` (BodyStep.cpp), 0x16860-0x169ec
thiscall, `ret 8`. 000714(dt); +0x4c bit pattern 0 (integer test: -0.0f counts as awake): saved and linear
zeroed; else save unless (+0x1e4 bit 5 and not kinematic), clear bit 5, frozen bits 1-6 zero live copy and
saved word (angular z through the FPU, bit 6 loads 0.0f), write back; kinematic zeroes +0x34..+0x48.
Dead store of +0x4c into the dt slot not reproduced (unobservable).
static_proof: "Walked 0x16860-0x169ec: thiscall ret 8; the 000714 call, the integer test of the +0x4c bits, the saved-velocity zeroing/saving rule on +0x1e4 bit 5 and +0x10c bit 7, the per-axis NX_BF_FROZEN_* handling of the live copy and the saved words (angular z through the FPU) and the final kinematic zeroing are reproduced by BodyStep.cpp Row000732Fixture::row000732. Reachable only from the simulation step (000636); no public path while NpScene::simulate is a stub."

### 000734 `Row000734Fixture::row000734(NxReal dt)` (BodyStep.cpp), 0x169f0-0x16a52
thiscall, `ret 4`. x product kept, y/z spilled, z sum spilled and stored.
static_proof: "Walked 0x169f0-0x16a52 (x87): thiscall ret 4; +0x158 += dt * +0x1a0 with the x product in a register and the y/z products spilled to float, as reproduced by BodyStep.cpp Row000734Fixture::row000734. Reachable only from the simulation step (000615 -> 000770 -> row) and the CCD sweep (002264 -> 000774 -> 000772 -> row), neither reconstructed; no public path while NpScene::simulate is a stub."

### 000736 `Row000736Fixture::row000736(NxReal* q, NxReal dt)` (BodyStep.cpp), 0x16a60-0x16bf3
thiscall, `ret 8`, eax 1/0. L^2 == 0 -> 0; k = fsin((dt L) 0.5f)/L and c = fcos(...) with L and the angle
formed inside the helpers; k w spilled to floats; q = (k w, c) q from the original q (four-term sums in the
listing's order); |q| = sqrt(((w w + z z) + y y) + x x) over the stored floats; 0 -> 1 unnormalised, else
q *= 1/|q| (w as inverse * w), 1. Divergence (the accepted `double` convention): MSVC keeps k and c as
double variables and spills them to qwords across the fcos call / under register pressure (53-bit under
0x0f7f, where the oracle keeps them in registers).
static_proof: "Walked 0x16a60-0x16bf3 (x87): thiscall ret 8 returning 1 or 0; |w| == 0 returns 0, the half-angle fsin/fcos (inline instructions, through X87Sqrt.h helpers), k w spilled to float, the quaternion product from the original components in the listing's order and the renormalisation (0 length returns 1 unnormalised) are reproduced by BodyStep.cpp Row000736Fixture::row000736; k and c are double variables the compiler may spill (the accepted convention). Reachable only from the simulation step (000615 -> 000770 -> row) and the CCD sweep (002264 -> 000774 -> 000772 -> row); no public path while NpScene::simulate is a stub."

### 000740 `Row000740Fixture::row000740(NxReal dt)` (BodyStep.cpp), 0x16c20-0x16dc4
thiscall, `ret 4`. +0x1e4 |= 0x200; null actor-body +0x10 object -> clear it from the computed value;
sweep floats; object slot 10 (`call [edx+0x28]`, `Row000740Target::slot10`) fills the sphere; R = sqrt of the
(z, y, x)-ordered squared differences + radius (registers); box min/max stored; each sweep component ordered
above 0 extends max, else min. The object behind slot 10 is unidentified (declared by offset).
static_proof: "Walked 0x16c20-0x16dc4 (x87): thiscall ret 4; the +0x1e4 bit-9 set/clear, the dt * +0x1a0 sweep, the owner object's slot-10 bounding sphere, R = |centre - com| + radius in registers, the +0x244..+0x258 box and the per-axis extension by the sweep sign are reproduced by BodyStep.cpp Row000740Fixture::row000740; its reader 000738 is core/JointSupport.cpp Row000738Fixture::row000738. Reachable only from the simulation step (000615 -> 000770 -> row) with NX_CONTINUOUS_CD set; no public path while NpScene::simulate is a stub."

### 000770 `Row000770Fixture::row000770(NxReal dt, NxReal unused)` (BodyStep.cpp), 0x183a0-0x1846a
thiscall, `ret 8`. +0x23c = 0x7f7fffff, +0x240 = 0, copy +0x134 (9 words) and +0x158 to +0x20c/+0x230; +0x4c
bits != 0: 000734, 000736(+0x124), 000758, 000746, and NX_CONTINUOUS_CD != 0 (NaN counts as set) -> 000740 and
return; else +0x1e4 &= ~0x200.
static_proof: "Walked 0x183a0-0x1846a: thiscall ret 8 (second argument unread); the CCD start-pose save, the integer wake test, the 000734/000736/000758/000746 advance and the NX_CONTINUOUS_CD (000429) test choosing 000740 or the +0x1e4 bit-9 clear are reproduced by BodyStep.cpp Row000770Fixture::row000770. Reachable only from the simulation step (000615); no public path while NpScene::simulate is a stub."

### 000772 `Row000772Fixture::row000772(NxReal toi)` (BodyStep.cpp), 0x18470-0x1853c
thiscall, `ret 4`, al. Only toi ordered below +0x23c: record it, +++0x240, restore +0x158/+0x134 from
+0x230/+0x20c, 000756, t = (float)(toi * Scene+0x548), 000734(t), 000736(+0x124, t), 000758, 000746, true.
static_proof: "Walked 0x18470-0x1853c: thiscall ret 4 returning al; the ordered toi < +0x23c test, the pose restore, 000756, the time rescale by Scene+0x548 spilled to float and the 000734/000736/000758/000746 re-advance are reproduced by BodyStep.cpp Row000772Fixture::row000772. Its only caller is 000774, whose only caller is the CCD sweep 002264 (reached from 002266 in the contact-matrix entries 001749 001772 001820 001851 001919 001929 001933, gated by NX_CONTINUOUS_CD, default 0); 002264 is not reconstructed, so no candidate path reaches it."

### 000774 `Row000774Fixture::row000774(NxReal toi)` (BodyStep.cpp), 0x18540-0x1855f
thiscall, `ret 4`. 000772(toi) true -> 000022 on [+0x19c] with 0.
static_proof: "Walked 0x18540-0x1855f: thiscall ret 4; 000772 and, on true, 000022 on the actor body with 0, reproduced by BodyStep.cpp Row000774Fixture::row000774. Its only caller is the CCD sweep 002264 (see 000772), which is not reconstructed, so no candidate path reaches it."

## Helpers added to X87Sqrt.h (not rows)

- `x87FsqrtQuotDot3(n, ...)`: fsqrt(n / ((a0b0 + a1b1) + a2b2)), 000726's clamp.
- `x87FsinHalfOverNorm3`, `x87FcosHalfNorm3`: 000736's fsin/fcos with L and the angle formed inside.
- `x87RateOverRoot`, `x87AcosRateOverRoot`: 000726's (a invDt + a invDt) / fsqrt(1 - w w), the second calling
  x87CIacos first so the arc cosine stays in st(0).
- `x87CIacos`: a stand-in for the oracle CRT's _CIacos (005697 / 0xf480d): fnstcw, (cw & 0x300) | 0x7f unless
  0x027f, fpatan(fsqrt((1+x)(1-x)), x) for |x| < 1, 0/pi at |x| == 1, NaN passthrough, indefinite otherwise,
  saved word restored. Not reproduced: _87except error reporting (errno/inexact handling); no value change.
  Encodings checked in the obj (`fdivp st(1),st` = DE F9, `fsub st,st(2)` = D8 E2, `fadd st,st(1)` = D8 C1).
  It does not claim 005697 (a CRT row, `classified`).

## Concerns

- The contract's 000762 row says the second body is woken; the listing wakes the record itself twice.
- The contract's 000716 row says the rank is bumped "on a tie"; the listing bumps it on every else-arm merge
  (same in 000718).
- No dynamic evidence is possible (no candidate caller); all rows would be source-only `reconstructed` on
  static proof, like 000738.
- 000738's inventory `source`/proofs still describe the old harness fixture; restate with 000740 (records).
