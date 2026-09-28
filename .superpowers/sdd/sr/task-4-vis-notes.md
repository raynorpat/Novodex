# Task 4, visualisation sub-area: notes

Branch `claude/sr-t4-vis` (from `claude/scene-raycast-block` at bb2e485). Source only: inventory.json, the
ledgers, the contract, gate_targets.ps1 and the timing table are not edited. The integrator registers the
test and records the rows.

## 1. Path decision: public path, feasible, written

The chain, from the oracle listing (every caller below is the row's only caller, found by searching the whole
Capstone listing for `call 0x...` and the image for the address as data):

| Row | RVA | Size | State (inventory) | Unit | Caller | Written here |
|---|---|---:|---|---|---|---|
| 000344 NpScene::visualize | 0xcc10 | 77 | discovered | NpScene.cpp | NpScene table slot 31 (0x10105b14 holds 0x1000cc10) | yes, NpScene.cpp |
| 002364 / 002366 lock / unlock | 0x5b730 / 0x5b790 | 82 / 32 | statically_reviewed | | 000344 | existing NpSceneGuard.h helpers |
| 000657 Scene::visualize | 0x139c0 | 636 | discovered | Scene.cpp | 000344 (0x1000cc4f) | yes, Scene.cpp |
| 000579 Scene::getDebugRenderable | 0x10a10 | 55 | discovered | Scene.cpp | 000657 (0x100139ff) | yes, Scene.cpp |
| 001978 collision pruner vis | 0x4c980 | 165 (+001969 134, +001939/001967/001937/005279) | discovered | gap:ContactPlaneMesh..PenetrationMap | 000657 (always, scale != 0) | placeholder |
| 000020 actor visualisation | 0x1560 | 731 | discovered | gap:<start>..Actor.cpp | 000657 (0x10013b0f) | yes, SceneVisualize.cpp |
| 000766 body visualisation | 0x179a0 | 1377 | discovered | gap:SceneRaycast..CapsuleShape | 000020 (0x1000182b) | yes, SceneVisualize.cpp |
| 004163 vector slot loop | 0x9aca0 | 56 | reconstructed | | 000766 (0x10017ef3) | inline reproduction (not claimed) |
| 000907 pair thunk | 0x1fda0 | 8 | discovered | gap:SceneRaycast..CapsuleShape | 000657 (0x10013b59) | yes, SceneVisualize.cpp |
| 000869 contact visualisation | 0x1d2d0 | 699 | discovered | gap:SceneRaycast..CapsuleShape | 000907 (jmp) | yes, SceneVisualize.cpp |
| 000638 world boxes | 0x128e0 | 428 | discovered | Scene.cpp | 000657, gated AABBS/COMPOUNDS | placeholder |
| 000581 (+000583) shape slot loop | 0x10a50 | 29 (+62) | discovered | Scene.cpp | 000657, gated SHAPES/AXES/SPHERES | placeholder |
| 003639 fluid vis | 0x89dc0 | 52 | discovered | fluids/FluidManager.cpp | 000657, only with a fluid manager at +0x61c | placeholder |
| joint slot +0x10 | | | | core/ | 000657, per joint on +0x59c | existing `Joint::row_slot4` |
| 000663 Scene destructor | 0x13f30 | 906 | discovered | Scene.cpp | | localized edit: releases +0x6b8 |

None of these rows is in NpActor.cpp's unit (0x2610-0xb100), the effector gap or the qhull gap. The rows the
test path runs with the collision, contact, fluid and joint-group parameters at their default 0 are
000344, 000657, 000579, 001978 (draws nothing then), 000020 and 000766. The user sees the Scene's renderable
through `NxPhysicsSDK::visualize` (000252 -> PhysicsSDK::visualize -> Foundation renderDebugData over every
renderable the Foundation holds); 000579 creates it through the Foundation on the first visualize with a
non-zero NX_VISUALIZATION_SCALE.

000869/000907 have no public path: 000657 reaches them only for contact pairs on the Scene's +0x674 list
whose stamp (+0x104) equals the Scene's (+0x540), and only the simulation step creates pairs. They are
static-only.

