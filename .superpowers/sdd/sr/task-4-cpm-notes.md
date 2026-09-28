# Task 4, contact-pair manager sub-area: source notes

Branch `claude/sr-t4-cpm` (from `claude/scene-raycast-block` bb2e485). Source only: no inventory, ledger,
contract, gate_targets or timing edits (the integrator does records).

## Files

- `Physics/src/ContactPairManager.cpp` (new): every row below, the shared helpers, the open-callee holes.
- `Physics/src/include/ContactPairManager.h` (new): the raw-offset objects (NxFrictionPatch 0x78,
  NxActorPair 0xec, NxPairNode 0x108, NxPairList, CpmPairHash, CpmFrictionParams, CpmRestitution,
  CpmBufferedContact) and the row declarations.
- `CMakeLists.txt`: ContactPairManager.cpp added to the `/arch:IA32` list with its reason (step rows, 0x0f7f).
- `Physics/src/PhysicsSDK.cpp` + `Physics/src/include/PhysicsSDK.h`: two accessors, `nxSdkParameterTable()`
  (the live parameter array .data 0x10123b18) and `nxSdkGroupCollisionMaskTable()` (.data 0x10123a98).
  The rows load these arrays directly (`fld [0x10123b2c]`, `mov eax,[eax*4+0x10123a98]`); the arrays are
  file-static in PhysicsSDK.cpp, so the accessors are the minimal way to read the same storage. Not rows.

Build: `cmake -S . -B build -A Win32; cmake --build build --config Release --target NxPhysics` -> OK,
ContactPairManager.cpp compiles with no warnings. The linker keeps every row (`/OPT:NOREF` is already set
for NxPhysics; the map lists all of them). Stable-ID check: 32 lines in ContactPairManager.cpp, all
fullmatch the pattern, RVA/size equal inventory.json, no duplicates across Physics/src. 32 rows,
13,082 bytes.

## Vtable reachability (the NxPhysicsJointSlotTests question)

