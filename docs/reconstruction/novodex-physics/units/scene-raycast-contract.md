# SceneRaycast..CapsuleShape block: audit contract

Written by Task 1 of `docs/superpowers/plans/2026-09-28-scene-raycast-block.md`. It covers every code
row of the work units `SceneRaycast.cpp` (10 rows), `gap:SceneRaycast.cpp..CapsuleShape.cpp` (141 rows)
and `CapsuleShape.cpp` (5 rows), 156 rows in all, in every state.

Revision 2 (after the Task 1 review) re-sweeps every implemented row against three rules: CRT math
where the oracle uses x87, the allocator, and the calling convention. It splits the sub-areas and adds
three sections: the handover to the NpActor session, the in-scope raycast entry rows, and the state
mismatches with recommended actions. See `## Review rules applied (revision 2)` below.

Evidence used:
- The Capstone listing (`oracle/capstone/manifest.json`, authoritative) for every row.
- The bundles `units/SceneRaycast.cpp.md`, `units/gap__SceneRaycast.cpp__to__CapsuleShape.cpp.md` and
  `units/CapsuleShape.cpp.md`, for prototypes, callers, callees and strings. The Ghidra decompiles in the
  bundles were used only as a cross-check.
- `oracle/ghidra/supplement.json`, rerun in this task for the 29 rows that had no `ok` decompile. The
  requested set is the union of the 34 existing RVAs and these 29. All 63 came back `ok`, and the 34
  existing entries are byte-identical.
- The NpScene public table at .rdata 0x10105a98, the NpActor table at 0x10104530, and the shape and
  facade tables named per row.
- Two cdb traces over the 16 staged-pair targets (`evidence/scene-raycast-trace-audit.txt`):
  - one of the oracle, with a breakpoint on every row, which fills the "Reached by" column;
  - one of the candidate, with a breakpoint on every mapped candidate symbol, which fills the
    "Candidate executed" column.

Definitions:
- **Status.** `implemented` means a product function, or a code region inlined into one, reproduces the
  whole row. `partial` means some of it is reproduced (the verdict says what is missing). `missing` means
  nothing reproduces it. Placeholders do not count as candidates: `// (unimplemented)` stubs,
  `return 0;` bodies, harness-only fixtures, and descriptions in the inventory `source` field.
- **Verdict.** `faithful` only where the listing was walked instruction by instruction against the
  candidate. Otherwise the verdict names the defect and the oracle addresses of the divergence.
- **Reached by.** Oracle hit counts per staged-pair target. Abbreviations: Life = ActorLifecycle,
  DynF = DynamicFirst, Name = ActorName, Meta = ActorMetadata, BFlag = ActorBodyFlag, Dyn = ActorDynamics,
  DSet = ActorDynamicSetter, Mom = ActorMomentum, Force = ActorForce, CMass = ActorCMass,
  ShMut = ActorShapeMutation, JSP = JointStagedPair, JAll = JointAllocator, JSlot = JointSlot.
  EmptyScene and FoundationTangent reach none of the 156 rows.
- **Candidate executed.** Candidate hit counts at the mapped symbol. `no own address` means the
  candidate is inlined, so no breakpoint can be set on it. `not hit` means a breakpoint was set and never
  fired.
- **Step only.** `yes` when the row's only oracle path is the simulation step, or when it is a fragment of
  a step row.

## Audit summary


Rows (bytes) by sub-area and status. All 156 rows are counted, whatever their inventory state.

| Sub-area | implemented | partial | missing | total |
|---|---:|---:|---:|---:|
| scene raycast | 0 (0) | 0 (0) | 10 (2082) | 10 (2082) |
| body-actor math | 8 (4858) | 9 (11446) | 6 (2604) | 23 (18908) |
| island | 6 (601) | 1 (117) | 12 (2022) | 19 (2740) |
| CCD | 0 (0) | 0 (0) | 4 (685) | 4 (685) |
| shape | 26 (2416) | 7 (932) | 3 (2553) | 36 (5901) |
| mass properties | 25 (3713) | 0 (0) | 1 (43) | 26 (3756) |
| contact-pair manager | 1 (103) | 1 (706) | 32 (13082) | 34 (13891) |
| visualisation | 0 (0) | 0 (0) | 3 (2084) | 3 (2084) |
| other | 0 (0) | 0 (0) | 1 (13) | 1 (13) |
| total | 66 (11691) | 18 (13201) | 72 (25168) | 156 (50060) |

Verdicts on the 84 implemented or partial rows: 36 faithful, 48 with a defect cited by address.
Inventory states today: discovered 91, dynamically_gated 14, reconstructed 51.
Discovered rows that are implemented and faithful (the Task 2 promotion candidates): 000746, 000923.

## Rows

This table is the Task 1 audit. `## Task 2 results` and `## Task 3 results` (at the end) give the new
candidate, status and verdict of every row those tasks changed, and supersede this table for those rows.