**Control word.** NxScene::visualize is a user call, so every row runs at API time under 0x027f; none is
reached from the step. The new file keeps the default architecture (SSE2 doubles are the x87 at 53 bits);
nothing is added to the /arch:IA32 list. Square roots through X87Sqrt.h (x87Fsqrt, x87FsqrtDot3); the one
`fistp qword` (000766 0x17bff) through a naked helper `nxVisFistpQword` in SceneVisualize.cpp (rounds by
the live control word, to nearest under 0x027f; a C cast would truncate).

**SDK parameters.** The rows read the live array at .data 0x10123b18 (PhysicsSDK.cpp `gParameter`, static)
at 4 * index. The candidate's convention (the joint rows, revolute-contract.md open issue 8) reads it through
`PhysicsSDK::instance->getParameter(p)`; used here (`nxVisSdkParameter`, `nxSceneVisParameter`). Indices
are pinned by static_asserts against the addresses (13 scale 0x4c, 14 world axes 0x50, 15 body axes 0x54,
16 mass axes 0x58, 17/18 lin/ang velocity 0x5c/0x60, 26 joint groups 0x80, 37-40 contact point/normal/
error/force 0xac-0xb8, 41 actor axes 0xbc, 42-45/48 collision 0xc0-0xcc/0xd8).

Constants read from the pinned image: 0x101041f0 0.0f, 0x101041ec 1.0f, 0x10106884 6.0f, 0x1010688c
2.5000002f (0x40200001), 0x10106888 255.0f, 0x101043cc 0.5f, 0x10106954 0.1f. NxDebugRenderable slots:
+0x18 clear, +0x20 addLine, +0x28 addOBB, +0x30 addArrow, +0x34 addBasis.

## 2. Files

- `Physics/src/SceneVisualize.cpp` (new): 000020, 000766, 000869, 000907; the four placeholders.
- `Physics/src/include/SceneVisualize.h` (new): receiver classes (no data members; member functions give
  __thiscall with the stack argument) and placeholder declarations.
- `Physics/src/Scene.cpp`: 000579 and 000657 appended at the end; three includes; 000663's emulation
  (`nxSceneDelete`) releases +0x6b8 through the Foundation (0x14066-0x14092) after the joint arrays.
- `Physics/src/include/Scene.h`: `visualize()` and `getDebugRenderable()` members, `class NxDebugRenderable;`.
- `Physics/src/NpScene.cpp`: 000344 replaces the `// (unimplemented) visualize` stub.
- `CMakeLists.txt`: SceneVisualize.cpp added to NxPhysicsInternalTests' and NxPhysicsCollisionTests' source
  lists (the DLL globs it); draft target NxPhysicsSceneVisualizeTests (with NX_PAGE_GUARDED_FILL).
- `tests/PhysicsSceneVisualizeTests.cpp` (new, draft, not in gate_targets.ps1).

## 3. Per row

### 000344 (0x0000cc10, 77 B) NpScene::visualize, NpScene.cpp
Walked 0x1000cc10-0x1000cc5c. thiscall, no stack arguments, `ret`. tryLock(+0xc) (002364); on failure the
`[0x101041b0]` instance test (int3) and error(2, "\Epic\Novodex\SDKs\Physics\src\NpScene.cpp", 0x13c, 0,
deadlock message) through `[0x101041b4]`, return without unlock. Otherwise the link is loaded before the
call, 000657 on +0x24, then the unlock (002366, a tail jump) on the kept link. Reproduced exactly as the
Task 3 raycast wrappers (`nxNpSceneGuardWriteTry` / `nxNpSceneGuardLeave`). Divergence: the unlock is a
call, not a tail jump (code shape).
static_proof: "Listing 0x1000cc10-0x1000cc5c walked (scene-raycast Task 4, visualisation): thiscall, ret;
the write lock at +0xc (002364), on failure the deadlock report (code 2, NpScene.cpp line 0x13c) and a
return without unlock; otherwise the link kept, Scene::visualize (000657) on +0x24 and the unlock (002366) on
the kept link, reproduced as NpScene::visualize (Physics/src/NpScene.cpp) through the NpSceneGuard.h lock
helpers; the unlock is a call where the image tail-jumps. Runs at API time (0x027f)."