No viable public-creation path. The only vtable-bearing object here is the friction patch (vtable
0x10106944: 004248, 001583, 005242, 000867). Patches are created only by 000893 (the ActorPair
constructor, inline patch) and 000895 (vector patches); the ActorPair only by 000901 <- 000911, whose
callers are 001971/001976 (broadphase new-pair, inside the step). ActorPair and the pair node have no
vtable. So no object carrying 0x10106944 is ever created by a public API while NpScene::simulate is a stub,
on either DLL. The only way to drive slot 3 would be a harness that builds a fake patch and writes the
oracle's base+0x106944 / the candidate's `??_7NxFrictionPatch@@6B@` (map symbol) as its vptr: that is an
address-resolved internal call, not the joint-slot pattern (public creation, then the object's own vtable),
and slot 3 is 000867 (another sub-area's row) anyway. Not recommended.

Non-step paths in the oracle that the candidate does not run:
- 000913: NxPhysicsSDK::releaseScene -> Scene vtable 0x101066f4 slot 0 (000668) -> 000663 -> 000913. With no
  step the Scene's report hash is empty, so the oracle runs the row's prologue and returns at 0x1001fec0.
  The candidate's Scene delete (Scene.cpp nxSceneDelete) does not call it. A trace would show the oracle
  hit / candidate not hit; wiring it is the Scene dtor owner's call (it needs the hash, 004153/004157).
- 000915: 001953 (Scene dtor) and 001955 (<- 001323, shape/actor scalar deleting destructors). Neither
  001953 nor 001955 exists in the candidate; with no step no pair node exists, so even the oracle only
  reaches it with a null/absent node list.
- 000903, 000887/000889: only from 000915.
- 000881: only from 000913/000917.

## Open callees (REPRODUCTION HOLES in ContactPairManager.cpp, named cpmOpenNNNNNN, no stable-ID line)

- 002348 (0x5ab80, 719 B): narrow-phase pair dispatcher, thiscall on [0x10123c18] (PhysicsSDK.cpp
  gShapePairFunctionTable), args (shape0, shape1, pair, scene). Called by 000905.
- 002354 (0x5b620, 86 B): stream sub-object reset, thiscall on pair+0x10. Called by 000863, 000887.
- 002356 (0x5b680, 22 B): stream sub-object constructor, thiscall on pair+0x10. Called by 000893.
- 004153 / 004155 / 004157 (0x9a570 / 0x9a610 / 0x9a920): pair-keyed hash find / insert / erase, thiscall.
  Called by 000905 (Scene+0x2c), 000913/000917 (the caller's hash; find on .data 0x10123c28).
- .data 0x10123c28, the SDK's actor-group pair-flags hash object: absent in the candidate
  (NpPhysicsSDK::setActorGroupPairFlags is blocked on it); the holes receive a null pointer.

Existing candidate callees used: 000598 NxSceneInternal::growJointRecords (Scene.cpp); 004389/004391/004393
JointSupportRecord members (core/JointSupport.cpp); 002352 nxContainerAddThunk (ObjectModel.cpp); 004840
SdkContainer::resize (Containers.cpp); 000867 nxAccumulateByKind0867 (ObjectModel.cpp, via the patch's
slot 3); NxNormalToTangents (Foundation import, as [0x1010418c]); x87FsqrtDot3 (X87Sqrt.h, existing helper).

Wiring: none. No existing candidate code calls any of these rows (grep: no caller of 000750..000921 in
Physics/src), and the step is a stub.

## Conventions applied (all rows)

- x87: register lifetimes `double`, spills NxReal, the listing's grouping and operand order per
  expression; `fst` sites keep the unrounded value for the next use (e.g. 000865's dz, 000861's dynamic
  product compared before rounding). Integer-move copies in the listing are integer copies here
  (cpmCopyWords), never float loads.
- Allocator: every allocation/free is `[0x101041bc]` = nxFoundationSDKAllocator (malloc slot 8 with type 0,
  free slot 0x14); malloc/free paired on it. No row calls 004803.
- The shared inline sequences are written once as helpers and used at each site: the material lookup
  (cpmMaterial), the combine mode (cpmCombine/cpmMaxMode), the record-kind bits (cpmRecordKind), the 004391
  tail (cpmSolveRecord), the record take/grow (cpmTakeRecord), the two body-local transforms (0865 and 0891
  operand orders differ, so two helpers), 000895's direction transform.
- /GS: row000879 and cpmBufferContactReports0917 carry a stack cookie (local arrays); the oracle has none.
  Same accepted difference as the joint rows with local buffers.

## Per row

Format: candidate symbol (file) | listing range walked | ABI | reproduced | divergence/open | static_proof.

### 000750 + 000752
- `CpmJointedBody::row000750` (ContactPairManager.cpp). Listing 0x16fb0-0x17004 (000752 is the second loop,
  entered by `jmp` at 0x10016fdb). thiscall(body, other), `ret 4`, returns eax 0/1.
- Walks body+0x1d8 (link +0x34) for joint +8/+0xc == other, then other's list for `this`; found:
  `(~(flags >> 8)) & 1` of joint+0x2c; none: 0. No divergence.
- static_proof: "transcribed from the Capstone listing (0x16fb0-0x17004, 000752 being the continuation entered
  by the jmp at 0x10016fdb) in scene-raycast Task 4: thiscall(body, other), ret 4; both joint lists (+0x1d8,
  link +0x34) searched for a joint naming the other body at +8/+0xc, result !(joint+0x2c bit 8), else 0.
  Candidate CpmJointedBody::row000750 (Physics/src/ContactPairManager.cpp), one function for both rows.
  Reachable only from the simulation step (000905 <- 000909 <- 001976 <- 000608 <- 000655); no public path
  while NpScene::simulate is a stub."

### 000855
- `cpmCombine0855` | 0x1ca00-0x1ca4b | stdcall(float a, float b, mode), `ret 0xc`, double in st(0) |
  mode 0 (a+b)*0.5f, 1 min (a<b?a:b), 2 a*b, else max (a<b?b:a); NaN picks as the `test ah,5; jp` tests.
- static_proof: "transcribed from the Capstone listing (0x1ca00-0x1ca4b) in scene-raycast Task 4: stdcall,
  ret 0xc, NxCombineMode average/min/multiply/max with the result left unrounded in st(0). Candidate
  cpmCombine0855 (Physics/src/ContactPairManager.cpp), __stdcall returning double. Reachable only from the
  simulation step (000861 <- 000879 <- 000897 <- 000728); no public path while NpScene::simulate is a stub."

### 000857
- `cpmRestitution0857` | 0x1ca50-0x1cb21 | stdcall(ids, out), `ret 8` | materials lo16 (A) / hi16 (B) via the
  inlined getMaterial (count by the 0x38e38e39 reciprocal, out-of-range -> material 0); combine inlined with
  max(restitutionCombineMode) (signed); out[1] = (A|B flags) & 4.
- SDK read through PhysicsSDK::instance +0x28/+0x2c as the listing does.
- static_proof: "... (0x1ca50-0x1cb21) ...: stdcall, ret 8; restitution of materials lo16/hi16 combined with
  the larger restitution combine mode (the combine inlined), out[1] = (flagsA|flagsB)&4. Candidate
  cpmRestitution0857 (...). Reachable only from the simulation step (000897 <- 000728); ..."

### 000859
- `cpmAnisotropicFriction0859` | 0x1cb30-0x1cd8f | stdcall(shape, mat, other, nf, out), `ret 0x14` | as the
  contract; other's averages (dynamic kept in st(0), static spilled); anisotropy dir = shape rot (+0xc) *
  dirOfAnisotropy with the listing's grouping (y, z, x order); dyn/dynV scale*combine, clamped; static
  scale*combine spilled, staticV combine*scale kept; raised to dyn/dynV; times nf into +8/+4.
- Divergence: NX_DYN/STA_FRICT_SCALING read through nxSdkParameterTable() (same array).
- static_proof: "... (0x1cb30-0x1cd8f) ...: stdcall, ret 0x14 ... Candidate cpmAnisotropicFriction0859 ...
  Reachable only from the simulation step (000861 <- 000879); ..."

### 000861
- `cpmFrictionParams0861` | 0x1cd90-0x1d0a4 | stdcall(shapes, ids, n, nf, out, t1, t2), `ret 0x1c` |
  both paths as the contract; the aniso choice tests the whole flags words (`test ebx,ebx; sete`) and
  |dyn - dynV|; t1 = dir x n / |.| when |.| > 0.01f, t2 = n x t1; fallback NxNormalToTangents + averages.
- Divergence: the root goes through x87FsqrtDot3 (cz passed as a qword: 53-bit under 0x0f7f, the accepted
  X87Sqrt.h convention).
- static_proof: "... (0x1cd90-0x1d0a4) ...: stdcall, ret 0x1c ... Candidate cpmFrictionParams0861 ...
  Reachable only from the simulation step (000879 <- 000897); ..."

### 000863
- `cpmActorPairResetStream0863` | 0x1d0b0-0x1d0b7 | fastcall(pair) | 002354 on pair+0x10 (OPEN: 002354 hole).
- static_proof: "... (0x1d0b0-0x1d0b7) ...: `add ecx,0x10; jmp 0x1005b620`. Candidate
  cpmActorPairResetStream0863 (__fastcall) ... 002354 is not in the candidate (reproduction hole).
  Reachable only from the simulation step (000905 <- 000909 <- 001976); ..."

### 000865
- `NxActorPair::row000865` | 0x1d0c0-0x1d256 | thiscall(patch, points, count byte), `ret 0xc` | count to
  +0x74, loop bound reloaded; dy stored, dz `fst` (unrounded dz feeds y only); raw copy for a null body;
  +0x75 = 0.
- static_proof: "... (0x1d0c0-0x1d256) ...: thiscall on the pair, ret 0xc ... Candidate
  NxActorPair::row000865 ... Reachable only from the simulation step (000897); ..."

### 000871
- `cpmPatchSpring0871` | 0x1d590-0x1d609 | cdecl(patch) | (mat[+0x68].flags | mat[+0x6a].flags) & 4.
- static_proof: "... (0x1d590-0x1d609) ...: cdecl ... Candidate cpmPatchSpring0871 ... Reachable only from
  the simulation step (000891, 000895 <- 000897); ..."

### 000875
- `NxActorPair::row000875` | 0x1d8e0-0x1dc70 | thiscall on the pair (sink), 9 args, `ret 0x24` | swap and
  negate on owner+8 != sink+8; header with flag word (4 for shape+0xde bit 0x20) | valid; normal block on
  raw-word change; contact record with bit 31 for wide features; flag 1 -> fid1<<16|fid0; flag 4 -> two
  words (wide) or f1<<16|f0. Every append grows through 004840 (SdkContainer::resize on pair+0x38) with the
  listing's two predicates at the eight sites.
- Divergence: none known (unlike the candidate's 000873, the growth path is reproduced).
- static_proof: "... (0x1d8e0-0x1dc70) ...: thiscall, ret 0x24 ... Candidate NxActorPair::row000875 ...
  Reachable only from the simulation step (narrow-phase entries 001762, 001779, 001844, 001909, 001927,
  001929 <- 002348); ..."

### 000877
- `NxFrictionPatch::operator=` | 0x1dc80-0x1dd3a | thiscall(src), `ret 4`, returns this | dwords
  +0x04..+0x70 and bytes +0x74/+0x75 by integer moves; vptr untouched.
- static_proof: "... (0x1dc80-0x1dd3a) ...: thiscall, ret 4, returns this; copies +0x04..+0x73 and the two
  bytes at +0x74/+0x75, not the vptr. Candidate NxFrictionPatch::operator= ... Reachable only from the
  simulation step (000891); ..."

### 000879
- `NxActorPair::row000879` | 0x1dd40-0x1e4fc | thiscall(scene, forceScale, errorScale), `ret 0xc` | as the
  header comment: per patch/anchor, world anchors, e1 stored/e2 kept, lever, error, two kind-4 records via
  000598/004391, penalty/0.7 scaling, +0x4c = dyn / dynV; patch +0x58..+0x64 zeroed.
- Divergence: the two records are one loop body (k = 0, 1) in the listing's order; /GS cookie.
- static_proof: "... (0x1dd40-0x1e4fc) ...: thiscall, ret 0xc ... Candidate NxActorPair::row000879 ...
  Reachable only from the simulation step (000897 <- 000728); ..."

### 000881
- `NxActorPair::row000881` | 0x1e500-0x1e5d2 | thiscall(outN, outF), `ret 8` | embedded patch then count-1
  vector patches; nothing written when count is 0.
- static_proof: "... (0x1e500-0x1e5d2) ...: thiscall, ret 8 ... Candidate NxActorPair::row000881 ... Its
  callers 000913 (NxPhysicsSDK::releaseScene -> 000668 -> 000663) and 000917 (the step, 000655) are not
  run by the candidate (000663's call to 000913 is not reproduced, the step is a stub), so no target
  executes it."

### 000883 + 000885
- `NxActorPair::row000883` | 0x1e5e0-0x1e6a6 and 0x1e6b0-0x1e8fe | thiscall(scene, patch, sep, point, normal),
  `ret 0x14`, args 3 and 5 unread | lever arms, swept-capsule search (null deref when none, as the oracle),
  separation along rotation column 1, spring record kind 0, +0x48 = FLT_MAX, 004393(1/(h(kh+d)), kh/(kh+d)).
- static_proof: "... (0x1e5e0-0x1e6a6, 000885 being the body at 0x1e6b0-0x1e8fe entered by the jmp at
  0x1001e6a6) ...: thiscall, ret 0x14 ... Candidate NxActorPair::row000883, one function for both rows ...
  Reachable only from the simulation step (000897); ..."

### 000887 + 000889
- `cpmActorPairRelease0887` | 0x1e910-0x1e937 and 0x1e940-0x1e99e | fastcall(pair) | 002354 (hole), free
  each vector patch (count re-read), count 0, free and zero the vector, tail to 002352 (nxContainerAddThunk).
- static_proof: "... (0x1e910-0x1e937, 000889 = 0x1e940-0x1e99e) ...: fastcall ... Candidate
  cpmActorPairRelease0887, one function for both rows ... 002354 is a reproduction hole. Reached only from
  000903 <- 000915 (pair deletion: 001953 Scene dtor, 001955 <- 001323 shape/actor release), which the
  candidate does not reproduce, and pair nodes exist only after a simulation step; no target executes it."

### 000891
- `NxActorPair::row000891` | 0x1e9b0-0x1ef92 | thiscall(scene), `ret 4` | as the header comment.
- Divergence: the frame slots persist across patches as in the oracle; before the first patch they are 0
  here, stack garbage in the oracle (only read for a patch with count 0).
- static_proof: "... (0x1e9b0-0x1ef92) ...: thiscall, ret 4 ... Candidate NxActorPair::row000891 ...
  Reachable only from the simulation step (000897); ..."

### 000893
- `NxActorPair::row000893` | 0x1efa0-0x1f00a | thiscall(shape0, shape1), `ret 8`, returns this | 002356
  (hole), embedded-patch vptr by placement new (the candidate's NxFrictionPatch vtable, not 0x10106944),
  zeros, 0xff bytes, objects/bodies.
- Divergence: vtable identity (compiler-generated; slot 3 forwards to the cdecl 000867, slots 0-2 are
  fresh empty bodies rather than the shared 004248/001583/005242).
- static_proof: "... (0x1efa0-0x1f00a) ...: thiscall, ret 8 ... Candidate NxActorPair::row000893 ... 002356
  is a reproduction hole. Reachable only from the simulation step (000901 <- 000911 <- 001971/001976); ..."

### 000895
- `NxActorPair::row000895` | 0x1f010-0x1f318 | thiscall(shape0, shape1, normal, ids), `ret 0x10` | as the
  header comment, vector growth 2n+2 on nxFoundationSDKAllocator.
- static_proof: "... (0x1f010-0x1f318) ...: thiscall, ret 0x10 ... Candidate NxActorPair::row000895 ...
  Reachable only from the simulation step (000897); ..."

### 000897 + 000899
- `NxActorPair::row000897` | 0x1f320-0x1f548 and 0x1f550-0x1fa99 | thiscall(scene, forceScale,
  penaltyScale), `ret 0xc` | as the header comment; includes the stale-feature-ids quirk (a -1 feature
  after a real one keeps the last feature word as the patch ids, 0x1001f5b6/0x1001f5e5), the flag-2 marking
  of pairs with one missing body, the anchor selection.
- static_proof: "... (0x1f320-0x1f548, 000899 = 0x1f550-0x1fa99 reached by the jmps at 0x1001f544/0x1f548/
  0x1f94a/0x1f9a8/0x1f9f0/0x1fa4c) ...: thiscall, ret 0xc ... Candidate NxActorPair::row000897, one function
  for both rows ... Reachable only from the simulation step (002400 -> 000659 -> 000655 -> 000611 -> 000730
  -> 000728); no public path while NpScene::simulate is a stub."

### 000901
- `NxPairNode::row000901` | 0x1faa0-0x1fb29 | thiscall(e0, e1, list), `ret 0xc` | tail/head link.
- static_proof: "... (0x1faa0-0x1fb29) ...: thiscall, ret 0xc ... Candidate NxPairNode::row000901 ...
  Reachable only from the simulation step (000911 <- 001971/001976); ..."

### 000903
- `cpmPairNodeUnlink0903` | 0x1fb30-0x1fba3 | fastcall(node) | the three unlink cases, then 000887 on +0x14.
- static_proof: "... (0x1fb30-0x1fba3) ...: fastcall ... Candidate cpmPairNodeUnlink0903 ... Reached only
  from 000915 (pair deletion via 001953/001955), not reproduced in the candidate; no target executes it."

### 000905
- `NxPairNode::row000905` | 0x1fbb0-0x1fd98 | thiscall(scene), `ret 4` | as the header comment.
- Divergence: the `scene+0x2c` null test at 0x1001fc70 (the address of a member) is not written; 002348 and
  004153/004155 are holes.
- static_proof: "... (0x1fbb0-0x1fd98) ...: thiscall, ret 4 ... Candidate NxPairNode::row000905 ... 002348,
  004153 and 004155 are reproduction holes. Reachable only from the simulation step (000909 <- 001976 <-
  000608 <- 000655); ..."

### 000909
- `NxPairList::row000909` | 0x1fdb0-0x1fe10 | thiscall(scene), `ret 4`.
- static_proof: "... (0x1fdb0-0x1fe10) ...: thiscall on the list, ret 4 ... Candidate NxPairList::row000909
  ... Reachable only from the simulation step (001976 <- 000608 <- 000655); ..."

### 000911
- `NxPairList::row000911` | 0x1fe20-0x1feac | thiscall(e0, e1), `ret 8` | order by +0xd4 (unsigned), group
  mask test (`shl edx,cl` -> `1u << (g1 & 31)`), 0x108 allocation, 000901.
- static_proof: "... (0x1fe20-0x1feac) ...: thiscall, ret 8 ... Candidate NxPairList::row000911 ...
  Reachable only from the simulation step (001971/001976 broadphase new pair); ..."

### 000913
- `cpmFireContactReports0913` | 0x1feb0-0x20016 | cdecl(scene, report, hash) | events via
  cpmReportEvents (shared with 000917), NxContactPair, report slot 0, free + 004157.
- Divergence: NxContactPair's inline constructor stores stream = NULL first (unobservable); sums unwritten
  without a node, as the oracle; 004153/004157 and the 0x10123c28 hash are holes/absent.
- static_proof: "... (0x1feb0-0x20016) ...: cdecl ... Candidate cpmFireContactReports0913 ... Its caller
  000663 (the Scene destructor, reached by NxPhysicsSDK::releaseScene via 000668) is reproduced as
  Scene.cpp nxSceneDelete, which does not call it; with no simulation step the oracle's report hash is
  empty, so the oracle row returns at 0x1001fec0. No target executes the candidate row."

### 000915
- `cpmDeletePairNode0915` | 0x20020-0x2003e | stdcall(node), `ret 4`.
- static_proof: "... (0x20020-0x2003e) ...: stdcall, ret 4 ... Candidate cpmDeletePairNode0915 ... Its
  callers 001953 (Scene dtor) and 001955 (<- 001323, shape/actor scalar deleting destructors) are not in
  the candidate, and pair nodes exist only after a simulation step; no target executes it."

### 000917 + 000919 + 000921
- `cpmBufferContactReports0917` | 0x20050-0x20075, 0x20080-0x2025b, 0x20260-0x203b2 | cdecl(scene, hash) |
  the 000913 event logic, 0x2c-byte records appended to Scene+0x60c (grown 2n+2, copied, old freed).
- static_proof: "... (0x20050-0x20075; 000919 = 0x20080-0x2025b, 000921 = 0x20260-0x203b2) ...: cdecl ...
  Candidate cpmBufferContactReports0917, one function for the three rows ... Reachable only from the
  simulation step (000655); ..."

## Concerns for the integrator

- Every row is source-only: `reconstructed` on static proof only if the integrator accepts the holes
  (002348/002354/002356/004153/004155/004157) as open callees; they change nothing on any path the
  candidate can run.
- 000867's owner may want to make 000867 a thiscall member of NxFrictionPatch (slot 3) directly and drop
  the forwarding `accumulate`.
- ContactGeneration.h's NxContactSink and NxActorPair describe the same object from +0; they are not
  merged (ContactGeneration.cpp is another sub-area's). NxActorPair's +0xe8 comment matches.