| Row | RVA | B | State | Role (callers -> callees; strings) | Sub-area | Candidate (file:line symbol) | Status | Verdict | Reached by (oracle trace) | Candidate executed | Step only | Public path |
|---|---|---:|---|---|---|---|---|---|---|---|---|---|
| 000688 | 0x153c0 | 339 | discovered | Closest-bounds loop. Custom convention: ecx=collector, ebx=NxRaycastHit*, stack (ray, maxDist, groups), cdecl `ret`. For each element S=[e+4]: skip if byte S+0xde&0x40; group word S+0xd8!=0xffff must have its bit set in groups. Gets the world AABB: Prunable at S+0xa4, handle word S+0xcc (0xffff gives a null box). Runs 004886 when [S+0xac]&2 is clear. Then 001722 NxRayAABBIntersect(min,max,orig,dir,&pt), NxComputeDistanceSquared import [0x1010419c](ray,&pt,0), fsqrt. Keeps the hit when sqrt<=maxDist and the squared \|orig-pt\| < hit.distance(+0x20). On a keep it writes hit.distance=squared value, hit.shape=[S+0x9c], hit.worldImpact=pt, hit.flags(+0x2c)=0x13. Callers 000696. Callees 004886, 001722. No strings. | scene raycast | none (NpScene.cpp:481 is a `return 0` stub) | missing | - | none | - | no | NxScene::raycastClosestBounds (slot 46, 000374) -> 000696 -> 000688 |
| 000690 | 0x15520 | 202 | discovered | Scene::raycastAnyBounds. thiscall(Scene; ray, shapesType, groups, maxDist), `ret 0x10`, returns bool. Unit-direction check \|d.d-1.0f(0x101041ec)\| < double eps (0x10106850); failing it reports error(1, F1, 0x147, E1) and returns false. Zeroes collector count ([+0x504]) when non-zero. mask = (type&1 ? 1:0) \| (type&2 ? 0xe:0). Calls 004864(Scene+0x624; collector, ray, mask, maxDist, 0, -1), then 000680(eax=collector, ebx=ray; maxDist, groups). Callers 000366. Callees 004864, 000680. Strings E1, F1. | scene raycast | none (NpScene.cpp:457 stub) | missing | - | none | - | no | NxScene::raycastAnyBounds (slot 42, 000366) |
| 000692 | 0x155f0 | 206 | discovered | Scene::raycastAllBounds(ray, report, type, groups, maxDist, hint), `ret 0x18`. Same check (line 0x170). Then collector reset, mask, 004864(..., maxDist, 0, -1), 000682(eax=collector, ebx=ray; maxDist, report, groups). Returns 000682's count. hintFlags is unused. Callers 000370. Callees 004864, 000682. Strings E1, F1. | scene raycast | none (NpScene.cpp:469 stub) | missing | - | none | - | no | NxScene::raycastAllBounds (slot 44, 000370) |
| 000694 | 0x156c0 | 207 | discovered | Scene::raycastAllShapes(ray, report, type, groups, maxDist, hint), `ret 0x18`. Same check (line 0x182). Then collector reset, mask, 004864, 000684/000686(eax=collector, esi=ray, ebx=groups; maxDist, report, hint). **Always returns 0** (`xor eax,eax` at 0x10015789); 000684's count is discarded. Callers 000372. Callees 004864, 000684. Strings E1, F1. | scene raycast | none (NpScene.cpp:475 stub) | missing | - | none | - | no | NxScene::raycastAllShapes (slot 45, 000372) |
| 000696 | 0x15790 | 245 | discovered | Scene::raycastClosestBounds(ray, type, hit, groups, maxDist, hint), `ret 0x18`. Same check (line 0x19b). Initialises hit.distance=FLT_MAX, hit.shape=0, hit.flags=1. Then collector reset, mask, 004864, 000688(ecx=collector, ebx=hit; ray, maxDist, groups). If hit.shape!=0: hit.distance=fsqrt(hit.distance) and returns [hit.shape+8] (the internal shape); else returns 0. Callers 000374. Callees 004864, 000688. Strings E1, F1. | scene raycast | none (NpScene.cpp:481 stub) | missing | - | none | - | no | NxScene::raycastClosestBounds (slot 46, 000374) |
| 000698 | 0x15890 | 126 | discovered | Any-shape loop. eax=collector; stack (ray, maxDist, groups); cdecl `ret`. Same disable/group filter as 000688. Calls shape vtable slot 5 [vt+0x14] (ray, maxDist, groups, 0, &localHit[0x30]) and returns 1 at the first non-null result, else 0. Callers 000704. Callees: indirect slot 5 (ShapeRaycast.cpp functions). | scene raycast | none | missing | - | none | - | no | NxScene::raycastAnyShape (slot 43, 000368) -> 000704 -> 000698 |
| 000700 | 0x15910 | 29 | discovered | Entry of the closest-shape loop; the body is 000702 (`jmp 0x10015930`). ecx=collector, ebx=ray, esi=hit (registers); stack (maxDist, groups, hint), cdecl `ret`. Loads count/data, reserves a 0x34 frame. Callers 000706. | scene raycast | none | missing | - | none | - | no | NxScene::raycastClosestShape (slot 47, 000376) -> 000706 -> 000700 |
| 000702 | 0x15930 | 277 | discovered | Continuation of 000700 (no callers of its own). Per element, same filter, then slot 5 (ray, maxDist, groups, hint, &local). Picks the candidate key: squared \|impact-orig\| if local.flags&2 (IMPACT), else the raw local.distance if &0x10, else FLT_MAX (0x10106858). If key < hit.distance, copies the whole 0x30 local hit to *hit, sets distance=key, hit.shape=[ret+0x9c] and flags\|=0x11. Loop count lives at [esp+8], ends `ret` at 0x10015a44. | scene raycast | none | missing | - | none | - | no | as 000700 |
| 000704 | 0x15a50 | 202 | discovered | Scene::raycastAnyShape(ray, type, groups, maxDist), `ret 0x10`, returns bool. Check at line 0x159. Then reset, mask, 004864, 000698(eax=collector; ray, maxDist, groups). Callers 000368. Callees 004864, 000698. Strings E1, F1. | scene raycast | none (NpScene.cpp:463 stub) | missing | - | none | - | no | NxScene::raycastAnyShape (slot 43, 000368) |
| 000706 | 0x15b20 | 249 | discovered | Scene::raycastClosestShape(ray, type, hit, groups, maxDist, hint), `ret 0x18`. Check at line 0x1bb. Initialises hit (distance=FLT_MAX, shape=0, flags=1). Then reset, mask, 004864, 000700(ecx=collector, ebx=ray, esi=hit; maxDist, groups, hint). If hit.shape: distance=fsqrt(distance) and returns [hit.shape+8]; else 0. Callers 000376. Callees 004864, 000700. Strings E1, F1. | scene raycast | none (NpScene.cpp:487 stub) | missing | - | none | - | no | NxScene::raycastClosestShape (slot 47, 000376) |
| 000708 | 0x15c20 | 130 | discovered | Copy-back of the per-step JointSupportBody. thiscall(body), plain `ret`. With R=[body+0x204]: body+0x34..0x3c = R+0x00..0x08 (linear velocity); body+0x40..0x48 = R+0x10..0x18; body+0x1e4 \|= 0x20; body+0x1a0..0x1a8 = R+0x44..0x4c; body+0x1ac..0x1b4 = R+0x50..0x58. Caller: the loop at 0x10011370-0x1001137e (000613, the continuation of 000611; the bundle lists 000611). | island | none (the layout is documented at include/core/JointSupport.h:37-58, JointSupportBody; no code) | missing | - | none | - | yes | simulation step only (NxScene::simulate -> worker 002400 @0x1005ba23 -> 000659 -> 000655 -> 000611/000613 -> 000708) |
| 000710 | 0x15cb0 | 122 | discovered | Per-body force/acceleration reset. thiscall(body; const NxVec3* gravity), `ret 4`. If byte body+0x10c bit0 (NX_BF_DISABLE_GRAVITY) is clear, body+0x88..0x90 = *gravity, else 0. Then zeroes body+0x94..0xb4 (nine dwords: accumulated torque/force). Caller 000619 (loop over Scene+0x56c..0x570 bodies, gravity = Scene+0x520). No callees. | body-actor math | none | missing | - | none | - | no | NxScene::fetchResults (slot 66, 000398 @0x1000d777) -> 000619 -> 000710 |
| 000712 | 0x15d30 | 32 | reconstructed | Joint-island find(): thiscall(body), `ret`. Root at +0x1bc, recursive with path compression. Re-reads +0x1bc for the return value. Callers 000716/000720/000722/000748/000762/000764/000778/000785. | island | core/JointSupport.cpp:415 Row000712Fixture::row000712 | implemented | faithful | none | not hit | no | Scene 000633 (Scene.cpp joint removal) -> 000778 -> 000712; also the step (000716/000720/000722) |
| 000713 | 0x15d50 | 32 | reconstructed | Sleep-group find(): thiscall(body), `ret`. Same shape as 000712 on +0x1e8. Callers 000655, 000718, 000724, 000744, 000776, 003979. | island | ObjectModel.cpp:3393 nxBodyRecordFixRoot (a second copy is NpActor.cpp:2329 nxNpActorGroupRoot) | implemented | defect: calling convention. The oracle is thiscall (`mov esi,ecx` at 0x10015d51; no stack argument, plain `ret` at 0x10015d6f). The candidate is a cdecl free function taking `rec` on the stack. Logic, offsets and the reload order match. | DSet 4 | yes: DSet 4 | no | NxActor::isGroupSleeping (NpActor table 0x10104530 slot 67, 000062) -> 000744 -> 000713; also the step |
| 000714 | 0x15d70 | 328 | discovered | Per-body sleep/wake-counter update. thiscall(body; float dt), `ret 4`. G=root cache [+0x1e8]. If body+0x1b8 != G+0x1f4 and +0x114 bit8 is clear, raises +0x4c to 0x3ecccccc when it is below the floor [0x101053d4]. Then +0x1b8=G+0x1f4. Uses the saved velocities +0x1a0/+0x1ac when +0x1e4&0x20, else the live +0x34/+0x40. Compares \|v\|^2 vs +0xd0 and \|w\|^2 (spilled to float) vs +0xd4. If both are below and +0x4c!=0: +0x4c-=dt, clamped to 0 (0x101041f0). Otherwise the same wake raise. Finally clears +0x114 bit8. Caller 000732. | island | none | missing | - | none | - | yes | simulation step only (000655 -> 000636 -> 000732 -> 000714) |
| 000716 | 0x15ec0 | 277 | discovered | Joint-island union-by-rank. thiscall(body a; body b), `ret 4`. Finds both roots via 000712. If they differ, the lower-rank (+0x1c0) root is attached under the other. Sets +0x1bc; bumps rank on a tie (inc). Splices the member lists via tail +0x1d4 / next +0x1d0. Sums +0x1c4 and +0x1c8. Sets +0x1e4\|=2 on the surviving root and clears it on the absorbed one. Caller 000762. Callees 000712. | island | none | missing | - | none | - | yes | simulation step only (000655 -> 000635 -> 000762 -> 000716) |
| 000718 | 0x15fe0 | 285 | discovered | Sleep-group union-by-rank. thiscall(body a; body b), `ret 4`. Roots via 000713 (+0x1e8), rank +0x1ec. Splices lists via tail +0x200 / next +0x1fc. Sums the ints +0x1f0 and +0x1f4 and the float +0x1f8 (fld/fadd/fstp). Caller 000724. Callees 000713. | island | none | missing | - | none | - | yes | simulation step only (000655 -> 000608 -> 000724 -> 000718) |
| 000720 | 0x16100 | 46 | discovered | thiscall(body), `ret`. Root r via 000712 (inlined first test, writes the compressed root). If byte r+0x1e4 & 2, calls 004172(r) as cdecl (push/pop ecx). Caller 000635 (loop over Scene+0x58c..0x590, body = [e+8] or [e+0xc]). Callees 000712, 004172. | island | none | missing | - | none | - | yes | simulation step only (000655 -> 000635 -> 000720) |
| 000722 | 0x16130 | 127 | discovered | Seeds the sleep-group forest from the joint-island forest. thiscall(body), `ret`. Root r via 000712. If r==body: walks the +0x1d0 chain and stores max(0, max +0x4c) (x87, `fcom; jp`) at r+0x1cc; otherwise r+0x1cc=0x4b7afafa. Then `rep movsd` copies the 7 dwords +0x1bc..+0x1d4 to +0x1e8..+0x200, and zeroes +0x25c and +0x208. Callers 000764 (tail jmp), 000776, 000797 (body ctor @0x1001b702, after 000760). Callees 000712. | island | none. Scene.cpp:2033-2034 only stores +0x1bc=+0x1e8=record in the body build; not a candidate. | missing | - | BFlag 3, CMass 30, DSet 12, Dyn 6, DynF 3, Force 33, JAll 6, JSP 24, JSlot 81, Life 18, Mom 9 | - | no | NxActor body creation: NpActor table slot 18 (000122) -> 000026 -> 000797 -> 000722; also the step via 000764/000776 |
| 000724 | 0x161b0 | 175 | discovered | Adds a contact pair to the island graph. thiscall(body a; body b or null, pair), `ret 8`. ++a+0x25c and ++b+0x25c. The root of a (000713) gets ++root+0x1f0. With b: fixes b's root. The pair goes onto the +0x208 list (pair+0x100 = old head) of whichever body has the smaller id (+0x11c, `jb` chooses b). Then a->000718(b). Without b: pushes onto a+0x208. Caller 000608 (Scene+0x668/0x66c pairs whose +0x104 == Scene+0x540 and +0x24!=0). Callees 000713, 000718. | island | none | missing | - | none | - | yes | simulation step only (000655 -> 000608 -> 000724) |
| 000726 | 0x16260 | 1371 | discovered | Per-body velocity integration. thiscall(body; dt, invDt?), `ret 8`. **Kinematic** (+0x10c bit7): from the target record [+0x118] (flags +0xc: bit0 position, bit1 orientation) it sets v=(target-com(+0x158))*scale. It also sets w from the quaternion delta target(+0x10..0x1c) x conj(current +0x124..0x130): hemisphere flip, normalise, then the 005697 CRT x87 intrinsic (acos-style _CI wrapper). **Dynamic**: v += (+0x88 accel + force +0xa0..0xa8 * invMass)*dt and w += (+0x94.. + +0xac..)*dt. Then damping +0xb8/+0xbc and the max-angular-velocity² clamp +0xd8. Reads +0x25c and Scene+0x554. Caller 000610 (islands Scene+0x57c..0x580, bodies via +0x1fc; args Scene+0x548/+0x54c). Callees 005697. | body-actor math | none | missing | - | none | - | yes | simulation step only (000655 -> 000610 -> 000726) |
| 000728 | 0x167c0 | 108 | discovered | Per-body constraint prep. thiscall(body; a1, a2), `ret 8`. For each joint J on the +0x1d8 list (link J+0x34): J+0x160=-1, J+0x164=0, then J->004133(a1) called directly. Then scene=[[body+0x19c]+4]; for each pair P on the +0x208 list (link P+0x100): 000897(ecx=P+0x14; scene, a1, a2). Caller 000730. Callees 004133 (Joint.cpp:396 Joint::row_slot6), 000897. | island | none (Joint.cpp:395 and tests only mention it as 004133's caller) | missing | - | none | - | yes | simulation step only (000655 -> 000611 -> 000730 -> 000728) |
| 000730 | 0x16830 | 42 | discovered | thiscall(body or null; a1, a2), `ret 8`. Walks the sleep-group chain from this via +0x1fc, calling 000728(a1,a2) on each. Caller 000611 @0x10011344 (ecx=[island], args Scene+0x548/+0x54c). Callees 000728. | island | none. The ObjectModel.cpp:1057 hit is a misnomer: it is the 0x5b730 write guard, not this row. | missing | - | none | - | yes | simulation step only (000655 -> 000611 -> 000730) |
| 000732 | 0x16860 | 399 | discovered | Per-body post-step. thiscall(body; dt, a2), `ret 8`. Calls 000714(dt). If +0x4c==0: zeroes +0x34..0x3c (linear only) and +0x1a0..0x1b4. Else, unless (+0x1e4&0x20 and +0x10c bit7 clear), saves +0x34..0x48 into +0x1a0..0x1b4. Clears +0x1e4 bit 0x20. If +0x10c & 0x7e: applies NX_BF_FROZEN_* per axis (bits 1..6 zero v.x,v.y,v.z,w.x,w.y,w.z and the matching saved copy) and writes back. Finally a kinematic body (bit7) zeroes +0x34..0x48. Caller 000636 (loop Scene+0x56c..0x570, args Scene+0x548/+0x54c). Callees 000714. | body-actor math | none | missing | - | none | - | yes | simulation step only (000655 -> 000636 -> 000732) |
| 000734 | 0x169f0 | 101 | discovered | thiscall(record, float dt), ret 4: world CM position += dt * linear velocity: +0x158/+0x15c/+0x160 += dt*[+0x1a0/+0x1a4/+0x1a8]; x product stays in a register, y/z products spilled to float, z sum spilled and stored by `mov`. 000770, 000772 -> none; no strings | body-actor math | none (no product code touches record +0x1a0..+0x1a8 with +0x158) | missing | - | none | - | yes | simulation step only (000770 <- 000615 <- 000655; 000772 <- 000774 <- 002264) |
| 000736 | 0x16a60 | 406 | discovered | thiscall(record, float* q, float dt), ret 8: integrates quaternion q (called with record+0x124) by angular velocity +0x1ac..+0x1b4: \|w\|=fsqrt; if \|w\|!=0: a=dt*\|w\|*0.5, k=fsin(a)/\|w\|, c=fcos(a), q=(k w, c)*q, then normalises (fsqrt, 1/len) and returns 1; \|w\|==0 returns 0; zero-length result returns 1 unnormalised. 000770, 000772 -> none | body-actor math | none (no fsin/fcos quaternion integrator in NpActor/Scene/ObjectModel) | missing | - | none | - | yes | simulation step only (as 000734) |
| 000738 | 0x16c00 | 21 | reconstructed | fastcall(record): returns record+0x244 if [+0x1e4]&0x200 else 0 (the swept-bounds box 000740 writes). 001303 -> none | CCD (broadphase / CCD swept bounds) | none: no product code reads +0x244 (only Scene.cpp:420 zeroes it); inventory `source` is a description ("flag-to-pointer (0x16c00)"), dynamic proof was a harness fixture (evidence/phase5-object-model.md 3z106) | missing | - | none | - | yes | simulation step only (001303 <- 001949 <- 001976 <- 000608 <- 000655) |
| 000740 | 0x16c20 | 423 | discovered | thiscall(record, float dt), ret 4: sets +0x1e4\|=0x200; if owner [[+0x19c]+0x10] null clears the bit and returns; else v=dt*[+0x1a0..+0x1a8] (floats), calls owner-object vtbl+0x28(&out) for a sphere (centre, radius), r=\|c-[+0x158]\|+radius, box +0x244..+0x24c = CM - r, +0x250..+0x258 = CM + r, then per axis adds v to max if v>0 else to min. 000770 -> indirect [edx+0x28] | CCD (broadphase / CCD swept bounds) | none | missing | - | none | - | yes | simulation step only (000770) |
| 000742 | 0x16dd0 | 89 | reconstructed | fastcall(record) -> st(0): 0.5*(((v74^2+v70^2)+v6c^2)*m188 + (I194*w80)*w80 + (I190*w7c)*w7c + (I18c*w78)*w78) — kinetic energy (linear vel +0x6c, angular vel +0x78, mass +0x188, inertia +0x18c). 000060 -> none | body-actor math | Physics/src/ObjectModel.cpp:1038 nxBodyRecordEnergyWord (faithful, but its only caller nxActorRecordEnergyWord ObjectModel.cpp:1121 is referenced by nothing); live public copy Physics/src/NpActor.cpp:2156 NpActorVtable::computeKineticEnergy (inline) | implemented | defect: live copy NpActor.cpp:2169-2177 sums (I0w0^2+I1w1^2)+I2w2^2 + ((v0^2+v1^2)+v2^2)*m; listing 0x10016dee-0x10016e20 is ((v2^2+v1^2)+v0^2)*m + I2w2^2 + I1w1^2 + I0w0^2 left-assoc | Mom 4 | yes: Mom 5 | no | NxActor::computeKineticEnergy (000060, guarded) -> 000742 |
| 000744 | 0x16e30 | 68 | reconstructed | fastcall(record) -> al: compress +0x1e8 chain via 000713, walk the +0x1fc list from the root; false if any node's +0x84 > 0.0 (fcomp 0x101041f0, test ah,0x41), true otherwise/null. 000062 -> 000713 | island | Physics/src/ObjectModel.cpp:3408 nxBodyRecordChainSettled (unreferenced except by the unreferenced nxActorChainSettled :3431); live inline copy Physics/src/NpActor.cpp:2329 nxNpActorGroupRoot + :2336 NpActorVtable::isGroupSleeping | implemented | defect: calling convention: the row takes the record in ecx (fastcall-style, plain ret at 0x10016e73); the standalone candidate nxBodyRecordChainSettled (ObjectModel.cpp:3408) is a cdecl free function and is uncalled; the live copy is inlined in NpActorVtable::isGroupSleeping (NpActor.cpp:2336, the 000062 row), so no function has the row's ABI. Logic walked faithful in both copies (NaN continues the walk) | DSet 12 | yes: DSet 14 | no | NxActor::isGroupSleeping (000062, guarded) -> 000744 |
| 000746 | 0x16e80 | 245 | discovered | cdecl(d, R, out), no purge: out = R diag(d) R^T; products d0R00, d0R20, d1R21, d2R22 kept in registers, the other five spilled to float (two into the arg slots); symmetric pairs stored with fst/fstp. 000140, 000142, 000144, 000768, 000770, 000772, 000789 -> none | body-actor math | Physics/src/include/NpActorDynamicMath.h:213 nxNpActorWorldTensorRDRt (inline; used only from nxNpActorUpdateMassFrame :318) | implemented | faithful (walked 0x16e8b-0x16f6e; every sum order and spill matches; cdecl, no purge, as the row). Evidence caveat: the out-of-line copies of nxNpActorWorldTensorRDRt (candidate 0x130a0 NpActor.obj, 0x2a960 Scene.obj) are never hit; the row executes only as the inlined copy inside 000768, so its dynamic evidence is 000768's breakpoint. SSE2 TU caveat as 000768 | BFlag 1, CMass 90, DSet 15, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 55, Life 6, Mom 17 | not hit | no | NxActor pose/CMass setters and NxScene::createActor via 000768; NxActor::getGlobalInertiaTensor/Inverse (000140/000142), 000144, setCMassGlobalPose/Position/Orientation (000204/206/208 -> 000789); step via 000770/000772 |
| 000748 | 0x16f80 | 47 | discovered | fastcall(record): root = compress +0x1bc via 000712; if root+0x1e0 (island object) non-null, root+0x1e4 \|= 2 (island dirty). 000793 -> 000712 | island | none (the only +0x1e4 writer is JointSupport.cpp:443/454, which is 000760) | missing | - | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | - | no | NxScene::createActor (000626 -> 000034 -> 000026 -> 000797 -> 000793 -> 000748) |
| 000750 | 0x16fb0 | 45 | discovered | thiscall(record, other), ret 4: walks this+0x1d8 joint list (link +0x34) for a joint whose +8 or +0xc is `other`; if none walks other+0x1d8 for `this` (tail in 000752); found: returns !(joint[+0x2c] bit 8); none: 0. 000905 -> none | contact-pair manager (contact/pair filtering: joint-connected pair test) | none | missing | - | none | - | yes | simulation step only (000905 <- 000909 <- 001976 <- 000608 <- 000655) |
| 000752 | 0x16fe0 | 36 | discovered | continuation of 000750 (entered by `jmp` at 0x10016fdb): second list loop, `xor eax,eax; ret 4` / flag-bit result `ret 4`. no callers | contact-pair manager (as 000750) | none | missing | - | none | - | yes | no callers (continuation of 000750) |
| 000754 | 0x17010 | 1027 | reconstructed | thiscall(record): R = C M^T (C +0x134, M +0xdc), position +0x18 = c(+0x158) - R m(+0x100), quaternion +0x24 from R (unnormalised, NxQuat-from-matrix with float spills). 000022 -> none | body-actor math | Physics/src/core/JointSupport.cpp:281 Row000754Fixture::row000754 | implemented | faithful (walked all 9 R sums, position rounding, trace arm and all three diagonal arms incl. the arm-2 float s at 0x10017305 and arm-0 float spill [esp] at 0x100173a4; roots through X87Sqrt.h) | JSlot 14 | yes: JSlot 14 | no | NxScene::createJoint (000665 -> 004300 -> 004298 -> 000022 -> 000754) and joint projection in the step (004207/004356, 000615, 000774) |
| 000756 | 0x17420 | 525 | discovered | fastcall(record): quaternion +0x124 from the +0x134 3x3 (same sequence as 000768's inline copy / 000801): yz = m11+m22 spilled to float, trace = yz + m00, trace/x/y/z arms. 000204, 000208, 000772 -> none | body-actor math | Physics/src/include/NpActorDynamicMath.h:323 nxNpActorUpdateCMassQuaternion -> :59 nxNpActorQuaternionFromMatrix (called from NpActor.cpp:1427 setCMassGlobalPose, :1593 setCMassGlobalOrientation) | implemented | defect: trace grouped (m00+m11)+m22, listing 0x10017421-0x10017430 is (m11+m22)+m00; x arm uses unrounded m11+m22, listing 0x100175ce subtracts the float spill made at 0x1001742d; CRT sqrt() where the oracle uses fsqrt (rule: X87Sqrt.h helpers) at 0x10017449/0x10017504/0x10017572/0x100175d7 (NpActorDynamicMath.h:64/77) | CMass 2 | not hit | no | NxActor::setCMassGlobalPose / setCMassGlobalOrientation (000204/000208); step via 000772 |
| 000758 | 0x17630 | 214 | reconstructed | fastcall(record): +0x134 3x3 from +0x124 quaternion (doubled products, spills 2yy, 2xz, 2yw, 2yz, 1-2xx). 000770, 000772, 004356 -> none | body-actor math | Physics/src/core/JointSupport.cpp:373 Row000758Fixture::row000758 | implemented | faithful (walked 0x17633-0x176fc) | JSlot 4 | yes: JSlot 4 | no | joint projection 004356 (RevoluteJoint.cpp:1446); step via 000770/000772 |
| 000760 | 0x17710 | 168 | reconstructed | thiscall(record): if own island root, frees island object +0x1e0 (004167 then allocator vtbl+0x14); resets island fields +0x1bc=this, +0x1c0/+0x1c4/+0x1d0/+0x1d8/+0x1dc=0, +0x1c8=1, +0x1cc=0x4b7afafa, +0x1d4=this, +0x1e4&=~2; unless [+0x114]&0x100 raises +0x4c to 0.39999998 when ordered-below. 000604, 000632, 000776, 000778, 000797 -> 004167 | island | Physics/src/core/JointSupport.cpp:431 Row000760Fixture::row000760 | implemented | faithful (walked; `test ah,5; jp` = ordered-below matches `wake < floor`; allocator +0x14 is NxUserAllocator::free) | BFlag 4, CMass 40, DSet 16, Dyn 8, DynF 4, Force 44, JAll 10, JSP 157, JSlot 122, Life 24, Mom 12 | yes: JAll 2, JSP 125, JSlot 14 | no | NxScene::createActor (000026 -> 000797 -> 000760), actor/joint release (000776, 000778), Scene teardown 000604 |
| 000762 | 0x177c0 | 357 | discovered | thiscall(record, joint), ret 4: link a joint into the island graph: wakes body0 (+0x4c floor unless +0x114 bit 8), compresses +0x1bc, frees root's island object, root+0x1c4++; with a second body (joint+0xc) wakes/compresses/frees it too, pushes the joint onto the lower-id (+0x11c) body's +0x1d8 list (link +0x34) and the other's +0x1dc list (link +0x38), then 000716 merge; single body: push on +0x1d8, root+0x1e4\|=2. 000635 -> 000712, 000716, 004167 | island | none | missing | - | none | - | yes | simulation step only (000635 <- 000655) |
| 000764 | 0x17930 | 100 | discovered | fastcall(record): compress +0x1bc; if root+0x1e4 bit 1: free island object (004167 + allocator free), 004172(root) cdecl; tail-jumps 000722(record). 000635 -> 000712, 000722, 004167, 004172 | island | none | missing | - | none | - | yes | simulation step only (000635 <- 000655) |
| 000766 | 0x179a0 | 1377 | discovered | thiscall(record, NxDebugRenderable*), ret 4: body debug visualisation gated on +0x10c bit 8 and params at .data 0x10123b54/58/5c/60/80 (times scale 0x10123b4c): mass-frame axes (3 lines, 0xff0000/0xff00/0xff), inertia box (fsqrt of I sums, OBB via vtbl+0x28, colour from +0x4c), velocity arrows (+0x34.., +0x1a0.., +0x40.., +0x1ac.., vtbl+0x30), then 004163 on the island object +0x1e0. 000020 -> 004163 | visualisation (debug visualisation) | none (NpScene.cpp:391 NpScene::visualize is `// (unimplemented)`) | missing | - | none | - | no | NxScene::visualize (000344 -> 000657 -> 000020 -> 000766) |
| 000768 | 0x17f10 | 1164 | discovered | thiscall(record): R from quaternion +0x24; +0x134 = R*F(+0xdc); +0x158 = R p(+0x100) + t(+0x18); +0x124 = quaternion of +0x134; then 000746(+0xc4, +0x134, +0x164). 000164, 000196..000222 setters, 000793, 000795 (0x1b497, missing from the bundle caller list) -> 000746 | body-actor math | Physics/src/include/NpActorDynamicMath.h:281 nxNpActorUpdateMassFrame (+ :242 nxNpActorComposeRotation, :98 nxNpActorBodyQuaternionFromMatrix, :213 nxNpActorWorldTensorRDRt); called from NpActor.cpp:1238 nxNpActorRefreshCMass and Scene.cpp:2106 | implemented | defect: CRT sqrt() where the oracle uses fsqrt (rule: X87Sqrt.h helpers) in the inline quaternion conversion (nxNpActorBodyQuaternionFromMatrix, NpActorDynamicMath.h:98) at 0x10018216/0x100182a8/0x100182f3/0x10018339; calling convention: the row is thiscall (ecx = record, plain ret at 0x1001838f), the candidate is a cdecl free function nxNpActorUpdateMassFrame(unsigned char*) (emitted twice, NpActor.obj 0x12820 and Scene.obj 0x2a220); the rest of the walk (0x17f18-0x1838a: rotation spills, centre x wide / y,z rounded, nine R*F sums, quaternion arm selection and spills, call args to 000746) matches; also compiled SSE2 (NpActor.cpp/Scene.cpp are not /arch:IA32) | BFlag 1, CMass 87, DSet 15, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 55, Life 6, Mom 3 | yes: BFlag 1, CMass 87, DSet 15, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 55, Life 6, Mom 3 | no | NxActor::setGlobalPose/Position/Orientation(/Quat), setCMassOffset* (000196..000222), NxScene::createActor (000795 0x1b497, 000793) |
| 000770 | 0x183a0 | 205 | discovered | Body::saveCcdPose(dt), thiscall ret 8: +0x23c=FLT_MAX, +0x240=0, copy +0x134(9)/+0x158(3) to +0x20c..+0x238; if wake +0x4c!=0: 000734(dt), 000736(+0x124,dt), 000758, 000746(+0xc4,+0x134,+0x164), then SDK param 0xb (NX_CONTINUOUS_CD, 000429 on [0x10123c04]) !=0 -> 000740(dt) and return; else +0x1e4 &= ~0x200. 000615 -> 000429, 000734, 000736, 000740, 000746, 000758 | body-actor math | none (no product step; grep of 0x23c/0x20c/0x240 on the record finds nothing) | missing | n/a | none | - | yes | simulation step only (NxScene::simulate -> 002400 -> 000659 -> 000655 -> 000615 -> row) |
| 000772 | 0x18470 | 207 | discovered | Body::restoreCcdPose(dt) -> bool, ret 4: only if dt < +0x23c (ordered): +0x23c=dt, +++0x240, restore +0x158 from +0x230, +0x134 from +0x20c, 000756, dt2=dt*[[+0x19c]+4]+0x548, 000734(dt2), 000736(+0x124,dt2), 000758, 000746; returns 1, else 0. 000774 -> 000734, 000736, 000746, 000756, 000758 | CCD (CCD) | none | missing | n/a | none | - | no | unreachable as shipped (only via 002264 CCD sweep, gated by NX_CONTINUOUS_CD=0 default; ContactGeneration.h:167-180) |
| 000774 | 0x18540 | 34 | discovered | if 000772(dt) then 000022([+0x19c], 0) (notify owning body); ret 4. 002264 -> 000772, 000022 | CCD (CCD) | none | missing | n/a | none | - | no | unreachable (002264 only) |
| 000776 | 0x18570 | 117 | discovered | Body::~Body (non-deleting): vtable=0x10106890; 000028(scene+0x6f8, id +0x11c) recycles record id; if +0x1e8!=this, +0x1e8=000713(); walk +0x1e8 list via +0x1fc calling 000722; 000760(this); 000722(this); import dtor [0x10104194] (Observable); tail-jmp 000799(this+0x18). 000030, 000122, 000602 -> 000028, 000713, 000722, 000760, 000799 | island | Scene.cpp:1332 NxSceneInternal::releaseActor, record part 1349-1375 (nxSceneAuxUnregisterRecord, nxSceneRecycleRecordId, free) | partial | defect: island teardown 0x18594-0x185d4 (000713/000722/000760 walk) and Observable dtor 0x185d6 absent; order differs (oracle recycles id first 0x1858f, aux-unregister last via 000799) | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | yes: BFlag 1, CMass 11, DSet 8, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 5, Meta 1, Mom 4, Name 1, ShMut 1 | no | NxScene::releaseActor (000295 -> 000628 -> 000030 -> row); NxActor::setDynamic slot 18 (000122); Scene dtor (000663 -> 000602/000596) |
| 000778 | 0x185f0 | 58 | reconstructed | Body::dissolveIsland(joint, jointArray) head: root refresh via 000712 on parent, loop entry (continues in 000780). 000632, 000633 -> 000712, 000760 | island | core/JointSupport.cpp:471 Row000778Fixture::row000778 (lines 471-480) | implemented | faithful | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 4, JSP 66, JSlot 41, Life 4, Mom 3 | yes: JAll 2, JSP 58, JSlot 14 | no | NxScene::releaseJoint (000299 -> 000653 -> 000633 -> row); NxScene::releaseActor (000628 -> 000030 -> 000632 -> row) |
| 000780 | 0x18630 | 243 | reconstructed | continuation of 000778: per island body, push every +0x1d8 joint != `joint` onto array (inlined 000661 push 0x1864c-0x186e6), clear link +0x34, read +0x1d0 then 000760; ret 8 | island | core/JointSupport.cpp:477-491 (loop of row000778) + include/core/JointSupport.h:211 nxJointPointerArrayPush | implemented | faithful | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 4, JSP 133, JSlot 41, Life 6, Mom 3 | not hit | no | via 000778 (fall-through, no own callers) |
| 000782 | 0x18730 | 3428 | discovered | Body::addForce(force*, torque*, mode, wake) thiscall ret 0x10; switch table 0x10019494 {0x1874d,0x189fd,0x18c87,0x18ec3,0x19138}: 0 FORCE +0x88/+0x94 mark 0x20/0x40; 1 IMPULSE +0x6c/+0x78 (+0x34/+0x40 copies) mark 4/8; 2 VELOCITY_CHANGE same targets unscaled; 3 SMOOTH_IMPULSE +0xa0/+0xac mark 0x80/0x100; 4 SMOOTH_VELOCITY_CHANGE unscaled; torque scaled by world inverse tensor +0x164; then if wake && !(+0x114&0x100) && +0x84<0.4f: +0x84=+0x4c=0.4f, mark 0x10. 000056, 000058, 000160, 000162, 000791, 003601 -> none | body-actor math | NpActor.cpp:2037 nxNpActorAccumulateForce (called once per vector) | partial | defect (see notes: x87 grouping in modes 0/1/3 at 0x18759, 0x1888b, 0x18b44, 0x18ecf, 0x1900b; wake arg ignored 0x1936f; mode>4 skips wake 0x18740); allocator: the dirty-mark vector growth goes through nxGetSdkAllocator() (NpActor.cpp:109/113 nxNpActorMarkRecordDirty; Scene.cpp record registration) where the oracle uses the Foundation allocator [0x101041bc] (listing loads of 0x101041bc in this row) | Force 26 | yes: Force 32 | no | NxActor::addForce/addLocalForce/addTorque/addLocalTorque (slots 58-61) -> row; addForceAt* via 000791 |
| 000784 | 0x194b0 | 368 | discovered | Body::setKinematicTarget(pos*, quat*), ret 8: if pos: [+0x118]+0..8=pos, [+0x118]+0xc |= 1; if quat: [+0x118]+0x10..0x1c=quat, +0xc |= 2; then wake (+0x114 bit 0x100 clear and +0x84 < [0x101053d4]=0.4f ordered) +0x84=+0x4c=0x3ecccccc, mark 0x10. 000090, 000124, 000126 -> none | body-actor math | inlined in NpActor.cpp:1128 moveGlobalPose / 1147 moveGlobalPosition (1164 moveGlobalOrientation routes via moveGlobalPose) | partial | defect: wake block 0x1950a-0x1961b missing; moveGlobalPose stores +0xc=3 instead of OR 1 (0x194d5) / OR 2 (0x19506); orientation-only path also rewrites the target position; allocator: the dirty-mark vector growth goes through nxGetSdkAllocator() (NpActor.cpp:109/113 nxNpActorMarkRecordDirty; Scene.cpp record registration) where the oracle uses the Foundation allocator [0x101041bc] (listing loads of 0x101041bc in this row) | Dyn 5 | yes: Dyn 8 | no | NxActor::moveGlobalPose/Position/Orientation (slots 10-12) -> row |
| 000785 | 0x19620 | 1325 | discovered | Body::setKinematic(bool) ret 4 (with continuation 000787). Enable (no-op if +0x10c bit 0x80): island root via inlined 000712 on +0x1bc, if root+0x1e0: root+0x1e4 |= 2; +0xc0=0 mark 0x10000; +0xc4..cc=0 mark 0x20000; +0x10c|=0x80 mark 0x80000; alloc 0x20 at +0x118 if null, [+0x118]+0xc=0. Disable: same island step; +0x10c&=~0x80 mark 0x80000; +0xc0=1/+0x188 mark 0x10000; (000787) 1/+0x18c..194 -> +0xc4..cc mark 0x20000; free +0x118. 000188, 000190, 000793 -> 000712 | body-actor math | NpActor.cpp:130 nxNpActorTransitionKinematic | partial | defect: island wake 0x19643-0x1966f / 0x19992-0x199be missing; 1/mass guarded (mass>0?1/m:0) vs unconditional fdiv 0x19abb; creation path never calls it (0x1b4a8); allocator: the 0x20-byte kinematic block is malloc'd/freed through nxGetSdkAllocator() (NpActor.cpp:141/158) where the oracle uses [0x101041bc] (0x100196d4, 0x10019719, 0x100197d1, 0x10019819, 0x100198d7, 0x10019911, 0x1001995d, 0x10019a34) | BFlag 1, CMass 10, DSet 6, Dyn 10, DynF 1, Force 13, JAll 2, JSP 8, JSlot 27, Life 6, Mom 5 | yes: DSet 2, Dyn 8, Force 2, Mom 2 | no | NxActor::raiseBodyFlag/clearBodyFlag (slots 78/79) -> row; NxScene::createActor -> ... 000793 -> row |
| 000787 | 0x19b50 | 428 | discovered | tail of 000785 (reached by 0x19b4b jmp 0x19b50 and 0x1963d/0x1998c js/jns -> 0x19cf2 epilogue): disable-path loop tail, +0xc4..cc = 1/+0x18c, 1/+0x190, 1/+0x194 (z spilled 0x19bd4), mark 0x20000, free/null +0x118; ret 4. No own callers, not in any dispatch table | body-actor math | NpActor.cpp:146-160 (else branch of nxNpActorTransitionKinematic) | partial | defect: inverse inertia guarded by >0 (0x19bb6-0x19bce is unconditional); allocator: free through nxGetSdkAllocator() (NpActor.cpp:158) where the oracle frees through [0x101041bc] (0x10019b69, 0x10019c54, 0x10019c8e, 0x10019cda) | none | no own address | no | via 000785 |
| 000789 | 0x19d00 | 1461 | discovered | Body::setPoseFromCMass() thiscall no args: 000746(+0xc4,+0x134,+0x164); M = C(+0x134)*F(+0xdc)^T; t = com(+0x158) - M*p(+0x100) -> +0x50 and +0x18, mark 1; quat(M) -> +0x5c and +0x24, mark 2. 000204, 000206, 000208 -> 000746 | body-actor math | NpActor.cpp:1484 nxNpActorApplyWorldMassPose | implemented | defect: tensor helper 0x19d1e; displacement grouping 0x19e5e-0x19ec8; quaternion trace/x/z arms 0x19fe1, 0x1a093, 0x1a148 (see notes); CRT sqrt() where the oracle uses fsqrt (rule: X87Sqrt.h helpers) at 0x1001a010/0x1001a0a5/0x1001a0fe/0x1001a156 (NpActor.cpp:1521 and NpActorDynamicMath.h:64/77); allocator: the inline dirty-mark growth uses nxGetSdkAllocator() (NpActor.cpp:109/113, nxNpActorMarkRecordDirty) where the oracle uses [0x101041bc] | CMass 3 | yes: CMass 3 | no | NxActor::setCMassGlobalPose/Position/Orientation (slots 26-28) -> row |
| 000791 | 0x1a2c0 | 133 | discovered | Body::addForceAtPos(force*, pos*, mode, wake) ret 0x10: d = pos - com(+0x158) (dx stays in st, dy/dz spilled), torque = d x force, then 000782(force, &torque, mode, wake). 000054, 000154, 000156, 000158, 003979 -> 000782 | body-actor math | NpActor.cpp:1962 nxNpActorForceAtPos | implemented | defect: lever.x rounded to float (0x1a2c7-0x1a2c9 keeps dx in the register); torque/force accumulated by two calls (effect-equivalent) | Force 6 | yes: Force 6 | no | NxActor::addForceAtPos, addForceAtLocalPos, addLocalForceAtPos, addLocalForceAtLocalPos (slots 54-57) -> row |
| 000793 | 0x1a350 | 1613 | discovered | Body::loadFromBodyDesc(bodyDesc) head (continues in 000795): +0x188=mass, +0xc0=1/mass (unconditional) mark 0x10000; +0xb8=linearDamping mark 0x800; +0xbc=angularDamping mark 0x1000; +0x100=massLocalPose.t mark 0x200 +++0x198; +0xdc=massLocalPose.M mark 0x400 +++0x198; if any inertia word nonzero: +0x18c=inertia, inverses via _fpclass (0xf4140, mask 0x207) -> +0xc4..cc (else zeros). 000797 -> 000748, 000768, 000785, 005666 | body-actor math | Scene.cpp:1985 nxActorComputeMass (lines 2039-2100) | partial | defect (see notes); allocator: the dirty-mark vector growth goes through nxGetSdkAllocator() (NpActor.cpp:109/113 nxNpActorMarkRecordDirty; Scene.cpp record registration) where the oracle uses the Foundation allocator [0x101041bc] (listing loads of 0x101041bc in this row) | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | yes: BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 6, JSlot 27, Life 5, Mom 3 | no | NxScene::createActor (000293 -> 000626 -> 000034 -> 000026 -> 000797 -> row); NxActor::setDynamic (000122 -> 000026) |
| 000795 | 0x1a9a0 | 3090 | discovered | tail of 000793 (0x1a99b jmp 0x1a9a0, 0x1a84d je 0x1aa06; no own callers, not a vtable slot): zero-inertia branch inertia=(1,1,1), inverses 1.0 (0x1aa06); mark 0x20000; linVel -> +0x6c,+0x34 mark 4; angVel -> +0x78,+0x40 mark 8; wakeUpCounter -> +0x84,+0x4c mark 0x10; solverIterationCount -> +0x110 mark 0x40000; +0xd8 = maxAngVel>0 ? v*v : g*g ([0x10123b34]) mark 0x8000; +0xd0 = sleepLinVel>0 ? v*v : [0x10123b20] mark 0x2000; +0xd4 = sleepAngVel>0 ? v*v : [0x10123b24] mark 0x4000; 000768; 000785((flags>>7)&1); +0x10c=flags mark 0x80000; 000748; ret 4 | body-actor math | Scene.cpp:2039-2107 nxActorComputeMass | partial | defect (see notes); allocator: the dirty-mark vector growth goes through nxGetSdkAllocator() (NpActor.cpp:109/113 nxNpActorMarkRecordDirty; Scene.cpp record registration) where the oracle uses the Foundation allocator [0x101041bc] (listing loads of 0x101041bc in this row) | none | no own address | no | via 000793 |
| 000797 | 0x1b5c0 | 402 | discovered | Body::Body(owner body, pose*, bodyDesc*) thiscall ret 0xc: take id from scene+0x6f8 pool; 000801(this+0x18, scene+0x48 aux, pose, id); Observable ctor import [0x10104190]; vtable 0x10106890; zero/identity +0x134..+0x160; +0x20c identity; +0x244..24c=FLT_MAX, +0x250..258=-FLT_MAX; +0x19c=owner; +0x198,+0x1a0..+0x1b4=0; +0x1b8=1; 000760; 000722; +0x1e4=+0x1e0=+0x204=0; copy +0x134->+0x20c (9), +0x158->+0x230 (3); 000793(bodyDesc). 000026 -> 000722, 000760, 000793, 000801 | body-actor math | Scene.cpp:1985 nxActorComputeMass (lines 1994-2037) + Scene.cpp:623 nxSceneTakeRecordId | partial | defect: fields missing (see notes) | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | not hit | no | NxScene::createActor (000626 -> 000034 -> 000026 -> row); NxActor::setDynamic (000122 -> 000026) |
| 000799 | 0x1b760 | 51 | discovered | base sub-object dtor (this = rec+0x18): 002411(aux=[+0x108], this) unregisters from scene aux; free [+0x100] (rec+0x118 kinematic block) and null it. 000776 -> 002411 | body-actor math | Scene.cpp:834 nxSceneAuxUnregisterRecord (called at Scene.cpp:1351) | partial | defect: free of rec+0x118 (0x1b76f-0x1b791) absent: the 0x20-byte kinematic block leaks | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | not hit | no | via 000776 (tail jmp 0x185e0) |
| 000801 | 0x1b7a0 | 741 | discovered | base sub-object ctor (this = rec+0x18) (aux, pose*, id) ret 0xc: identity 3x3 at +0xc4 (rec+0xdc); +0x104=id (rec+0x11c), +0x108=aux (rec+0x120); pose.t -> +0 and +0x38 (rec+0x18, +0x50); quaternion from pose.M -> +0xc (rec+0x24) 0x1b82e-0x1b987, copied to +0x44 (rec+0x5c); zero +0x70..+0x118 region, identity again; 002421(aux, this) registers. 000797 -> 002421 | body-actor math | include/NpActorDynamicMath.h:98 nxNpActorBodyQuaternionFromMatrix, used at Scene.cpp:2019-2022; remaining stores at Scene.cpp:2002-2003, 2030-2032, 2108 | partial | defect: quaternion 0x1b82e-0x1b987 walked and matches except CRT sqrt() where the oracle uses fsqrt at 0x1001b84d/0x1001b8d5/0x1001b914/0x1001b951 (rule: X87Sqrt.h helpers); registration moved to the end (Scene.cpp:2108) and uses first-free slot, not id | BFlag 1, CMass 10, DSet 4, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 6, Mom 3 | yes: BFlag 2, CMass 97, DSet 19, Dyn 4, DynF 2, Force 22, JAll 4, JSP 16, JSlot 82, Life 12, Mom 6 | no | NxScene::createActor -> 000026 -> 000797 -> row |
| 000803 | 0x1ba90 | 23 | dynamically_gated | NxComputeSphereMass: r*r*r*density*[0x101068d8], result in st0, cdecl; no callers, no callees | mass properties (mass properties, exported kernel) | Physics/src/MassProperties.cpp:73 NxComputeSphereMass | implemented | faithful | none | not hit | no | DLL export NxComputeSphereMass |
| 000805 | 0x1bab0 | 23 | dynamically_gated | NxComputeSphereDensity: mass / (r^3*4pi/3) (fdivr is last) | mass properties (mass properties) | MassProperties.cpp:80 NxComputeSphereDensity | implemented | faithful | none | not hit | no | DLL export |
| 000807 | 0x1bad0 | 44 | dynamically_gated | NxComputeBoxMass: accumulator opens at [0x101041ec]; integer word tests of each extent (replace, then multiply); *density | mass properties (mass properties) | MassProperties.cpp:88 NxComputeBoxMass (+ extentProduct :59, isZeroWord :48) | implemented | faithful | none | not hit | no | DLL export |
| 000809 | 0x1bb00 | 44 | dynamically_gated | NxComputeBoxDensity: mass / extentProduct | mass properties (mass properties) | MassProperties.cpp:94 NxComputeBoxDensity | implemented | faithful | none | not hit | no | DLL export |
| 000811 | 0x1bb30 | 50 | dynamically_gated | NxComputeEllipsoidMass: product*density*4pi/3 | mass properties (mass properties) | MassProperties.cpp:100 NxComputeEllipsoidMass | implemented | faithful | none | not hit | no | DLL export |
| 000813 | 0x1bb70 | 50 | dynamically_gated | NxComputeEllipsoidDensity: mass/(product*4pi/3) | mass properties (mass properties) | MassProperties.cpp:106 NxComputeEllipsoidDensity | implemented | faithful | none | not hit | no | DLL export |
| 000815 | 0x1bbb0 | 25 | dynamically_gated | NxComputeCylinderMass: (l+l)*r*r*density*pi | mass properties (mass properties) | MassProperties.cpp:113 NxComputeCylinderMass | implemented | faithful | none | not hit | no | DLL export |
| 000817 | 0x1bbd0 | 25 | dynamically_gated | NxComputeCylinderDensity: mass/((l+l)*r*r*pi) | mass properties (mass properties) | MassProperties.cpp:122 NxComputeCylinderDensity | implemented | faithful | none | not hit | no | DLL export |
| 000819 | 0x1bbf0 | 25 | dynamically_gated | NxComputeConeMass: fabs(l)*r*r*density*pi/3 | mass properties (mass properties) | MassProperties.cpp:129 NxComputeConeMass | implemented | faithful | none | not hit | no | DLL export |
| 000821 | 0x1bc10 | 25 | dynamically_gated | NxComputeConeDensity: mass/(fabs(l)*r*r*pi/3) | mass properties (mass properties) | MassProperties.cpp:137 NxComputeConeDensity | implemented | faithful | none | not hit | no | DLL export |
| 000823 | 0x1bc30 | 75 | dynamically_gated | NxComputeBoxInertiaTensor: m/12 * pairwise sums; x*x spilled to m32 at 0x1bc5e and reloaded at 0x1bc6b for z | mass properties (mass properties) | MassProperties.cpp:144 NxComputeBoxInertiaTensor | implemented | faithful | none | not hit | no | DLL export |
| 000825 | 0x1bc80 | 62 | dynamically_gated | NxComputeSphereInertiaTensor: stores unscaled m*r*r to [out], then *2/3 (hollow) or *0.4, and copies x to y and z | mass properties (mass properties) | MassProperties.cpp:166 NxComputeSphereInertiaTensor | implemented | faithful | none | not hit | no | DLL export |
| 000827 | 0x1bcc0 | 50 | reconstructed | MassFrame fill: rep movsd 9 dwords arg1 -> [this+0..0x20], arg2[0..2] -> +0x24..0x2c, arg3 -> +0x30; thiscall ret 0xc; caller 001397 (MESH slot 4) | mass properties (mass frame) | ObjectModel.cpp:5174 BoxShape::nxPoseCopyWithTail0827; also inlined in ObjectModel.cpp:5975-5977 (MeshShape::nxMeshAccumulateMassCached) | implemented | faithful | none | not hit | no | NxActor::updateMassFromShapes / setDynamic -> 000008 -> MESH slot 4 (001397) -> row |
| 000829 | 0x1bd00 | 187 | reconstructed | unit-density box MassFrame over half-extents: mass=8*prod, F=mass*(1/3), diag=F*pairwise sums, off-diag and offset zeroed; ret 4; caller 000849 | mass properties (mass frame) | ObjectModel.cpp:4703 MassFrame::nxMassFrameBuildBox | implemented | defect: F spilled to m32 at 0x1001bd39 and the three pairwise sums spilled to m32 at 0x1001bd67/0x1001bd6c/0x1001bd72; the candidate keeps all four as double (see Notes) | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 7 | not hit | no | ... -> 000008 -> BOX slot 4 (000947) -> 000849 -> row |
| 000831 | 0x1bdc0 | 635 | reconstructed | payload fold: nine m32-rounded products A=f(I,R), then inertia overwritten, offset <- {o.d, o.K0, o.K1}; mass untouched; ret 4; callers 000835/843/849/853 | mass properties (mass frame) | ObjectModel.cpp:4799 MassFrame::nxMassFrameFoldPayload | implemented | faithful (walked every product, store and fxch) | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 7 | not hit | no | shape slot-4 mass rows (see 000849) |
| 000833 | 0x1c040 | 1371 | reconstructed | MassFrame translate by d: all-zero-word early out; c=d+o; centered path (0x1c0d7) / displaced path (0x1c26f) build -Q(c), -Q(o); diff*mass added to inertia; offset += d; ret 4 | mass properties (mass frame) | ObjectModel.cpp:4879 MassFrame::nxMassFrameTranslate | implemented | defect (minor): off-diagonal terms omit the `x*[0x101041f0]` (0.0) addends (e.g. 0x1001c108, 0x1001c2b4, 0x1001c374); the result differs only for non-finite c/o (0*inf=NaN) and in signed-zero cases. Finite values match: I walked the diagonal spills at 0x1c296/0x1c2f0/0x1c3c5 | BFlag 2, Dyn 2, DynF 2, JAll 4, JSP 16, JSlot 54, Life 13 | not hit | no | shape slot-4 rows; 000841 |
| 000835 | 0x1c5a0 | 30 | reconstructed | fold(arg) then translate(arg+0x24); ret 4; caller 001397 | mass properties (mass frame) | inlined at ObjectModel.cpp:5980-5981 (MeshShape::nxMeshAccumulateMassCached); no standalone function | implemented | faithful | none | not hit | no | ... -> 000008 -> MESH slot 4 (001397) -> row |
| 000837 | 0x1c5c0 | 101 | reconstructed | scale nine inertia words and mass (+0x30) by s; offset untouched; ret 4 | mass properties (mass frame) | ObjectModel.cpp:4620 MassFrame::nxMassFrameScale | implemented | faithful | none | not hit | no | shape slot-4 rows |
| 000839 | 0x1c630 | 231 | reconstructed | merge: sum=m+m'; q=float(1.0/sum); offset=weighted average; mass=sum; inertia accumulates; ret 4 | mass properties (mass frame) | ObjectModel.cpp:4630 MassFrame::nxMassFrameMerge | implemented | faithful (walked the spill pattern: ay,az,cx,cy,cz,sy,sz,q are m32; ax and x-sum stay in registers) | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 7 | not hit | no | shape slot-4 rows; 001397 |
| 000841 | 0x1c720 | 43 | reconstructed | negated-offset translate: {-o} on stack, call 000833; ret 0; caller 000008 | mass properties (mass frame) | none (no member builds {-offset}; the only caller 000008 is unimplemented, NpActor.cpp:1219 placeholder) | missing | - | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 6 | - | no | NxActor::updateMassFromShapes (NpActor vtable 0x10104530 slot 17 -> 000164) or slot 18 setDynamic (000122 -> 000026) -> 000008 -> row |
| 000843 | 0x1c750 | 110 | reconstructed | unit-density sphere MassFrame: mass=r^3*4pi/3, diag=((r*mass)*r)*0.4, rest integer-zeroed, optional fold+translate; ret 8 | mass properties (mass frame) | ObjectModel.cpp:4579 MassFrame::nxMassFrameBuildSphere | implemented | faithful | none | not hit | no | ... -> SPHERE slot 4 (001371) -> 000851 -> row |
| 000845 | 0x1c7c0 | 181 | reconstructed | unit-density cylinder MassFrame: m=(2c)*r*r*pi; axial=m*r*r*0.5, side=(3r^2+4c^2)*m/12; selector routes the diagonals; ret 0xc | mass properties (mass frame) | ObjectModel.cpp:4741 MassFrame::nxMassFrameBuildCapsule | implemented | defect: selector==1 writes side to +0x00 at 0x1001c836 (before the `je` at 0x1c838); the candidate leaves +0x00 unwritten (see Notes) | none | not hit | no | ... -> CAPSULE slot 4 (001008, selector 1) -> 000853 -> row |
| 000847 | 0x1c880 | 53 | reconstructed | if (byte arg != 0) integer-zero all 13 words; ret 4; caller 000008 | mass properties (mass frame) | ObjectModel.cpp:4868 MassFrame::nxMassFrameConditionalZero | implemented | defect (minor): the oracle tests only the low byte (`mov dl,[esp+4]; cmp dl,cl` at 0x1001c880/0x1c888); the candidate takes `unsigned flag` and tests all 32 bits. No product caller yet | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 6 | not hit | no | updateMassFromShapes/setDynamic -> 000008 -> row |
| 000849 | 0x1c8c0 | 101 | reconstructed | BOX slot-4 helper: build box (000829), when extra!=0 fold (000831) + translate(extra+0x24) (000833), scale unless density==1.0 (fucompp/jnp), merge into this; ret 0xc | mass properties (mass frame) | ObjectModel.cpp:5206 BoxShape::nxBoxComputeMassFrame | implemented | defect: the call to 0x1c040 at 0x1001c8eb is replaced by an inline "centered" formula (see Notes); calling convention: the row is thiscall on the destination MassFrame (ecx = dest, 3 stack args, ret 0xc at 0x1001c922); the candidate is a member of BoxShape with the destination as an extra first argument, so ecx is the shape, not the MassFrame (record like 000713) | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 7 | not hit | no | ... -> 000008 -> BOX slot 4 (000947) -> row |
| 000851 | 0x1c930 | 77 | reconstructed | SPHERE slot-4 helper: build sphere(radius=a2, extra=a3), scale unless density==1.0, merge; ret 0xc | mass properties (mass frame) | ObjectModel.cpp:4665 SphereShape::nxSphereComputeMassFrame | implemented | defect: calling convention: the row is thiscall on the destination MassFrame (ecx = dest, 3 stack args, ret 0xc); the candidate is a member of SphereShape with the destination as an extra first argument, so ecx is the shape, not the MassFrame (record like 000713); the body (build sphere, scale unless density == 1.0, merge) walks faithfully | none | not hit | no | ... -> SPHERE slot 4 (001371) -> row |
| 000853 | 0x1c980 | 115 | reconstructed | CAPSULE slot-4 helper: build cylinder(sel=a2,r=a3,c=a4), fold+translate(extra=a5), scale unless 1.0, merge; ret 0x14 | mass properties (mass frame) | ObjectModel.cpp:5240 CapsuleShape::nxCapsuleComputeMassFrame | implemented | defect: calling convention: the row is thiscall on the destination MassFrame (ecx = dest, 5 stack args, ret 0x14); the candidate is a member of CapsuleShape with the destination as an extra first argument, so ecx is the shape, not the MassFrame (record like 000713); its own body walks faithfully, but the result is wrong through the 000845 defect | none | not hit | no | ... -> CAPSULE slot 4 (001008) -> row |
| 000855 | 0x1ca00 | 75 | discovered | NxCombineMode combine(a,b,mode): 0 average, 1 min, 2 multiply, else max; stdcall ret 0xc, x87 return; caller 000861 | contact-pair manager (contact material combine) | none | missing | - | none | - | yes | simulation step (see 000897) |
| 000857 | 0x1ca50 | 209 | discovered | contact-pair restitution: material ids lo16/hi16 of arg1 index SDK [0x10123c04]+0x28 (stride 0x48, clamped to 0); restitution +0x10 combined with max(restitutionCombineMode +0x40); out[1]=(flagsA\|flagsB)&4 (NX_MF_SPRING_CONTACT); stdcall ret 8; caller 000897 | contact-pair manager (contact material combine) | none | missing | - | none | - | yes | simulation step |
| 000859 | 0x1cb30 | 607 | discovered | anisotropic friction: world anisotropy dir = shape rot(+0xc..0x2c)*mat.dirOfAnisotropy (+0x1c..0x24) -> out+0x14; dynamic/dynamicV friction combined with frictionCombineMode (+0x3c), *NX_DYN_FRICT_SCALING [0x10123b2c], clamped to [0,1] -> out+0xc/+0x10; static/staticV *NX_STA_FRICT_SCALING [0x10123b30], max with dynamic, *normalForce -> out+8/+4; stdcall ret 0x14; caller 000861 | contact-pair manager (contact friction) | none | missing | - | none | - | yes | simulation step |
| 000861 | 0x1cd90 | 791 | discovered | friction-patch parameters: spring flag, isotropic path via 000855 + scaling/clamp, or anisotropic path via 000859; tangents from NxNormalToTangents ([0x1010418c]) or dir x n normalized (fallback when \|t\|<=0.01 averages the two coefficients); enable byte at out+0x20; stdcall ret 0x1c; caller 000879 | contact-pair manager (contact friction) | none | missing | - | none | - | yes | simulation step |
| 000863 | 0x1d0b0 | 8 | discovered | thunk: ecx+=0x10, tail to 002354 (reset the pair's contact-stream sub-object); caller 000905 | contact-pair manager (contact pair management) | none | missing | - | none | - | yes | simulation step (001976 -> 000909 -> 000905) |
| 000865 | 0x1d0c0 | 409 | discovered | store the friction anchors in each body's local frame: count byte ->patch+0x74; for up to N points: (p - body+0x158) * body rot (+0x134..0x154) into patch+4.. (body0) / +0x10.. (body1), stride 0x18; clear patch+0x75; thiscall ret 0xc; caller 000897 | contact-pair manager (contact friction anchors) | none | missing | - | none | - | yes | simulation step |
| 000867 | 0x1d260 | 103 | reconstructed | v=a/b; unless kind([desc+0xc]&0x1f) is 4/5: [this+0x64]+=v; else [this+0x58..0x60]+=v*desc[0..2], bit 6 -> byte [this+0x75]=1; ret 0xc; caller 000893 | contact-pair manager (contact pair accumulator) | ObjectModel.cpp:2685 nxAccumulateByKind0867 | implemented | defect: v stays unrounded in st0 (0x1001d26b); the candidate rounds it to float (`const float v`). v*desc[2] is spilled to m32 at 0x1001d299 before its add; the candidate does not round that product (see Notes) | none | not hit | yes | simulation step (000911 -> 000901 -> 000893) |
| 000869 | 0x1d2d0 | 699 | discovered | contact visualization: walks the pair's contact stream (+0x40); per contact, NX_VISUALIZE_CONTACT_FORCE/NORMAL/ERROR ([0x10123bb8]/[0x10123bb0]/[0x10123bb4]) * NX_VISUALIZATION_SCALE ([0x10123b4c]) lines, and NX_VISUALIZE_CONTACT_POINT ([0x10123bac]) crosses of +-0.1*scale, drawn via debug renderer vtable +0x20; thiscall ret 4; caller 000907 | visualisation (contact debug visualization) | none | missing | - | none | - | no | NxScene::visualize (NpScene vtable slot 31 @0x10105b14) -> 000344 -> 000657 -> 000907 -> row |
| 000871 | 0x1d590 | 122 | discovered | spring-contact test for a patch: materials of ushorts patch+0x68/+0x6a; returns (flagsA\|flagsB)&4; cdecl; callers 000891, 000895 | contact-pair manager (contact material) | none | missing | - | none | - | yes | simulation step |
| 000873 | 0x1d610 | 706 | dynamically_gated | contact-stream emitter (sink=this): orientation swap/negate, pair header (obj ids +0x9c, material<<24\|flag<<16), normal block on raw-word change, contact record (sep&0x7fffffff, optional feature word); grows through 004840 at 8 sites; ret 0x1c; callers 001753, 001770, 001775, 001779, 001859, 001865, 001876, ... | contact-pair manager (narrow-phase contact stream) | ContactGeneration.cpp:217 NxEmitContact (+ helpers :68-209) | partial | defect: stream growth (call 0x100b4de0 at 0x1001d6e7/0x1d711/0x1d752/0x1d7c7/0x1d7fc/0x1d83f/0x1d87b/0x1d8a8) is not called; nxReserve (:56) drops the write instead. Everything else I walked matches | none | not hit | yes | simulation step (narrow-phase matrix rows) |
| 000875 | 0x1d8e0 | 915 | discovered | second contact-stream emitter with 32-bit features: like 000873, plus shape+0xde bit 0x20 -> sink flag 4; when set and either feature id >=0x10000, sep word gets bit31 and two full feature words, else packed fid1<<16\|fid0; ret 0x24; callers 001762, 001779, 001844, 001909, 001927, 001929 -> 004840 | contact-pair manager (narrow-phase contact stream) | none | missing | - | none | - | yes | simulation step |
| 000877 | 0x1dc80 | 189 | discovered | FrictionPatch copy (dwords +4..+0x70, bytes +0x74/+0x75; the vptr is not copied); thiscall ret 4; caller 000891 | contact-pair manager (contact friction patches) | none | missing | - | none | - | yes | simulation step |
| 000879 | 0x1dd40 | 1983 | discovered | friction row generation: for each patch (inline +0x4c, then ptr vector +0xc4) with normal force +0x64!=0 and 000861 enabled: for each anchor, world points, tangential error, two 0x50-byte scene records (scene+0x5b8/+0x5bc/+0x5c0, grown by 000598) along t1/t2, kind bits, 004391 effective mass, *[0x10123b18] or *0.7; then zeroes patch +0x58..+0x64; thiscall ret 0xc; caller 000897 | contact-pair manager (contact/friction constraint generation) | none | missing | - | none | - | yes | simulation step |
| 000881 | 0x1e500 | 213 | discovered | sum the pair's contact forces: out0 = sum over patches of patch.n * normalForce (+0x98.. * +0xb0, then others +0x4c * +0x64), out1 = sum of friction force (+0xa4.., +0x58..); thiscall ret 8; callers 000913, 000917 | contact-pair manager (contact report) | none | missing | - | none | - | no | contact reports (000913 / 000917) |
| 000883 | 0x1e5e0 | 200 | discovered | spring-contact row: picks the pair shape with type 3 (capsule) and +0xe8 bit 1 (NX_SWEPT_SHAPE); separation along its axis minus +0xe4 minus [0x10123b1c]; one 0x50 scene record (000598 grow) using material programData (+0x44, NxSpringDesc) and scene +0x548/+0x54c; 004393 gains; ret 0x14 (body continues in 000885); caller 000897 | contact-pair manager (spring contact / swept-capsule wheel) | none | missing | - | none | - | yes | simulation step |
| 000885 | 0x1e6b0 | 593 | discovered | tail of 000883 (reached by `jmp` at 0x1001e6a6 and loop `jb` at 0x1001e6cb); holds the shape search, record build and 004393 call | contact-pair manager (spring contact) | none | missing | - | none | - | yes | no callers of its own: fall-through body of 000883 |
| 000887 | 0x1e910 | 41 | discovered | pair (ActorPair) clear/dtor: reset stream via 002354(+0x10), free every extra FrictionPatch in +0xc4..+0xc8 (allocator slot 0x14), free the vector, zero +0x48/+0xc4/+0xc8/+0xcc, 002352(+0x10); fastcall; body continues in 000889; caller 000903 | contact-pair manager (contact pair management) | none | missing | - | none | - | no | pair deletion (see 000915) |
| 000889 | 0x1e940 | 99 | discovered | tail of 000887 (the free loop), reached by `jmp` at 0x1001e937 and `jb` at 0x1001e969 | contact-pair manager (contact pair management) | none | missing | - | none | - | no | no callers of its own: fall-through body of 000887 |
| 000891 | 0x1e9b0 | 1509 | discovered | FrictionPatch refresh/cull: per patch, world normals/anchors from both bodies; drops a patch (000877 moves the last one in, allocator free) when stale frame (+0xd8+1 != scene+0x540), normal dot <=0.996, anchor drift >= -[0x10123b1c], or spring flag; otherwise re-anchors (000865-style) and stores the normal; accumulates +0xd4; ret 4; caller 000897 | contact-pair manager (contact friction patches) | none | missing | - | none | - | yes | simulation step |
| 000893 | 0x1efa0 | 109 | discovered | ActorPair ctor: 002356(+0x10) stream, patch vptr 0x10106944 at +0x4c, zero vector/counters, +0xe8/+0xe9=0xff, +0/+4 = shapes' owners, +8/+0xc = their bodies; ret 8; caller 000901 (callees include 000867, 004248, 005242, 001583 via the patch vtable/inline) | contact-pair manager (contact pair management) | none | missing | - | none | - | yes | simulation step (broadphase new pair) |
| 000895 | 0x1f010 | 779 | discovered | find-or-create FrictionPatch for (material pair, shape ids, normal): matches a patch with normal dot >=0.996, else uses inline +0x4c or allocates 0x78 (vptr 0x10106944) and pushes onto the grown vector; stores body-local normals and ids, +0x64 = +0xd4 unless spring; ret 0x10; caller 000897 | contact-pair manager (contact friction patches) | none | missing | - | none | - | yes | simulation step |
| 000897 | 0x1f320 | 554 | discovered | ActorPair::generateConstraints: skips when either shape +0x14 bit 2 is set; 000891 refresh; walks the contact stream (+0x40): per normal block 000857 restitution; per contact 000895 patch, anchor selection (up to 2, max distance), one 0x50 contact record per contact (rA x n, rB x n, penalty, 004391, bounce threshold [0x10123b28]); spring contacts -> 000883; then 000865 and 000879; thiscall ret 0xc; caller 000728 | contact-pair manager (contact constraint generation) | none | missing | - | none | - | yes | simulation step: NxScene::simulate worker (vtable 0x10108898 -> 002400) -> 000659 -> 000655 -> 000611 -> 000730 -> 000728 -> row |
| 000899 | 0x1f550 | 1356 | discovered | loop body of 000897 (0x1f550..0x1fa9c), reached by jmp/je from 0x1001f544/0x1f548/0x1f94a/0x1f9a8/0x1f9f0/0x1fa4c: the per-contact record build and anchor bookkeeping | contact-pair manager (contact constraint generation) | none | missing | - | none | - | yes | no callers of its own: fall-through body of 000897 |
| 000901 | 0x1faa0 | 140 | discovered | broadphase pair node ctor (0x108 B): 000893 at +0x14, +0x104=-1, +0x10=list, links the node at the tail (both actors dynamic) or the head of the list; ret 0xc; caller 000911 | contact-pair manager (contact pair management) | none | missing | - | none | - | yes | simulation step |
| 000903 | 0x1fb30 | 120 | discovered | pair node unlink from the doubly-linked list (+8 next/+0xc prev, list head/tail at [+0x10]) then 000887(+0x14); fastcall; caller 000915 | contact-pair manager (contact pair management) | none | missing | - | none | - | no | pair deletion |
| 000905 | 0x1fbb0 | 491 | discovered | per-pair refresh: skip jointed pairs (000750: joint on body+0x1d8 list without the collision flag); needs a dynamic owner (+0x10c sign) and no disabled shapes; actor-pair report record via hash 004153/004155 (0x14-byte record); on actor change 000863 + 002348 re-register; stores pair/shape ptrs and bit31 in the report record; ret 4; caller 000909 | contact-pair manager (contact pair management / report flags) | none | missing | - | none | - | yes | simulation step (001976 -> 000909) |
| 000907 | 0x1fda0 | 8 | discovered | thunk: ecx+=0x14, jmp 000869; ret 4; caller 000657 | visualisation (contact visualization) | none | missing | - | none | - | no | NxScene::visualize -> 000344 -> 000657 -> row |
| 000909 | 0x1fdb0 | 99 | discovered | walk the pair list; for pairs where either body has +0x4c != 0, call 000905; ret 4; caller 001976 | contact-pair manager (contact pair management) | none | missing | - | none | - | yes | simulation step (000655 -> 000608 -> 001976) |
| 000911 | 0x1fe20 | 143 | discovered | create pair node: order by shape +0xd4, check the collision-group table [0x10123a98] with groups +0xd8, allocate 0x108, 000901; ret 8; callers 001971, 001976 | contact-pair manager (contact pair management) | none | missing | - | none | - | yes | simulation step (broadphase new pair) |
| 000913 | 0x1feb0 | 359 | discovered | fire contact reports immediately: per actor-pair record in the hash, compute NX_NOTIFY_ON_START/ON_END/ON_TOUCH (2/4/8) against frame scene+0x540 and pair flags ([0x10123c28] via 004153), fill NxContactPair {actors, stream, sumNormalForce/sumFrictionForce via 000881}, call userReport vtable slot 0, free stale records (004157); cdecl; caller 000663 | contact-pair manager (contact report) | none | missing | - | none | - | no | NxPhysicsSDK::releaseScene -> Scene table 0x101066f4 slot 0 (000668) -> 000663 -> row |
| 000915 | 0x20020 | 33 | discovered | delete pair node: if non-null 000903, then allocator free; stdcall ret 4; callers 001953, 001955 | contact-pair manager (contact pair management) | none | missing | - | none | - | no | shape/actor release: scalar deleting dtors -> 001323 -> 001955 -> row; Scene dtor 000663 -> 001953 -> row |
| 000917 | 0x20050 | 39 | discovered | buffered contact reports: same event logic as 000913 but appends 0x2c-byte records to the scene vector +0x60c/+0x610/+0x614 (grown x2+2); body continues in 000919/000921; cdecl; caller 000655 | contact-pair manager (contact report) | none | missing | - | none | - | yes | simulation step (000655) |
| 000919 | 0x20080 | 477 | discovered | loop body of 000917 (reached by `jmp` at 0x10020075 and `jne` at 0x100203a5) | contact-pair manager (contact report) | none | missing | - | none | - | yes | no callers of its own: fall-through body of 000917 |
| 000921 | 0x20260 | 339 | discovered | record-copy / append tail of 000917 (reached by `jmp` at 0x1002025b and `jne` at 0x100202ae) | contact-pair manager (contact report) | none | missing | - | none | - | yes | no callers of its own: fall-through body of 000917 |
| 000923 | 0x203c0 | 120 | discovered | AABB/NxBounds3::setCenterExtents(c,e): min=c-e, max=c+e (identical-code-folded out-of-line copy); thiscall ret 8; callers 000973, 002280, 002296, 002304, 005143, 005295, 005317, 005318 | shape | External/opcode/upstream/Opcode/Ice/IceAABB.h:82 AABB::SetCenterExtents (same code as Foundation/include/NxBounds3.h:263 NxBounds3::setCenterExtents) | implemented | faithful | BFlag 1, CMass 11, DSet 7, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 8, Meta 1, Mom 4, Name 1, ShMut 2 | not hit | no | NxScene::createActor -> BOX loadFromDesc 000981 -> 000973 -> row; OPCODE AABB::MakeCube 005143 |
| 000925 | 0x20440 | 13 | reconstructed | Element ctor: zero [this+0,4,8], return this. No direct callers; its address is pushed at 0x1002b786 as the ctor of a `new T[n]` (element size 0x24, vector-ctor iterator 0x1000) inside 001472 (0x2b6f0, discovered) | other (array element ctor used by 001472) | none in Physics/src (the inventory "source" is a description; the 3z44 drive was harness-only) | missing | - | none | - | no | only via 001472 (discovered); no public path established |
| 000927 | 0x20450 | 40 | reconstructed | BOX slot 13 saveToDesc: dims +0xe4/e8/ec -> rec+0x4c/50/54, tail-jmp BASE save 0x256f0 (001277) | shape | OM:5544 BoxShape::nxBoxSaveState | implemented | faithful | DSet 1 | not hit | no | NxBoxShape::saveToDesc -> NpBoxShape (call [edx+0x34] at 0x100234a8) -> slot 13 |
| 000929 | 0x20480 | 7 | reconstructed | `lea eax,[ecx+0xe4]`: address of box dims. Callers 001071 (NpBoxShape::getDimensions 0x23520), 001770 | shape | NPA:487 nxBoxHandleGetDimensions (inlined `+0xe4`); OM:1537 nxLockedFieldAddress is a generic unbound helper | implemented | faithful | DSet 1, Life 4 | yes: DSet 1, Life 4 | no | NxBoxShape::getDimensions -> 001071 -> row |
| 000931 | 0x20490 | 70 | reconstructed | Fill 60-byte box descriptor: out[0..2]=+0x30.., rep movsd 9 from +0x0c to out+0x18, out[3..5]=+0xe4.. (ret 4). Callers 000945, 002310 | shape | OM:324 BoxShape::nxFillShapeDescriptor | implemented | faithful | none | not hit | no | debug visualization: shape slot 3 (000945) |
| 000933 | 0x204e0 | 188 | discovered | Box getWorldOBB: identity 3x4 on stack, 001309 (0x25f60, shape global pose from [owner+8] body pose x local pose) fills it; out center = pose t, out+0x18..0x3c = rot, out+0xc..0x14 = dims (ret 4). Caller 001073 (0x23550) | shape | NPA:492 nxBoxHandleGetWorldOBB | partial | defect: copies cached +0x30/+0x0c instead of calling 001309 at 0x1002054d | DSet 1 | yes: DSet 1 | no | NxBoxShape::getWorldOBB -> 001073 -> row |
| 000935 | 0x205a0 | 198 | reconstructed | BOX slot 9 world AABB: ext_k = sum abs(dim*rot) per row, min=t-ext, max=t+ext (ret 4) | shape | OM:5564 BoxShape::nxBoxWorldAABB | implemented | defect: association/precision (see notes) at 0x100205ce, 0x100205dc, 0x10020625, 0x10020646 | DSet 2 | yes: DSet 2 | no | shape slot 9 (world bounds; broadphase update, NxShape::getWorldBounds) |
| 000937 | 0x20670 | 69 | reconstructed | BOX slot 10: out[0..2]=pose-one t, out[3]=sqrt((dx^2+dy^2)+dz^2) | shape | OM:5520 BoxShape::nxBoxCenterAndDiagonal | implemented | defect (minor): CRT/SSE sqrt instead of an x87 fsqrt helper at 0x100206a7 | DSet 1 | yes: DSet 1 | no | shape slot 10 (bounding sphere; also called by 001305 debug render) |
| 000939 | 0x206c0 | 62 | reconstructed | BOX slot 11: out[0..2]=0, out[3]=same diagonal | shape | OM:5532 BoxShape::nxBoxZeroCenterAndDiagonal | implemented | defect (minor): CRT/SSE sqrt at 0x100206f0 | none | not hit | no | shape slot 11 (no direct callers) |
| 000941 | 0x20700 | 68 | reconstructed | BOX slot 8 local AABB: out[0..2]=-dims, out[3..5]=dims | shape | OM:5552 BoxShape::nxBoxLocalAABB | implemented | faithful | DSet 1 | yes: DSet 1 | no | shape slot 8 (no direct callers) |
| 000943 | 0x20750 | 139 | dynamically_gated | Box corner: (int signs fild)*dims, R*v with per-row float spill, + t; `__thiscall ret 0x10`. Callers 001881 [PLANE][BOX], 001883 | shape | Physics/src/NarrowPhase.cpp:62 NxBoxShapeCorner (also used ContactGeneration.cpp:950) | implemented | defect: calling convention: the row is thiscall ret 0x10 (ecx = shape, four stack args); NxBoxShapeCorner (NarrowPhase.cpp:62) is a cdecl free function. Arithmetic walked faithful: fild signs, rows (fz*m2 + fy*m1) + fx*m0 with a float spill, then + t | none | not hit | yes | simulation step (plane-box overlap/contact matrix) |
| 000945 | 0x207e0 | 104 | discovered | BOX slot 3 debug render: gate +0xde&8 (001287); 001305(renderer); if param 0x10123bc4 != 0.0 (unordered renders): 000931 descriptor, renderer vtbl+0x28(desc, color, 0), color 0xffffffff/0xffff00ff by +0xde&7 | shape | OM:573 BoxShape::nxDebugRenderDispatch | implemented | defect: guard read through test-bound pointer (0x10020804), not the SDK parameter array | none | not hit | no | NxScene debug visualization -> shape slot 3 |
| 000947 | 0x20850 | 39 | reconstructed | BOX slot 4 mass: if !(byte +0xde & 7) call 000849(dest=arg1; density, +0xe4, +0x6c); return 1 (ret 0xc) | shape | OM:4941 BoxShape::nxBoxAccumulateMass | implemented | faithful | BFlag 1, Dyn 1, DynF 1, JAll 2, JSP 8, JSlot 27, Life 7 | not hit | no | actor mass from shapes (createActor/updateMassFromShapes) -> slot 4 |
| 000949 | 0x20880 | 663 | discovered | BOX slot 5 raycast: world ray -> local, 001726 ray/AABB, write hit point/t, gate t>max, fill hit record (+0x00 colobj, tag 0x13/0x17, normal) return this (ret 0x14) | shape | OM:343 BoxShape::nxBoxRaycast | implemented | defect: x87 precision/association (5 sites) and normal sign at 0x10020a9b (see notes) | none | not hit | no | NxScene raycast* -> shape slot 5 |
| 000951 | 0x20b20 | 507 | discovered | BOX slot 7: exit distance from box center along arg2 direction: local origin R^T(-t)+R^T t, local dir R^T d, 001730 (0x38050) slab test vs [-dims,+dims], *arg1 = abs(far t), return 1/0 (ret 8) | shape | OM:423 BoxShape::nxBoxSweep | partial | defect: fitted model, not the listing (no 001730 call, wrong formula); rewrite | none | not hit | no | shape slot 7 (no direct callers) |
| 000953 | 0x20d20 | 6 | reconstructed | facade slot 1: `mov eax,8` vertex count | shape | OMH:138 BoxHullFacade::kVertexCount (constant only) | partial | defect: constant only (BoxHullFacade::kVertexCount = 8 matches `mov eax,8` at 0x10020d20); the row is a real function, slot 1 of facade vtable 0x10106a88, called through [+0xe0] by 000973, and no product function has its ABI | BFlag 6, CMass 66, DSet 42, Dyn 12, DynF 6, Force 66, JAll 12, JSP 48, JSlot 162, Life 48, Meta 6, Mom 24, Name 6, ShMut 12 | no own address | no | facade slot 1; called by 000973 via [+0xe0 vtbl]+4 |
| 000955 | 0x20d30 | 4 | reconstructed | facade slot 2: `lea eax,[ecx+0x10]` vertices | shape | OM:71 BoxHullFacade::vertices | implemented | faithful | BFlag 6, CMass 66, DSet 42, Dyn 12, DynF 6, Force 66, JAll 12, JSP 48, JSlot 162, Life 48, Meta 6, Mom 24, Name 6, ShMut 12 | not hit | no | facade slot 2; called by 000973 |
| 000957 | 0x20d40 | 578 | discovered | facade slot 9: optional 3x4 transform (arg1, 4x4 stride) of dir arg2, then argmax over faces of normal(record+0xc)·dir (first face seeds, update on strictly greater, unrolled x4); returns face index (ret 8). Calls own slot 3 | shape | none | missing | - | none | - | no | no direct callers (facade table 0x10106a88 slot 9) |
| 000959 | 0x20f90 | 1062 | discovered | facade slot 10: same best-face search, then best of the 12 edges via slots 6/7/8 tables (+0x18/+0x1c/+0x20), returns feature and writes arg3 (ret 0xc) | shape | none | missing | - | none | - | no | no direct callers (facade slot 10) |
| 000961 | 0x213c0 | 6 | reconstructed | facade slot 3: `mov eax,6` face count | shape | OMH:140 BoxHullFacade::kFaceCount (constant only) | partial | defect: constant only (BoxHullFacade::kFaceCount = 6 matches `mov eax,6` at 0x100213c0); the row is slot 3 of facade vtable 0x10106a88, called by 000973/000957/000959, and no product function has its ABI | BFlag 7, CMass 77, DSet 49, Dyn 14, DynF 7, Force 77, JAll 14, JSP 56, JSlot 189, Life 56, Meta 7, Mom 28, Name 7, ShMut 14 | no own address | no | facade slot 3; called by 000973, 000957, 000959 |
| 000963 | 0x213d0 | 14 | reconstructed | facade slot 4: face record k = this+0x70+36k (ret 4) | shape | OM:76 BoxHullFacade::face | implemented | faithful | none | not hit | no | facade slot 4 (no direct callers) |
| 000965 | 0x213e0 | 3 | reconstructed | `xor eax,eax; ret`: facade slot 5 and Prunable owner-query hook (.data 0x10128470). Callers 004996, 005071, 005111, 005406, 005495 | shape | OM:235 shapeOwnerQuery (+ OMH kZero) | implemented | faithful | none | not hit | no | pruning queries via .data 0x10128470 (simulation step, scene queries) |
| 000967 | 0x213f0 | 6 | reconstructed | facade slot 6: returns .rdata 0x10122180 (24-dword edge-pair table) | shape | OM:82 BoxHullFacade::edgeTable (gEdgeTable OM:64) | implemented | defect: table has 12 of 24 dwords | none | not hit | no | facade slot 6; used by 000959 |
| 000969 | 0x21400 | 6 | reconstructed | facade slot 7: returns 0x101221e0 (24 dwords) | shape | OM:87 faceCornerTable (gFaceCornerTable OM:66) | implemented | defect: 12 of 24 dwords | none | not hit | no | facade slot 7; used by 000959 |
| 000971 | 0x21410 | 6 | reconstructed | facade slot 8: returns 0x10122240 (24 dwords) | shape | OM:92 adjacencyTable (gAdjacencyTable OM:68) | implemented | defect: 12 of 24 dwords | none | not hit | no | facade slot 8; used by 000959 |
| 000973 | 0x21420 | 913 | discovered | Box hull rebuild: 000923 OBB(center 0, identity, dims) + 005147 (0xe2fd0, vendored ICE) corners -> +0xf0; face records: index lists 0x10106998../0x101069f8.., corner count 4, axis normals, plane d; then per face (facade slots 3/1/2) min/max projection of all vertices (FLT_MAX seeds) | shape | none (OM:4276 is a comment) | missing | - | BFlag 1, CMass 11, DSet 7, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 8, Meta 1, Mom 4, Name 1, ShMut 2 | - | no | NxScene::createActor -> 000032 -> slot 12 000981 -> row; NxBoxShape::setDimensions -> 001069 -> 000983 -> row |
| 000975 | 0x217c0 | 168 | reconstructed | facade slot 11: project 8 vertices through pose, min/max along dir (ret 0x18) | shape | OM:177 BoxHullFacade::supportBounds | implemented | defect: calling convention: the row is thiscall ret 0x18 (six stack dwords, 0x10021865); supportBounds (ObjectModel.cpp:177) is a four-argument member (ret 0x10) and is not installed in any table. Arithmetic walked faithful (associations A/B/C and (C*dz + B*dy) + A*dx) | none | not hit | no | facade slot 11 (no direct callers) |
| 000977 | 0x21870 | 207 | reconstructed | Box ctor (ret 8): base 001273, vptr 0x10106ab8, facade vptr 0x10106a88 at +0xe0, zero face words, colobj 001075, +0xd0=2, dims 1.0 | shape | OM:4000 BoxShape::BoxShape | implemented | defect: +0xe0 facade vptr never stored (0x10021895); allocator: the 0x1c-byte collision object is allocated through nxGetSdkAllocator() (ObjectModel.cpp:4022) where the oracle uses [0x101041bc] at 0x100218f1 | BFlag 1, CMass 11, DSet 6, Dyn 2, DynF 1, Force 11, JAll 2, JSP 6, JSlot 27, Life 7, Meta 1, Mom 4, Name 1, ShMut 2 | not hit | no | NxScene::createActor -> 000626 -> 000034 -> 000032 -> row |
| 000979 | 0x21940 | 69 | reconstructed | BOX slot 0 scalar deleting dtor | shape | OM:5587 BoxShape::nxBoxScalarDeletingDtor | implemented | defect: allocator: the scalar-deleting free goes through nxGetSdkAllocator() (ObjectModel.cpp:5597) where the oracle frees through [0x101041bc] slot +0x14 at 0x10021971; the vptr restores [esi]=0x10106ab8 and [esi+0xe0]=0x10106a88 at 0x1002194b/0x10021951 are dropped | BFlag 1, CMass 11, DSet 6, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 8, Meta 1, Mom 4, Name 1, ShMut 1 | not hit | no | NxActor::releaseShape / NxScene::releaseActor -> slot 0 |
| 000981 | 0x21990 | 55 | reconstructed | BOX slot 12 loadFromDesc: dims <- desc+0x4c.., 000973, BASE apply 0x27740; returns its al=1 (ret 4) | shape | OM:4270 BoxShape::nxBoxLoadFromDesc | partial | defect: 000973 call at 0x100219b5 omitted; bool return dropped | BFlag 1, CMass 11, DSet 6, Dyn 2, DynF 1, Force 11, JAll 2, JSP 8, JSlot 27, Life 8, Meta 1, Mom 4, Name 1, ShMut 2 | not hit | no | NxScene::createActor -> 000034 -> 000032 (call [eax+0x30] at 0x10001efd, tests al) -> row |
| 000983 | 0x219d0 | 62 | discovered | Box setDimensions: dims <- *arg, 000973, slot 6(1), 001325(0x40) (ret 4). Caller 001069 | shape | NPA:557 nxBoxHandleSetDimensions | partial | defect: no 000973 (0x100219f3); dirty flag 0x20 vs 0x40 (0x10021a01) | DSet 1 | yes: DSet 1 | no | NxBoxShape::setDimensions -> 001069 -> row |
| 000985 | 0x21a10 | 78 | reconstructed | facade slot 0: once-guarded static (0x10123c64) zeroed + atexit(0x10103010), returns its address | shape | OM:217 BoxHullFacade::sharedHook | implemented | faithful | none | not hit | no | facade slot 0 (no direct callers) |
| 000987 | 0x21a60 | 101 | reconstructed | Capsule ctor (ret 8): base, vptr 0x10106b20, +0xe0/+0xe4=0, colobj 001123, +0xd0=3 | shape | OM:5261 CapsuleShape::CapsuleShape | implemented | defect: allocator: the 0x1c-byte collision object is allocated through nxGetSdkAllocator() (ObjectModel.cpp:5272) where the oracle uses [0x101041bc] at 0x10021a8e; the rest (base, vptr 0x10106b20, +0xe0/+0xe4, colobj 001123, +0xd0=3) matches | DSet 1 | not hit | no | NxScene::createActor -> 000034 -> 000032 -> row |
| 000989 | 0x21ad0 | 110 | reconstructed | CAPSULE slot 12 loadFromDesc: radius, height*0.5, +0xe8; report(1, CapsuleShape.cpp, 0x37, 0, "...loadFromDesc: radius should be positive!") when radius <= 0; BASE apply; returns its al (ret 4) | shape | OM:5479 CapsuleShape::nxCapsuleLoadFromDesc | implemented | defect: NaN radius reported (0x10021b07); bool return dropped (0x10021b34) | DSet 1 | not hit | no | NxScene::createActor -> 000034 -> 000032 -> row |
| 000991 | 0x21b40 | 42 | reconstructed | CAPSULE slot 13 saveToDesc: +0x4c=r, +0x50=2*h, +0x54=+0xe8, tail-jmp 0x256f0 | shape | OM:5282 CapsuleShape::nxCapsuleSaveState | implemented | faithful | DSet 1 | yes: DSet 1 | no | NxCapsuleShape::saveToDesc -> NpCapsuleShape -> slot 13 |
| 000993 | 0x21b70 | 108 | discovered | Capsule setDimensions(r, h) (ret 8): +0xe0=r, +0xe4=h*0.5, report(1, CapsuleShape.cpp, 0x4f, 0, "CapsuleShape::setDimensions: radius should be positive!") when r <= 0, slot 6(1), 001325(0x100). Caller 001113 | shape | NPA:566 nxCapsuleHandleSetDimensions | partial | defect: no report arm (0x10021b91..c0); dirty flag 0x20 vs 0x100 (0x10021bcc) | DSet 1 | yes: DSet 1 | no | NxCapsuleShape::setDimensions -> 001113 -> row |
| 000995 | 0x21be0 | 23 | reconstructed | CAPSULE slot 14 setRadius: +0xe0=arg, tail-jmp slot 6 with arg forced to 1 | shape | OM:5295 CapsuleShape::nxCapsuleSetRadius (slot 14); NPA:543 nxCapsuleHandleSetRadius inlines the full row | implemented | defect: slot-14 member omits the slot-6 tail jump at 0x10021bf4 | DSet 1 | yes: DSet 1 | no | NxCapsuleShape::setRadius -> NpCapsuleShape (call [edx+0x38] at 0x10023bd5) -> row |

## Review rules applied (revision 2)

The first pass of this contract marked some rows `faithful` without applying three project rules. Every
`implemented` or `partial` row was re-swept against them:

- **(a) Math intrinsics.** CRT math is a defect where the oracle uses x87 instructions (fsqrt, fsin,
  fcos). The rows whose listings contain such instructions are 000688, 000696, 000706, 000726, 000736,
  000740, 000754, 000756, 000766, 000768, 000789, 000801, 000861, 000937 and 000939.
  - 000754 goes through the X87Sqrt.h helpers, so it stays faithful.
  - 000756, 000768, 000789 and 000801 reach CRT `sqrt()` (NpActorDynamicMath.h:64/77/105-199 and
    NpActor.cpp:1521). They are defects.
  - 000937 and 000939 use CRT sqrt. They are defects.
  - The other rows in that list have no candidate.
- **(b) Allocator.** Every row in these units that allocates or frees uses the imported Foundation
  allocator [0x101041bc]. None calls nxGetSdkAllocator (row 004803, 0xb4000). The candidates that use
  nxGetSdkAllocator() are therefore defects:
  - 000977 (ObjectModel.cpp:4022, oracle 0x100218f1);
  - 000979 (:5597, oracle 0x10021971);
  - 000987 (:5272, oracle 0x10021a8e);
  - 000785/000787 (NpActor.cpp:141/158);
  - the dirty-mark vector growth inside 000782, 000784, 000789 and 000793/000795 (NpActor.cpp:109/113,
    nxNpActorMarkRecordDirty).

  JointSupport.cpp (000760, 000780) uses nxFoundationSDKAllocator and is faithful. ObjectModel.cpp has 17
  nxGetSdkAllocator uses; only the three listed above are in these units. The others belong to rows
  001079, 000028, 000118, 002326/002328/002340, 001263, 001375, 001399 and the sphere, plane and mesh
  constructors.
- **(c) Calling convention.** A candidate whose ABI differs from the row's is a defect, as recorded for
  000713:
  - 000744 (cdecl, row takes ecx);
  - 000768 (cdecl, row is thiscall);
  - 000849, 000851 and 000853: the rows are thiscall on the destination MassFrame; the candidates are
    shape members taking the destination as an extra argument;
  - 000943 (cdecl free function, row is thiscall ret 0x10);
  - 000975 (four-argument member, row is ret 0x18).

  Candidates whose `this` type differs but whose ABI is identical stay faithful: 000827 (a BoxShape
  member that writes through `this` as raw storage) and 000985 (a static member in a slot whose `this` is
  unused).

Rows whose verdict changed from `faithful` to a defect in this revision: 000744, 000768, 000851, 000853,
000943, 000975, 000979 and 000987. Rows already recorded as defects that gained a finding: 000756, 000782,
000784, 000785, 000787, 000789, 000793, 000795, 000801, 000849 and 000977.

## Scene raycast

(Task 3 wrote the API; see `## Task 3 results`. The audit as recorded:)

**The public scene raycast API is missing.** All six `NxScene` raycasts in `Physics/src/NpScene.cpp`
(455-491) are `// (unimplemented)` stubs that return 0, false or null. No `SceneRaycast.cpp` exists.
`ShapeRaycast.cpp` holds only the per-shape slot-5 raycasts that the loops dispatch to: sphere 001377,
capsule 001010 and plane 001261. BOX slot 5 is 000949, reproduced in ObjectModel.cpp as `nxBoxRaycast`,
which is not faithful. No existing target calls a scene raycast, so none of these rows is reached.

**In scope for Task 3.** The oracle's public entries and loop helpers sit outside these three units, but
Task 3 has to write them to make the rows reachable. No parallel session claims them: the NpActor session
owns 0x2610-0xb100, and these are at 0xced0-0xd2b0 and 0x15060-0x153bf.

| Slot (NpScene table 0x10105a98) | NxScene method | NpScene row (NpScene.cpp unit) | Scene row | Loop row(s) |
|---:|---|---|---|---|
| 42 | raycastAnyBounds | 000366 (0xced0) | 000690 | 000680 (0x15060, gap:Scene..SceneRaycast) |
| 43 | raycastAnyShape | 000368 (0xcf90) | 000704 | 000698 |
| 44 | raycastAllBounds | 000370 (0xd050) | 000692 | 000682 (0x15150, gap:Scene..SceneRaycast) |
| 45 | raycastAllShapes | 000372 (0xd110) | 000694 | 000684 + 000686 (0x152e0/0x15300, gap:Scene..SceneRaycast) |
| 46 | raycastClosestBounds | 000374 (0xd1d0) | 000696 | 000688 |
| 47 | raycastClosestShape | 000376 (0xd2b0) | 000706 | 000700 + 000702 |

What the rows share:
- **NpScene wrappers.** Each takes the write guard at +0xc (0x5b730; on failure `PhysicsSDK: WriteLock
  is still aquired...`, return 0). It then requires `maxDist > 0` (error 1, "Scene::raycastXxx: The maximum
  distance must be greater than zero!", strings 0x10105d90..0x10105f18). It calls the Scene row on
  this+0x24 and unlocks through 0x5b790.
- **Scene rows.** Each checks for a unit direction: `|d.d - 1| < eps` against the double at 0x10106850,
  with error(1, SceneRaycast.cpp, line, "NxRay direction not valid: must be unit vector."). It resets the
  collector count at Scene+0x504 and builds the pruner mask: bit 0 static, 0xe dynamic, from shapesType.
  004864 (on Scene+0x624; reconstructed, candidate nxMaskedFourSlotLoop4864 is not ABI-identical) fills the
  collector at Scene+0x500 and returns 1. The loop row then walks the collector.
- **Loop filter.** A shape is skipped when byte +0xde bit 0x40 is set, or when its group word +0xd8 is
  not 0xffff and its bit is clear in `groups`.

Oracle quirks to keep:
- 000374 validates maxDist, then passes `NX_MAX_F32, 0xffffffff` (0x1000d263/0x1000d265), not the
  caller's maxDist and hintFlags.
- 000694 (raycastAllShapes) always returns 0 (`xor eax,eax` at 0x10015789) and discards 000684's count.
- 000688 keeps the squared distance in hit.distance. 000696 and 000706 take fsqrt at the end.
- 000702 compares a squared distance (IMPACT) against a raw distance (DISTANCE only).
- 000374 and 000376 return [ret+0x9c], the public NxShape.
- 000700+000702 and 000684+000686 are each one function that the splitter cut in two.

Custom register conventions: 000688 takes ecx = collector and ebx = hit. 000698 takes eax. 000700 takes
ecx, ebx and esi. 000680, 000682 and 000684 take eax, ebx, esi and ebx. All of these are cdecl-style
`ret`. Nothing outside this block calls them.

## Findings by sub-area

The sub-areas are now: scene raycast, body-actor math, island, CCD, shape, mass properties, contact-pair
manager, visualisation, and other.

**Step-only rows.** 46 rows (17,939 B) are reachable only from the simulation step, including the
fragments of step rows. 45 of them (16,991 B) are missing or partial; the exception is 000943,
implemented and reached only by the step's plane-box pair. By sub-area:

| Sub-area | step-only rows | bytes |
|---|---:|---:|
| contact-pair manager | 28 | 13,026 |
| body-actor math | 5 | 2,482 |
| island | 10 | 1,848 |
| CCD | 2 | 444 |
| shape | 1 | 139 |

They have no public path while `NpScene::simulate` and `fetchResults` are stubs (NpScene.cpp 589/601), so
the "Step only" column marks them. They are separate from visualisation: 000766, 000869 and 000907, whose
public path is `NxScene::visualize` (slot 31 -> 000344 -> 000657). That is also a stub
(NpScene.cpp:391).

**Island (union-find).** Two forests share the body record:
- the joint island: root +0x1bc, rank +0x1c0, counts +0x1c4/+0x1c8, +0x1cc, next +0x1d0, tail +0x1d4,
  island object +0x1e0, flags +0x1e4 bit 1;
- the sleep group: root +0x1e8, rank +0x1ec, counts +0x1f0/+0x1f4, float +0x1f8, next +0x1fc,
  tail +0x200.

000722 seeds the second forest from the first. 000722 and 000748 run on every actor creation in the
oracle, and they are missing. The rest of the missing island rows are step-only.

**Body-actor math.** The body is the 0x260-byte dynamic record: vtable 0x10106890, base sub-object at
+0x18 with ctor 000801 and dtor 000799. The product has hand-written emulations of most of these rows in
Scene.cpp (`nxActorComputeMass`, `releaseActor`) and NpActor.cpp (`nxNpActor*`). None was written from
the listing, and every one has defects. 000746 is the only faithful row, and it runs only inlined in
000768. 000768 itself now has two defects: CRT sqrt, and a cdecl candidate where the row is thiscall.

**CCD.** 000738 (swept-box getter), 000740 (swept bounds), 000772 and 000774 (pose restore; reached only
from the CCD sweep 002264; NX_CONTINUOUS_CD defaults to 0). All four are missing, including 000738,
whose inventory state is stale. 000770 (the pose save and integration called each step) stays under
body-actor math.

**Shape.** The box hull facade is the gap:
- the +0xe0 facade vptr is not stored (000977);
- the tables 000967/969/971 hold 12 of 24 dwords;
- 000953 and 000961 exist only as constants;
- 000973, 000957 and 000959 are missing;
- 000981 and 000983 do not call 000973.

On top of that, 000977, 000979 and 000987 use the wrong allocator.

**Mass properties.** Two groups:
- the twelve exported kernels 000803-000825, `dynamically_gated` and faithful;
- the MassFrame rows 000827-000853, `reconstructed`. They carry defects in 000829, 000833, 000845 (a
  real bug), 000847, 000849, 000851 and 000853, and 000841 has no product function.

The oracle runs 000947 -> 000849 -> 000829, 000831, 000833 and 000839 on every dynamic box creation, and
000841 and 000847 through 000008. The candidate never runs this chain: `nxActorComputeMass` computes the
mass inline.

**Contact-pair manager.** 000750/000752 and 000855-000921 (except the visualisation rows 000869 and
000907), about 14 KB:
- material combine: 000855, 000857, 000859, 000861, 000871;
- friction patches: 000865, 000877, 000891, 000895;
- contact and friction constraint generation: 000897+000899, 000879, 000883+000885;
- pair lifetime: 000887+000889, 000893, 000901, 000903, 000905, 000909, 000911, 000915, 000863;
- joint-pair filter: 000750+000752;
- contact reports: 000881, 000913, 000917+000919+000921;
- contact streams: 000873 (partial), 000875;
- 000867 (reconstructed, with a defect).

Only 000867 and 000873 have candidates. 000913 (Scene destructor) and 000915 (shape/actor release) have
non-step paths, but no target reaches them, because no pair exists without a step.

**Other.** 000925 is an element ctor, used only by 001472.

## Rows the splitter cut out of one function

Write each set as one function. The rows after the first have no callers of their own:
- 000684+000686
- 000700+000702
- 000750+000752
- 000778+000780 (already written together)
- 000785+000787
- 000793+000795
- 000883+000885
- 000887+000889
- 000897+000899
- 000917+000919+000921

## State mismatches in inventory.json

Each entry gives the recommended action. None of them changes state in Task 1.

| Row | Inventory | Finding | Recommended action |
|---|---|---|---|
| 000738 | reconstructed; source "flag-to-pointer (0x16c00)"; dynamic_proof "smallflag2 differential (3z106)" | No product function reads +0x244 or tests +0x1e4 bit 9. The `smallflag2` block does exist (`tests/PhysicsObjectLayoutTests.cpp`, a UTF-16 file, line ~5365), but it calls the oracle at 0x16c00 and compares against expectations written in the harness. No product code takes part, so the proof shows the oracle's behaviour and not a reproduction. | Write it in Task 4 together with 000740 (the CCD block), with a stable-ID line, then restate source, implementation and proofs. Until then, treat it as missing. If the coordinator prefers, demote it to discovered now, with the ledger count the validator requires. |
| 000841 | reconstructed; source "negated-offset translate via nxMassFrameTranslate"; proof "negtrans run=5 failures=0" | No product function builds {-offset}. The `negtrans` block (PhysicsObjectLayoutTests.cpp ~3715) composes `nxMassFrameTranslate` with a negation written in the harness. Its only oracle caller 000008 (updateMassFromShapes / setDynamic) is unwritten. | Write a `MassFrame` member with the row's ABI (thiscall on the frame, `ret 0` at 0x1c74a), claim it, and wire it when 000008 is written. Restate the proof then. |
| 000925 | reconstructed; source "zero-init three dwords (0x20440)"; proof "shapegetters2 drive (3z44)" | No product function. `shapegetters2` (~3830) calls the oracle and checks 12 zero bytes. The only user is the vector-ctor call in 001472 (discovered). | Write it with 001472. Until then, treat it as missing. |
| 000953, 000961 | reconstructed | They exist only as constants (`BoxHullFacade::kVertexCount`/`kFaceCount`). The oracle rows are real functions in slots 1 and 3 of facade vtable 0x10106a88, and 000973, 000957 and 000959 call them through [+0xe0]. | Write them as the facade's slot functions when the facade vtable is installed (Task 4 hull batch). |
| 000981 | reconstructed | It does not call 000973 (0x21420) at 0x100219b5, and it drops the bool return of 0x27740. | Add both in the hull batch (Task 4), after 000973 is written. |
| 000713 | reconstructed; `source` null | The candidate exists (ObjectModel.cpp:3393 `nxBodyRecordFixRoot`), but it has the calling-convention defect. | Fix the ABI and set source and implementation (Task 2). |
| 000803-000825 | dynamically_gated; implementation "Physics/src/ContactGeneration.cpp" | The code is in `Physics/src/MassProperties.cpp` (lines 45-166). | Correct `implementation` (and `source`) to MassProperties.cpp when the rows are next touched (Task 2), with no state change. |

## Defects in rows already `reconstructed` or `dynamically_gated`

Task 2 fixes these, from the listing walk, before claiming:
- 000713: cdecl free function for a thiscall row.
- 000742: the live `computeKineticEnergy` has the wrong association (see Handover).
- 000744: cdecl standalone copy, and the live copy is inlined into 000062.
- 000829: F and the pairwise sums are float spills (0x1001bd39, 0x1001bd67/6c/72).
- 000833 (minor): the `x*0.0` addends are omitted.
- 000845: **a real bug.** Selector 1 stores `side` at +0x00 (0x1001c836). The candidate leaves it
  unwritten, so every capsule mass computation folds an uninitialised word.
- 000847 (minor): the oracle tests the low byte only (0x1001c888).
- 000849: the call to 000833 at 0x1001c8eb is replaced by an inline formula, and the convention differs.
- 000851, 000853: convention.
- 000867: v must stay unrounded (0x1001d26b), and v*desc[2] is a float spill (0x1001d299).
- 000873: the stream growth through 004840 is missing.
- 000935: association and precision (0x100205ce, 0x100205dc, 0x10020625, 0x10020646).
- 000937, 000939: CRT sqrt (0x100206a7, 0x100206f0).
- 000943: convention.
- 000967, 000969, 000971: 12 of 24 dwords.
- 000975: convention.
- 000977: facade vptr not stored (0x10021895); wrong allocator (0x100218f1).
- 000979: wrong allocator (0x10021971); vptr restores dropped (0x1002194b/0x10021951).
- 000987: wrong allocator (0x10021a8e).
- 000989: NaN reported (0x10021b07); bool return dropped (0x10021b34).
- 000995: the slot-14 member omits the slot-6 tail jump (0x10021bf4).

## Marker-rule violation for Task 2

`Physics/src/ObjectModel.cpp:1057` begins a comment line with `// phys_fn_000730-equivalent guard
upgrade (0x5b730)...`. It is not a stable-ID line, and it is not about row 000730: it describes the
0x5b730 write guard. It breaks the rule that no other comment line may begin with `// phys_fn_`. Task 2
should reword it, for example `// Guard upgrade (0x5b730, the 000730-style write guard): ...`.

The same file has about 200 other `// phys_fn_` lines that are not in the exact `// phys_fn_NNNNNN
(0x%08x, N B)` form. Most are pre-existing row descriptions from earlier phases, and are out of scope
here. Only lines for rows this block claims need converting when they are claimed.

## Ownership

These handle-path defects are **ours** (not NpActor-unit rows). They get fixed in this block, with
localized edits to NpActor.cpp:

| Row | Oracle caller(s) (unit) | Product candidate |
|---|---|---|
| 000929 | 001071 NpBoxShape::getDimensions (gap:NpBoxShape..NpCapsuleShape), 001770 | NpActor.cpp:487 `nxBoxHandleGetDimensions` (inline +0xe4) |
| 000933 | 001073 NpBoxShape::getWorldOBB (gap:NpBoxShape..NpCapsuleShape) | NpActor.cpp:492 `nxBoxHandleGetWorldOBB` |
| 000983 | 001069 NpBoxShape::setDimensions (NpBoxShape.cpp) | NpActor.cpp:557 `nxBoxHandleSetDimensions` |
| 000993 | 001113 NpCapsuleShape::setDimensions (NpCapsuleShape.cpp) | NpActor.cpp:566 `nxCapsuleHandleSetDimensions` |
| 000995 | capsule table 0x10106b58 (slot 14); called through [edx+0x38] at 0x10023bd5 | ObjectModel.cpp:5295 member, and NpActor.cpp:543 `nxCapsuleHandleSetRadius` |

## Handover to the NpActor session

These rows' defects sit at call sites inside NpActor-unit rows (0x2610-0xb100). The NpActor session owns
the fix, and this block does not edit them:

| Row | NpActor-unit call site | Fix |
|---|---|---|
| 000742 | called only by 000060 (NxActor::computeKineticEnergy). The live `NpActorVtable::computeKineticEnergy` (NpActor.cpp:2156) re-derives the formula with the wrong association. | 000060 calls `nxBodyRecordEnergyWord` (ObjectModel.cpp:1038, faithful to 0x10016dd0-0x10016e22) instead. The convention of that helper (cdecl where the row is fastcall) is this block's fix. |
| 000784 | inlined in 000090 (moveGlobalPosition), 000124 (moveGlobalPose), 000126 (moveGlobalOrientation) | Call a 000784 reproduction (target ORs 1 and 2, the wake block 0x1950a-0x1961b, orientation-only stores only the quaternion) in place of the inline emulation, once this block writes 000784. |
| 000785 | called by 000188/000190 (raiseBodyFlag/clearBodyFlag), and by this block's 000795 | Call the 000785/000787 reproduction (island step, unconditional inverses, Foundation allocator) once this block writes it. Task 2 already moved the kinematic block's malloc and free in `nxNpActorTransitionKinematic` to the Foundation allocator. |
| 000713 | the live copy `nxNpActorGroupRoot` (NpActor.cpp) is used by 000062's `isGroupSleeping` | Call the claimed `nxBodyRecordFixRoot` (ObjectModel.cpp) or keep the copy; both are faithful in logic (Task 2 claim is the ObjectModel.cpp function). |
| 000744 | inlined into 000062 (`NpActorVtable::isGroupSleeping`) | Call the claimed `nxBodyRecordChainSettled` (ObjectModel.cpp, faithful in logic) instead of the inline copy. |
| 000746 | 000140/000142 (getGlobalInertiaTensor/Inverse) call `nxNpActorWorldTensor`, a different helper; 000789's product copy does too | Call `nxNpActorWorldTensorRDRt` (NpActorDynamicMath.h, now `reconstructed`). The oracle row is hit 17 more times than the candidate for this reason (Mom 14, CMass 3). |
| 000756 | 000204/000208 call `nxNpActorUpdateCMassQuaternion` | Nothing to do: Task 2 retargeted that header function to the listing's conversion (now `reconstructed`); the call sites are unchanged. |

Task 2 touched NpActor.cpp in two places only: the kinematic block's allocator in `nxNpActorTransitionKinematic`
(000785/000787, ours), and one comment line near `nxNpActorRefreshCMass` that began `// phys_fn_000768 (` and
now begins `// Row 000768 (` (the marker rule). Scene.cpp: two comment lines reworded the same way (000797,
000768). The NpActorDynamicMath.h edits are listed in `## Task 2 results`.

**Merge hazard.** The NpActor contract walks 000756, 000782, 000784, 000785 and 000789 as callees of its
rows. Their current emulations live in NpActor.cpp:
- `nxNpActorUpdateCMassQuaternion` and `nxNpActorQuaternionFromMatrix`;
- `nxNpActorAccumulateForce`;
- the moveGlobal* bodies;
- `nxNpActorTransitionKinematic`;
- `nxNpActorApplyWorldMassPose`.

The NpActorDynamicMath.h helpers they share are also used by 000768 and 000801 here. Both sessions will
edit these functions. Whichever lands second must re-walk the other's version against the listing and
not merge textually. The shared files are NpActor.cpp and NpActorDynamicMath.h.

## Suggested write order for Tasks 2-4

1. **Task 2 (claim and promote).** No discovered row is both implemented and faithful except 000746,
   which runs only inside 000768, and 000923.
   - 000923: vendored IceAABB `SetCenterExtents`, recorded as vendored correspondence. Its breakpoint is
     not hit until 000973 is written.
   - Fix, then promote: 000768 (X87Sqrt.h helpers in `nxNpActorBodyQuaternionFromMatrix`, and a thiscall
     wrapper); 000746 with it; 000756 and 000801 (the same sqrt fix).
   - The defects in already-reconstructed rows listed above.
2. **Task 3 (scene raycast).** 000688-000706, plus the in-scope helpers 000680, 000682 and
   000684/000686, and the NpScene wrappers 000366-000376.
3. **Task 4 (the remaining missing and partial rows).** In order:
   - body creation 000797, 000801 and 000793/000795 (with 000722 and 000748), and the destructor
     000776/000799;
   - the setters 000782, 000784 and 000785/000787, handed to the NpActor session for their call sites;
   - the hull facade: 000973, 000957, 000959, 000953 and 000961 as slots, the tables, the 000977 vptr;
   - the handle rows 000929, 000933, 000983, 000993 and 000995;
   - 000738/000740 (CCD), 000841 and 000925 (state mismatches).

   The step-only rows would be source-only `reconstructed`.

## Task 2 results

Task 2 fixed the listed defects, claimed every implemented row's product function with its stable-ID line,
and promoted the rows the evidence supports. Candidate trace after the fixes:
`evidence/scene-raycast-trace-task2.txt` (62 breakpoints over the 16 staged-pair targets, candidate
NxPhysics.dll sha256 12ad8413b4fd51ce...; the final build differs from it only in the two link timestamps,
PE header and debug directory).

**Promoted to `reconstructed` (3 rows, 1,934 B), all with dynamic evidence:**

| Row | B | Candidate | Evidence |
|---|---:|---|---|
| 000768 | 1164 | `nxNpActorUpdateMassFrame` (NpActorDynamicMath.h; NpActor.obj and Scene.obj copies) | 191 hits, per target exactly the oracle's counts |
| 000746 | 245 | `nxNpActorWorldTensorRDRt` (NpActorDynamicMath.h), now `__declspec(noinline)` | 191 hits, one per 000768 call; the oracle's 17 extra calls come from 000140/000142/000789, whose product copies use another helper (handover) |
| 000756 | 525 | `nxNpActorUpdateCMassQuaternion` (NpActorDynamicMath.h), now `__declspec(noinline)`, calling `nxNpActorBodyQuaternionFromMatrix` | CMass 2, the oracle's count |

000768 was re-walked in full (rotation spills, centre, the nine R F sums, the quaternion arms, the call
arguments to 000746). 000746 and 000756 were walked against their listings (0x16e80-0x16f74,
0x17420-0x1762c).

**Defects fixed:**
- 000845: selector 1 stores the transverse term at +0x00 (0x1c836) before its branch; the comment that said
  the image leaves the word unwritten is corrected.
- 000768/000801/000756: the four roots of `nxNpActorBodyQuaternionFromMatrix` are now fsqrt through the
  X87Sqrt.h helpers in 000768's operand order (0x18216/0x182a8/0x182f3/0x18339): trace `x87FsqrtSum4`, z and
  y arms `x87FsqrtDiag`, x arm `x87FsqrtSum3` on the float spill. The helpers are naked asm, so they execute
  fsqrt in the SSE2 units NpActor.cpp and Scene.cpp too; under 0x027f the result equals the CRT's, and every
  Phase 5 target stayed at stdout_delta=0. 000756's candidate now uses this conversion (it used
  `nxNpActorQuaternionFromMatrix`, which grouped the trace (m00 + m11) + m22). `nxNpActorQuaternionFromMatrix`
  keeps CRT sqrt; its only user is now 000789, which is left for its rewrite.
- Allocator: 000977 and 000987 allocate the collision object from `nxFoundationSDKAllocator` ([0x101041bc],
  0x218f1/0x21a8e); 000979 frees `this` through it (0x21971). To keep every block on one allocator, the
  collision object's deleting destructor (`CollisionObject::nxScalarDeletingDtor`, row 001079, 0x235f4) and
  the sphere, plane and mesh constructors' collision-object allocations moved too (their oracle rows use
  [0x101041bc] at 0x277e4, 0x24eea and 0x27dde). The kinematic block at +0x118 (000785/000787,
  `nxNpActorTransitionKinematic`) is allocated and freed through it (0x1995d, 0x19cda).
- 000979: the vptr restores at 0x2194b (BOX table) and 0x21951 (facade table) are written.
- 000977: stores the facade table at +0xe0 (0x21895). The facade table is new
  (`nxBoxHullFacadeVtable`, ObjectModel.cpp), dumped from .rdata 0x10106a88: slot 0 000985, 1 000953,
  2 000955, 3 000961, 4 000963, 5 000965, 6-8 000967/000969/000971, 9 000957 and 10 000959 (null: not
  written), 11 000975.
- 000953 and 000961 are real slot functions (`BoxHullFacade::vertexCount`/`faceCount`).
- 000975 has the row's ABI: six stack arguments, `ret 0x18` (unread, min, max, direction, pose, unread). It
  sits in a table, so its convention matters; the layout harness call was updated to it.
- 000967/000969/000971: the tables hold all 24 dwords of the image (they held 12).
- 000937/000939: the root is `x87FsqrtDot3` in the listing order, not CRT sqrt.
- 000847: only the argument's low byte is tested (0x1c888).
- The `phys_fn_000730-equivalent` comment line (ObjectModel.cpp) is reworded, and 18 other comment lines that
  began `// phys_fn_` for rows of this block without the stable-ID form now begin `// Row` (ContactGeneration.cpp
  and .h, NarrowPhase.h, NpActor.cpp, ObjectModel.cpp, Scene.cpp, core/JointSupport.h).

**State mismatches resolved (rows stay `reconstructed`; stale proofs replaced by static proofs):**
- 000738: written as `Row000738Fixture::row000738` (core/JointSupport.cpp), the row's ABI; step-only.
- 000841: written as `MassFrame::nxMassFrameTranslateToCentre` (ObjectModel.cpp); its only caller 000008 is
  not reproduced.
- 000925: written as `HullScratchElement::HullScratchElement` (ObjectModel.cpp); its only user 001472 is not
  reproduced.

The old dynamic proofs of these three drove only the oracle (or a harness composition) and are removed;
the Task 2 trace confirms nothing in the product calls them yet.
- 000713: source and implementation set (`nxBodyRecordFixRoot`, ObjectModel.cpp).
- 000803-000825: implementation corrected to MassProperties.cpp, where the stable-ID lines now are.
- 000981: demoted to `discovered` (phase 5 ledger reason `not_reconstructed_in_phase`). It still omits the call
  to 000973 (0x219b5) and drops the bool return; 000973 is the Task 4 hull batch.

**Calling conventions (rule c).** Decided per row by whether anything reaches the row other than a direct call:

| Row | Row ABI | Candidate | Callers | Decision |
|---|---|---|---|---|
| 000713 | thiscall | cdecl | direct only (000655, 000718, 000724, 000744, 000776, 003979) | code shape; recorded |
| 000744 | ecx, plain ret | cdecl | direct only (000062) | code shape; recorded |
| 000756 | ecx, plain ret | cdecl | direct only (000204, 000208, 000772) | code shape; recorded |
| 000768 | thiscall | cdecl | direct only (000164, 000196-000222, 000793, 000795) | code shape; recorded |
| 000849, 000851, 000853 | thiscall on the destination | shape member, destination first | direct only (000947, 001371, 001008) | code shape; recorded |
| 000943 | thiscall ret 0x10 | cdecl | direct only (001881, 001883) | code shape; recorded |
| 000975 | thiscall ret 0x18 | was a four-argument member | facade table slot 11 | **fixed** |
| 000738 | ecx, plain ret | thiscall member, no arguments | (001303) | same ABI |

**Deferred, with the reason:**
- The sibling shape deleting destructors -- plane 001263, sphere 001375, capsule 001014 and mesh 001399
  (ObjectModel.cpp, the `nx*ScalarDeletingDtor` members) -- still free `this` through the SDK allocator where
  the oracle uses [0x101041bc] (0x22607, 0x27c57, 0x28eb4, 0x25447). Since Task 2 that is inconsistent with the
  box (000979), which frees through the Foundation allocator. It is part of the candidate-wide scene/actor/shape
  allocator gap. An earlier session left an unverified branch, `claude/scene-allocator-unverified`, that routes
  these; it has not been reviewed or merged.
- Latent mismatch: `nxShapeFactory` (Scene.cpp) allocates the 0x228 box and its +0x9c helper through the SDK
  allocator but installs the BOX table, whose slot 0 (000979) now frees through the Foundation allocator. It is
  not live today -- actor release frees shapes and helpers directly through the SDK allocator (Scene.cpp,
  `releaseActor`) and never calls slot 0 -- but the pair must be matched when the factory or the allocator gap
  is fixed.
- The dirty-mark vector growth inside 000782, 000784, 000789 and 000793/000795 (`nxNpActorMarkRecordDirty`,
  NpActor.cpp) stays on `nxGetSdkAllocator()`. The vector belongs to the Scene auxiliary manager: it is
  allocated and freed in Scene.cpp by rows outside this block, so moving only the growth would free
  Foundation blocks through the SDK allocator. The whole vector lifetime has to move in one change.
- 000923 stays `discovered`: its candidate is vendored ICE code (IceAABB.h `SetCenterExtents`, the ICF-folded
  copy), which the vendored-correspondence method promotes, and its oracle callers (000973, 005143) are not
  reproduced, so no trace reaches it.
- Remaining defects in claimed rows, unchanged by Task 2: 000829 (float spills), 000833 (the `x*0.0` addends),
  000849 (inline formula for the 000833 call), 000867 (v rounding), 000873 (stream growth), 000935
  (association), 000989 (NaN report, bool return), 000995 (slot-6 tail jump). 000742's live copy is the NpActor
  session's (handover).
- Partial rows left for Task 4: 000776/000799, 000782, 000784, 000785/000787 (island step, inverses), 000789,
  000791, 000793/000795/000797, 000801 (registration), 000933, 000951, 000981, 000983, 000993.

**Claims.** 63 stable-ID lines for rows of this block, all in the exact form, none duplicated, every RVA and
size equal to the inventory's: ObjectModel.cpp (41), MassProperties.cpp (12), NpActorDynamicMath.h (3),
core/JointSupport.cpp (6: 000712, 000738, 000754, 000758, 000760, 000778), NarrowPhase.cpp (1: 000943).

**Harness changes (no registered line edited).** NxPhysicsShapeVtableTests: the oracle's [0x101041bc] holder and
the candidate's `nxFoundationSDKAllocator` are the same word in that process (one NxFoundation.dll), so the
candidate's collision-object frees had started to count in `oracleFreeCount` and moved the oracle digest.
Every free comparison now takes the oracle's count before the candidate runs and counts the candidate's share of
the holder plus its SDK bridge; the registered line `shape vtable oracle_digest=ed1294b6 cases=626 failures=0`
is reproduced. NxPhysicsObjectLayoutTests: the one `supportBounds` call takes the six-argument form; its
transcript, including `layout candidate mismatches=1 mode=differential candidate_fold=4492c8c1`, is unchanged.

## Task 3 results

Task 3 wrote the public scene raycast API and everything it runs through, drove it with a new staged-pair
target, and promoted every row it wrote. Traces of both sides over that target:
`evidence/scene-raycast-trace-task3.txt` (38 candidate breakpoints plus context functions, candidate
NxPhysics.dll sha256 eeb12a6652488a47... after the review fixes; every claimed row hit exactly as often as the oracle row, and the
two ordered hit sequences identical, 19,113 hits each).

**Written, claimed and promoted to `reconstructed` (40 rows, 8,607 B), all with dynamic evidence:**

| Rows | B | Candidate | Hits (both sides) |
|---|---:|---|---|
| 000366, 000368, 000370, 000372, 000374, 000376 | 1,163 | `NpScene::raycast*` (NpScene.cpp): write lock, maxDist > 0, the Scene row, unlock; 000374 passes FLT_MAX and 0xffffffff | 203-211 each |
| 000690, 000692, 000694, 000696, 000704, 000706 | 1,311 | `NxSceneInternal::raycast*` (new SceneRaycast.cpp): unit-direction test, collector reset, mask, 004864 | 201-209 each |
| 000680, 000682, 000684+000686, 000688, 000698, 000700+000702 | 1,625 | the loop helpers, static and kept out of line (SceneRaycast.cpp) | 198-206 each |
| 000949 | 663 | `BoxShape::nxBoxRaycast` (ObjectModel.cpp), rewritten from the listing | 315 |
| 005481, 005483, 005485 | 1,350 | `PruningPool::Resize/AddObject/RemoveObject` (new opcode/IcePruner.cpp) | 17 each |
| 005208, 005210 | 29 | the base pruner's slots 1 and 2 | 7 each |
| 005232, 005218, 005220, 005222, 005214 | 285 | the static pruner's constructor and slots 1-4 | 1, 10, 10, 11, 1 |
| 005216, 005225, 005227 | 528 | the static pruner's tree build, touched report and slot 6 | 4, 1,054, 1,054 |
| 005464, 005468 | 521 | the dynamic pruner's constructor and slot 6 | 1, 1,054 |
| 005541, 005543 | 864 | `nxSegmentAABB`, `nxRayAABB` | 1,500, 4,740 |
| 004857, 004859 | 268 | the engine's add and remove (new opcode/IcePruningEngine.cpp) | 17 each |

The split rows 000684+000686 and 000700+000702 are one function each, as the image's are; both rows of a pair
carry their stable-ID line on it. The loop rows take their arguments in registers in the image and are
called directly by one row each, so the candidate's ordinary static functions are a code-shape difference.
Ledgers: phase 7 (20 rows), phase 4 (17), phase 5 (000949), phase 3 (004859) and phase 2 (004857, from
`homeless_shared_code`) move to `reconstructed_not_falsified`; the phase 5 note's counts follow (121/84).

**The test target.** `NxPhysicsSceneRaycastTests` (tests/PhysicsSceneRaycastTests.cpp, Phase 7, 0xcd-filled
allocations): ten single-shape actors (static box, sphere, capsule, plane, a raycast-disabled box and a
rotated box; dynamic box, sphere, capsule and rotated box) in distinct collision groups, a static and a
dynamic compound (box + sphere); all six raycasts for 15 rays (hits, misses, rays inside a box, sphere and
capsule, from below the plane, the disabled box) under each shapes type; group masks; finite maximum
distances (both pruners' segment paths, limits inside a box and on a face); five hint-flag sets; reports that
stop after one and two hits; the error paths (non-unit, zero and just-off-unit directions, maxDist 0 and -1);
then after moving a dynamic actor (setGlobalPosition) and a static shape (setLocalPosition, onto a ray only its
new place meets), after releasing three actors (a static, a dynamic, the static compound) and after adding a
static actor to a built tree. Every onHit is printed as the words its flags declare. 1,815 transcript lines (1,810 of them `raycast` lines),
equal on both pairs. 207 oracle-sourced lines are registered; the Phase 7 floor goes from 276 to 483 (gate_targets.ps1,
test_gate_targets.py; the Phase 7 target list in test_gate_commands.py). No triangle mesh: the candidate's
`NpPhysicsSDK::createTriangleMesh` (000242 -> 000478) still returns 0.

**Defects the differential found, all fixed at the source:**
- 000949 (BOX slot 5) differed in the rotated box's impact words: the float transcription rounded and grouped
  where the listing keeps dx and the first row's dz in registers and associates each row of the local
  direction differently. Rewritten from the listing, which also fixes the normal sign for a hit coordinate
  of exactly zero (0x10020a9b: -1, the candidate gave +1).
- Every finite maxDist missed the static shapes: the vendored `RayCollider::_SegmentStab(const AABBTreeNode*,
  Container&)` (004919) used stock OPCODE's inline SegmentAABBOverlap, whose `float f` overflows to +inf
  against the static tree's root box (the plane's +-1.7e38 box) and rejects the root. The image keeps those
  values in x87 registers. External/opcode/novodex/OPC_RayCollider.cpp now writes that site's test out with the
  listing's lifetimes (build parity: the file's comment [2] and MODIFICATIONS.md's paragraph on the segment
  test); the other stabs keep the stock form, whose inlined lifetimes differ per site in the image. 004919 was
  promoted by the vendored-correspondence plan on the stock body: its proofs are amended (appended, history
  kept) with the listing walk of 0x000b8355-0x000b843d and this trace (745 hits each side);
  opcode_review.csv and vendored_coverage.csv carry a note; the vendored match outputs were regenerated (no
  class changed; vendored-correspondence.md, "Later change").
- A static shape moved after the tree was built was missed at its new place: the shapes' owner update
  (001315, ObjectModel.cpp `ShapeBase::nxApplyOwnerUpdate`, not a row of this block) incremented the pruner's
  stamp where the image calls the pruner's slot 3 (0x10026a92-0x10026ab5), which for the static pruner drops
  the tree (005222). It now calls slot 3 through the pruner's table. Reverting that line makes 21 transcript
  lines differ (the x_moved_box queries).

**What the candidate's scene was missing for the raycasts (Scene.cpp, ObjectModel.cpp; not rows of this
block, emulations brought to the oracle's observable state):**
- The shape factory never constructed a shape's Prunable (+0xa4): no owner, no invalid handle, and the owner
  hooks (`gPrunableOwnerWorldAABB`) never installed, so no world box could be computed.
  `nxShapeFactoryInstallPrunable` (ObjectModel.cpp) does what ShapeBase's constructor does for it, for
  factory shapes and group shapes.
- The pruners were data-only emulations without tables. The static pool stopped at four objects (a fifth
  static shape was never registered), the dynamic pool's section counts were never written (so nothing was
  visible to a query), and compounds were registered in an emulated order (static compounds: only the group).
  Registration now goes through the reconstructed engine (004857/004859): single shapes in section 1, a
  compound's children in section 0 and its group in section 2, and a shape added at runtime moves the original
  to section 0 with the new child and joins the group to section 2 (nxActorAppendShape). All four layouts were
  checked word by word against the pinned DLL's pools, with a measurement probe that is not committed (a
  temporary `NX_SCENE_RAYCAST_PROBE` build of the harness that dumped Scene+0x624, the four pruners, each
  pooled prunable's shape words +0x9c..+0xe3 and the world boxes, after creation, a runtime shape add and
  remove, and release); releasing an added shape leaves its pool entry in place, as the pinned DLL does. The
  claim rests on that uncommitted probe. Its non-pool differences are open items (below).