### 000657 (0x000139c0, 636 B) NxSceneInternal::visualize, Scene.cpp
Walked 0x100139c0-0x10013c3b. thiscall, no stack arguments, `ret`. +0x70c bit 1 -> return; clear (+0x18) of
+0x6b8 if set; NX_VISUALIZATION_SCALE == 0 -> return (fucompp; NaN continues); 000579 (result unused);
001978 on +0x624; world axes (0x13a16-0x13ae2: addBasis at the origin, identity, lengths 1, scale the raw
parameter, colours ffff0000/ff00ff00/ff0000ff); actors +0x55c..+0x560 (count = sar of the byte difference,
compared unsigned, taken once, renderable re-read per element) -> 000020 on element +0x14; joints from +0x59c
through +0x10 -> slot +0x10 (`Joint::row_slot4`); pairs from +0x674 through +0x08 with +0x104 == Scene +0x540
-> 000907; AABBS -> 000638(engine, r, 0xffffff00, 0); COMPOUNDS -> 000638(engine, r, 0xffff00ff, 1); SHAPES or
AXES or SPHERES (jp/jp/jnp) -> 000581(engine, r); +0x61c -> 003639. Reproduced in full; divergences: the
parameters are read through getParameter; 001978, 000638, 000581 and 003639 are placeholders that draw
nothing (the row's own body is complete, the chain is not when those parameters are set or a fluid manager
exists).
static_proof: "Listing 0x100139c0-0x10013c3b walked (scene-raycast Task 4, visualisation): thiscall, ret;
+0x70c bit 1 gate, clear of the renderable at +0x6b8, the NX_VISUALIZATION_SCALE gate, 000579, 001978 on the
pruning engine (+0x624), the world-axes basis, 000020 per actor (+0x55c array, element +0x14), joint slot
+0x10 per joint (+0x59c list, link +0x10), 000907 per pair on +0x674 whose +0x104 stamp equals +0x540, the
AABB/compound (000638), shape (000581) and fluid (003639) calls, reproduced as NxSceneInternal::visualize
(Physics/src/Scene.cpp); parameters read through PhysicsSDK::getParameter; 001978, 000638, 000581 and 003639
are unwritten placeholders that draw nothing. Runs at API time (0x027f)."

### 000579 (0x00010a10, 55 B) NxSceneInternal::getDebugRenderable, Scene.cpp
Walked 0x10010a10-0x10010a46. thiscall, `ret`. If +0x6b8 is null: FoundationSDK instance (int3 when null),
its NxFoundationSDK part (+0x14) slot +0x1c createDebugRenderable, stored; returns +0x6b8. Exact.
static_proof: "Listing 0x10010a10-0x10010a46 walked (scene-raycast Task 4, visualisation): thiscall, ret;
creates the Scene's debug renderable at +0x6b8 on first use through the Foundation instance's
createDebugRenderable (slot +0x1c of its NxFoundationSDK part at +0x14, int3 without an instance) and
returns it, reproduced as NxSceneInternal::getDebugRenderable (Physics/src/Scene.cpp). Runs at API time."

### 000020 (0x00001560, 731 B) NxActorVisualRecord::visualize, SceneVisualize.cpp
Walked 0x10001560-0x10001838. thiscall on the 0x50-byte actor record (NpActor +0x14), `ret 4`. When
NX_VISUALIZE_ACTOR_AXES != 0 (NaN draws): the global pose formed twice inline (body at +0x08: rotation of the
body quaternion +0x24 in the spill pattern of `nxNpActorComposeRotation` -- 2yy, 2xz, 2yw, 1-2xx, 2yz spilled
to float, checked instruction by instruction at 0x100015cf-0x1000168c and 0x100016ef-0x100017ac -- and
position +0x18; no body: the 12 floats at +0x20); addBasis (slot +0x34) with the first copy's rotation, the
second copy's position, lengths (1,1,1), scale the raw parameter, colours ffff0000/ff00ff00/ff0000ff. Then,
unconditionally, 000766 on +0x08 when non-null. Divergences: the dead store `[esp+8] = 0` (0x1000157f) is
not reproduced; the parameter read through getParameter.
static_proof: "Listing 0x10001560-0x10001838 walked (scene-raycast Task 4, visualisation): thiscall on the
actor record, ret 4; under NX_VISUALIZE_ACTOR_AXES the global pose (body quaternion +0x24 with the listing's
five float spills and position +0x18, or the record's own pose at +0x20) formed twice and drawn by addBasis
(slot +0x34) with the first copy's rotation, the second's position, unit lengths, the raw parameter as
scale and colours ffff0000/ff00ff00/ff0000ff; then 000766 on the body (+0x08); reproduced as
NxActorVisualRecord::visualize (Physics/src/SceneVisualize.cpp). Runs at API time (0x027f)."

### 000766 (0x000179a0, 1377 B) NxBodyVisualRecord::visualize, SceneVisualize.cpp
Walked 0x100179a0-0x10017efe. thiscall on the 0x260-byte body record, `ret 4`. Gate +0x10c & 0x100
(NX_BF_VISUALIZATION). Blocks, each under its parameter != 0 (NaN draws):
- BODY_AXES 0x179b5-0x17b00: s = scale * param (register); the nine products of the +0x134 matrix by s all
  spilled to float except +0x140 (column 0 y), which is added to centre.y in the register; three addLine from
  the centre (+0x158) in ff0000, ff00, ff (columns 0, 1, 2).
- BODY_MASS_AXES 0x17b03-0x17c24: k = 6.0f / +0x188; e0 = fsqrt(((I194 + I190) - I18c) k), e1 =
  fsqrt(((I18c - I190) + I194) k) (floats), e2 = fsqrt(((I18c + I190) - I194) k) (register); h = float((param
  * scale) * 0.5f); extents (e0 h, e1 h, e2 h), centre +0x158, rotation +0x134 (rep movsd); grey =
  fistp((+0x4c * 2.5000002f) * 255.0f), colour ((g | 0xffffff00) << 8 | g) << 8 | g; addOBB(box, colour,
  false).
- BODY_LIN_VELOCITY 0x17c27-0x17d7b, BODY_ANG_VELOCITY 0x17d7d-0x17ece: t = float(param * scale); for
  +0x34 (ffffff), +0x1a0 (8000), +0x40 (0), +0x1ac (5f1fe0): |v| = fsqrt((zz + yy) + xx) to float; when
  non-zero, r = 1.0f / |v| in the register, direction (x r, y r, z r) floats, addArrow(centre, dir, |v|, t,
  colour).
- BODY_JOINT_GROUPS 0x17ed0-0x17ef3: 004163 on +0x1e0 when non-null.
Divergences: parameters through getParameter; 004163 reproduced inline as `nxVisIslandMembers` (a static
noinline copy that calls each element's own slot +0x18 and re-reads the count per iteration, as the listing
does) because the claimed kernel `nxVectorVirtualLoop4163` (ObjectModel.cpp) calls a fixed member function
and caches the count once; the fistp through a naked helper.
static_proof: "Listing 0x100179a0-0x10017efe walked (scene-raycast Task 4, visualisation): thiscall on the
body record, ret 4; the NX_BF_VISUALIZATION gate (+0x10c bit 8), the mass-frame axes (+0x134 columns times
scale * param from the centre of mass +0x158, the +0x140 product kept in the register), the inertia box
(half extents fsqrt of the principal-moment sums times 6/mass, times float(param * scale * 0.5), rotation
+0x134, grey from fistp(+0x4c * 2.5000002 * 255), addOBB), the linear (+0x34, +0x1a0) and angular (+0x40,
+0x1ac) arrows (length fsqrt((zz + yy) + xx) to float, direction times the register reciprocal) and the
island loop 004163 on +0x1e0, reproduced as NxBodyVisualRecord::visualize (Physics/src/SceneVisualize.cpp);
parameters read through PhysicsSDK::getParameter, square roots through X87Sqrt.h, the fistp through a naked
x87 helper, 004163 as an inline copy. Runs at API time (0x027f)."

### 000869 (0x0001d2d0, 699 B) NxPairContactVisual::visualize, SceneVisualize.cpp
Walked 0x1001d2d0-0x1001d586. thiscall on the pair's contact sub-object (pair +0x14), `ret 4`. The stream
at +0x40: count word; patch headers of 12 bytes whose third word has the normal count (low 16) and flags
(bits 16-23), flag 2 = skip header; per normal 16 bytes (normal, point count); per point 16 bytes (position,
separation) + 4 if flag 1 + (4, or 8 when the separation's sign bit is set) if flag 4. Per point: FORCE
(bb8 * scale, ff0000; the length is the parameter, not the force) else NORMAL (bb0 * scale, ff) else ERROR
(|-|sep| * bb4 * scale|, ffff00); addLine(point, point + len n) when len != 0 (x, y products spilled to
float, z kept); then under CONTACT_POINT a cross of half size d = float(float(bac * scale) * 0.1f), three
addLine in ff where lines 2 and 3 re-add/re-subtract d from line 1's rounded coordinates. Reproduced in full.
Dependency: the stream format and the pair offsets (+0x14 sub-object, +0x40 stream = pair +0x54, link +0x08,
stamp +0x104; Scene list +0x674, stamp +0x540) are declared by raw offset here; the contact-pair manager
sub-area (000855-000921, e.g. the stream writers 000873/000875) must produce this layout.
static_proof: "Listing 0x1001d2d0-0x1001d586 walked (scene-raycast Task 4, visualisation): thiscall on the
pair's contact sub-object, ret 4; the contact stream at +0x40 (12-byte patch headers with the normal count
and flags, flag 2 skipped; 16-byte normals with point counts; 16-byte points plus the flag 1/flag 4
extras), one addLine per point along the normal of length NX_VISUALIZE_CONTACT_FORCE, _NORMAL or
|separation| * _ERROR times the scale (ff0000/ff/ffff00), and the NX_VISUALIZE_CONTACT_POINT cross of half
size 0.1 * scale, reproduced as NxPairContactVisual::visualize (Physics/src/SceneVisualize.cpp). Reached
from NxScene::visualize only for contact pairs on the Scene's +0x674 list, which only the simulation step
creates; no public path while NpScene::simulate is a stub. Runs at API time (0x027f)."

### 000907 (0x0001fda0, 8 B) NxActorPairVisual::visualize, SceneVisualize.cpp
Walked 0x1001fda0-0x1001fda7: `add ecx, 0x14; jmp 0x1001d2d0`. Reproduced as a member forwarding to
000869 on this + 0x14 (000869 is noinline, so MSVC emits the thunk: `push ebp; mov ebp, esp; add ecx, 14h;
pop ebp; jmp` -- the frame pointer is the only difference).
static_proof: "Listing 0x1001fda0-0x1001fda7 walked (scene-raycast Task 4, visualisation): `add ecx, 0x14;
jmp 000869`, reproduced as NxActorPairVisual::visualize (Physics/src/SceneVisualize.cpp), which forwards to
NxPairContactVisual::visualize on the pair's +0x14. Called by 000657 only for contact pairs on the Scene's
+0x674 list, which only the simulation step creates; no public path while NpScene::simulate is a stub."

## 4. Test draft: NxPhysicsSceneVisualizeTests

tests/PhysicsSceneVisualizeTests.cpp, built (CMake target added, NX_PAGE_GUARDED_FILL), not registered.
Seven actors (static box, rotated static box, dynamic box, rotated dynamic box, sphere, rotated capsule,
dynamic box without NX_BF_VISUALIZATION), each body with explicit mass, massSpaceInertia and velocities;
stages: no parameters (no renderable), scale 0, scale only, world axes, actor axes, body axes, mass axes,
lin velocity, ang velocity, all, all again, cleared (scale 0 with the renderable existing), moved bodies,
and after releaseScene (no renderable). Every point/line/triangle the renderer receives is printed as words.
Stdout: 878 lines per side.

Runs (pairs staged by copying into build/vispairs/{oracle,candidate,mixed}):
- oracle pair (NxPhysics 4b7db3e1..., NxFoundation 7e0596e4...): exit 0, stderr empty.
- candidate pair (NxPhysics 25eee77b..., NxFoundation 0893e628...): exit 0, stderr empty; 77 of 873
  non-identity lines differ, all by 1 ulp in points the Foundation's DebugRenderable computes (arrow lobes of
  addArrow/addBasis, and 4 addOBB corners with a near-zero coordinate): stages actor_axes 5, lin_velocity 3,
  ang_velocity 10, mass_axes 5, all 20, again 20, moved 14.
- mixed pair (candidate NxPhysics + the oracle NxFoundation): 0 of 873 lines differ. The candidate rows
  hand the renderable bit-identical arguments; the remaining delta is NxFoundation.dll's
  (Foundation/src/DebugRenderable.cpp addArrow line 176 and addOBB), outside this sub-area.

## 5. Findings outside this sub-area (not fixed)

1. Candidate NxFoundation.dll DebugRenderable::addArrow/addOBB differ from the oracle Foundation by 1 ulp
   (see 4). The test cannot be registered with stdout_delta=0 until that is fixed or the cases avoid it.
2. Candidate body creation (Scene.cpp `nxActorComputeMass`, ~line 2020) derives the inertia only from a
   box shape with density; with a given mass and no massSpaceInertia +0x18c..+0x194 stay 0 (the oracle runs
   000947 -> 000849 ...). The first test run showed a zero inertia box; the draft gives every body its
   inertia.
3. The oracle's NxScene::releaseActor leaves released actors on the +0x55c array (all 7 still drawn after
   releasing all), the candidate removes them at once. The draft does not release actors before the scene.
4. +0x1e0 (island object) is never set by the candidate (000722/000748 missing), so
   NX_VISUALIZE_BODY_JOINT_GROUPS is not exercised.
5. ObjectModel.cpp `nxVectorVirtualLoop4163` caches the count once; the 004163 listing re-reads it after
   every call (0x9acc4-0x9accb).
6. 000663's emulation (`nxSceneDelete`) did not release the debug renderable; fixed locally (without it
   the candidate's Foundation keeps rendering a destroyed Scene's renderable).

## 6. Follow-up (branch `claude/sr-t4-vis2` from 914f6b0): candidate Foundation made faithful

Finding 5.1 fixed (a faithful fix, not the test-case fallback). The Foundation oracle has no Capstone listing
in the repo and no stable-ID convention (no `// *_fn_` lines in Foundation/src; its x87 functions are
documented by oracle address in comments, as Utilities.cpp's NxNormalToTangents is). The pinned oracle
NxFoundation.dll (D:\FlamingEnt__\Unreal_3\Binaries, sha256 7e0596e4...) was disassembled with Capstone 5.0.6
over the pefile image (base 0x10000000). DebugRenderable's vtable is 0x1001c1c8 (set by the ctor 0x100028c0
that createDebugRenderable 0x10003f10 calls): +0x20 addLine 0x100028f0, +0x28 addOBB 0x10001fb0, +0x2c
addAABB 0x10001520, +0x30 addArrow 0x10001640, +0x34 addBasis 0x10001860, +0x38 addCircle 0x10001930. Rule
applied: x87 register lifetimes NxF64, dword spills NxF32 (the process is at _PC_53, so SSE2 doubles
reproduce the finite words; no /arch:IA32 added -- only NaN payloads would need it, as for Utilities.cpp,
and none is compared here).