- 000503's tail was missing: the three containers at Scene+0x50, +0x500 (the queries' result collector) and
  +0x510 now borrow the shared buffer, and the engine loop (004861) hands it to every pruner through slot 4,
  where the static pruner's touched container borrows it (005214).
- Scene release destroys the pruners through their destructors (the static tree, then the pool's arrays).

**Precision.** The rows run only at API time, under 0x027f. Register lifetimes are `double`, the listing's
spills `float`, and the roots go through X87Sqrt.h. SceneRaycast.cpp and IcePruningEngine.cpp keep the default
architecture (SSE2 double arithmetic is the x87's at 53 bits there). IcePruner.cpp is built /arch:IA32 /GR-, as
NxOpcode is (review fix): it instantiates OPCODE's AABBTreeOfAABBsBuilder, and the linker keeps this object's
COMDAT copies of the builder's inline virtuals and table ahead of NxOpcode's; built SSE2, the kept
`AABBTreeBuilder::GetSplittingValue` (002150) rounded the centre to float where the x87 copy returns it
unrounded (the matcher showed it). The three files are built /EHs-c- like the joint files: the image's tree
build is frameless, where /EHsc gave `new AABBTree` an unwind frame.

**Not reconstructed here (not claimed):**
- The base pruner's constructor (0x000f1550) and the destructors: its +0x34 member registers the pruner with a
  process-wide object (0x000b4cc0); the engine's factory creates that object through Scene.cpp's existing
  emulation (`nxOpcodeEnsurePool`) and the handle stays 0 (the pinned DLL shows 0 and 1).
- The engine's factory (0x000b5090) for types 1 and 3, and the pruners' slots 5, 7 and 8 (the base's `false`).
- 005212 and 005242 (the base pruner's slots 3 and 4) are recorded elsewhere; the class carries their bodies.

**Open, with the reason:**
- The probe's non-pool differences (candidate vs pinned DLL), none read by the raycasts: the engine's +0x30
  word (Scene+0x654; the image 3, the candidate 0); Scene+0x6a0 (the engine dump's +0x7c, the tracked-shape count
  nxSceneTrackShape keeps) differs by 2; shape +0xdc bit 2 (the owner update's dirty flag) is not set on factory shapes; shape +0xe0
  is not the box facade table 0x10106a88 (the product's shape factory does not run the BOX constructor 000977
  that installs it; see Task 2's facade note); shape +0xa0 (an object the image's shapes carry, 0 on
  factory shapes); the pruners' +0x34 handle (the image 0 and 1, the candidate 0).
- The owner update 001315 still skips the image's dirty-list push through the shape's +0xa0 object
  (0x10026a50-0x10026a76): the candidate's factory shapes have no +0xa0 object. Not a row of this block.
- The engine's bounds (Scene+0x628..+0x63c) are never set by the candidate, so the pruners' +0x1c bounds stay
  empty, as they are in the pinned DLL's scenes the test builds.
- The bundle for `gap:opcode\OPC_Model.cpp..<end>` (the dynamic pruner and pool rows) cannot be written on
  Windows (`<` in the file name); its rows are documented here and in IcePruner.cpp.

**Harness adaptations (no registered line edited).** NxPhysicsThirdPartyTests: its NxCandidatePruner fixture
overrides slot 2 as `bool RemoveObject`, sets the boxes through `mPool.mWorldBoxes` (+0x14, unchanged), and
hands its own box array back before destroying the fixture, because the pool frees its arrays in the pruner's
destructor (0x000efed0); the `Pruner.mWorldBoxes` layout check reads the same offset. NxPhysicsInternalTests
and NxPhysicsCollisionTests link the new files.

**Claims.** 40 stable-ID lines, exact form, none duplicated, RVA and size equal to the inventory's:
NpScene.cpp (6), SceneRaycast.cpp (14), ObjectModel.cpp (1: 000949, replacing the "Provisional" line),
opcode/IcePruner.cpp (17), opcode/IcePruningEngine.cpp (2). The Ghidra supplement was rerun for the three
written rows with no decompile (000686, 005208, 005214), requesting the union with the 63 existing RVAs (66,
all `ok`; the 63 are byte-identical). Bundles regenerated: SceneRaycast.cpp,
gap:Scene.cpp..SceneRaycast.cpp (new), gap:SceneRaycast.cpp..CapsuleShape.cpp and
gap:opcode\IcePrunable.cpp..opcode\OPC_MeshInterface.cpp.