Changes:
- `Foundation/src/DebugRenderable.cpp` addArrow, from 0x10001640-0x10001858: arrowLength = float(length *
  scale); tip x/y = float(double(L * d) + p), z product spilled first; headScale (0.15f at 0x1001c1bc) kept in
  the register for tipBase.x only, its spilled float used everywhere else; tipBase.x spilled, .y (double
  product) and .z (minus the spilled product) kept on the stack for all four lobes; per tangent: x product
  spilled, y product kept, z product kept for the sum and spilled for the difference. Tangents through the
  existing NxNormalToTangents (0x100062b0).
- addBasis, from 0x10001860-0x1000192a: a null `colors` passes colour 0 (the listing tests the pointer per
  arrow); otherwise unchanged (columns 0/1/2, each arrow through the vtable).
- `Foundation/src/Box.cpp` NxComputeBoxPoints (the addOBB corners; addOBB 0x10001fb0-0x100020ec itself only
  adds floats for the frame and was already exact), from 0x10007cf0-0x10007f3a: the z products of Axis1 and
  Axis2 stay in registers; Axis1+Axis2 keeps x/y (double) and spills z = float(a2z + a1z); Axis1-Axis2 spills
  x/y and keeps z (double). The candidate NxPhysics does not import NxComputeBoxPoints (the oracle's does).
- Not touched: addAABB (0x10001520, not on this test's path), addCircle, addLine/addPoint.

Verification (own build dir `build`, pairs staged as run_differential.ps1 does, in `build/pairs-vis2`):
- candidate NxPhysics 814c8812..., candidate NxFoundation 422f14b1...; oracle 4b7db3e1... / 7e0596e4....
- NxPhysicsSceneVisualizeTests: oracle exit 0, candidate exit 0; 873 compared lines (pair/module lines
  dropped), stdout diff 0, stderr identical (empty). Before: 77 lines off by 1 ulp.
- NxFoundationTangentTests (38 lines), NxFoundationCustomArrayTests (1 line): diff 0 on both pairs.
- NxFoundationSDKTests <dll> (7 lines), NxFoundationExportTests <dll>, NxFoundationClusterTests
  {exception, observable, profiler, time, fpu, util, box, capsule, sphere, ray_seg, volume, debug} <dll>:
  exit 0 both sides, stdout and stderr identical (the box group's bit-level line included).
- run_phase_gate.ps1 -Phase 6 (NxFoundationTangentTests among its differentials): status=pass, every target
  stdout_delta=0. -Phase 2: status=pass, every target stdout_delta=0.
The test can now be registered with stdout_delta=0 (not done here: registration and records are the
integrator's).

## 7. Integration (onto claude/scene-raycast-block 4c74429)

Cherry-picked 914f6b0 and 697a25b; one conflict (CMakeLists.txt's InternalTests/CollisionTests source lists, merged
with BodyCreation.cpp and StepOnlyRows.cpp). Records: units/scene-raycast-contract.md `## Task 4 results:
visualisation`; evidence/scene-raycast-trace-task4-vis.txt; evidence/phase6-joints.md 18e (the Foundation change);
timing rows in evidence/scene-raycast-block.md.

- Review: 000344, 000657, 000020 walked in full, 000766 over 0x179a0-0x17d7b and 0x17ea6-0x17efe; addArrow checked
  against the oracle Foundation's disassembly. Faithful.
- 000869/000907 moved onto ContactPairManager.h: `NxActorPair::row000869` (stream = the SdkContainer at +0x38's
  entries, the layout 000873/000875 write) and `NxPairNode::row000907`; 000657 walks the +0x674 list as NxPairNode.
- 000579 made noinline (the candidate had inlined it; trace 11/0 before, 11/11 after).
- 000945 (BOX slot 3): reads NX_VISUALIZE_COLLISION_SHAPES through nxSdkParameterTable() instead of the probe-bound
  g_nxGuardC; draws through NxDebugRenderable::addOBB. The ObjectLayout and ShapeVtable harnesses (no PhysicsSDK.cpp)
  define nxSdkParameterTable; ObjectLayout points it at the oracle's .data 0x123b18 during its slot-3 contract.
  Not reached from NxScene::visualize (000581 is a placeholder); promoted on the static proof with the harness drive
  (65/65) recorded.
- Test: a joint_groups stage (and the parameter in `all`); registered on Phase 7 (185 oracle lines, floor 676).
- Findings 5.2-5.4 re-checked: still true on HEAD (inertia with mass only, +0x1e0 stays 0 before a step, released
  actors leave +0x55c at once); the test keeps its workarounds.
- Promoted: 000344, 000657, 000579, 000020, 000766 (dynamic), 000945, 000869, 000907 (static); 8 rows, 3,687 B.
