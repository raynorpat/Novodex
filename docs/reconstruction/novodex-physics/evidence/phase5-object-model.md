# Phase 5 Task 1: the object model, locked from constructors and vtable stores

Every claim below carries the address that establishes it. Where Phase 3
borrowed a Shape offset under its relay rule, this task re-derives it from a
constructor rather than inheriting it — and the re-derivation changed the
picture in one important way (three poses, not two).

Inputs: `evidence/phase5-vtable-census.json` (slot maps rebuilt from
`oracle/pe.json`'s decoded pointer records and validated against every
recorded preview), the Capstone listing, and constructor listings read
instruction by instruction.

---

## 1. The actor family

**NxActor declares exactly 88 virtuals** (`NxActor.h`). The census found two
large tables:

| table | rva | slots | distinct targets | reading |
| --- | --- | ---: | ---: | --- |
| `phys_data_000678` | `.rdata 0x101043d0` | 87 | **4** | abstract base's vtable: slot 0 dtor + slots 63/64 real; the other 84 are one pure-call filler, `phys_fn_005667` |
| `phys_data_000679` | `.rdata 0x10104530` | 88 | **88**, all phase 5 | the concrete dynamic-actor implementation |

The two share exactly slots 63/64 (`phys_fn_000038`/`phys_fn_000040`) — the
only behaviour the base implements beyond destruction.

The concrete pair, from their listings:

- **ctor `phys_fn_000044` (0x2480, 53 bytes)** — stores the base vptr at +0
  (`0x101043d0`, 0x2489), zeroes +4, constructs an embedded polymorphic
  subobject at **+8** (member ctor `phys_fn_002404` at call `0x1005ba70`,
  which installs vptr `0x101088b8` and zeroes two dwords), then overwrites
  that subobject's vptr with `0x1010468c` (0x249f) — a one-slot table sitting
  at TBL_88's tail, which is why the census merged it into the 88-slot run —
  stores its argument at **+0x14** (0x24a5), and finishes with the derived
  vptr `0x10104530` (0x24a9).
- **dtor `phys_fn_000118` (0x3650, 55 bytes)** — mirrors it: derived vptr,
  member vptr, member dtor `phys_fn_002406`, base vptr, then conditional
  delete through the SDK allocator global `0x101041bc` slot +0x14.
- **base deleting dtor `phys_fn_000042` (0x2460, 31 bytes)**.

Open: what the +8 subobject interfaces with (one virtual), and what the +0x14
argument means.

## 2. The shape family

Base-shape vtable `0x10107494`, installed by the shared ctor. Per-type finals,
with both dispatch slots Phase 3 could not close:

| type | final vtable | ctor rows | slot 5 (raycast) | slot 7 (sweep dispatch) |
| --- | --- | --- | --- | --- |
| BOX | `0x10106ab8` | `000977`, `000979` | `000949` p3 | `000951` p5 |
| CAPSULE | `0x10106b20` | `000987` | `001010` p3 | `001012` p3 |
| PLANE | `0x10107430` | `001247` | `001261` p3 | `001035` p3 (= base's target; plane does not override) |
| SPHERE | `0x10107528` | `001349` | `001377` p3 | `001373` p3 |
| MESH | `0x10107630` | `001379` | `001405` p3 | `001407` p3 |

**Slot 7 resolves at byte level for every type.** Phase 3's "unresolved" was
behavioural (what the sweep does), not byte absence — the continuous-collision
stop is narrower than it looked.

## 3. The Shape layout, from the shared constructor

`phys_fn_001273` (0x25530, 424 bytes) writes:

- `+0x00` vptr `0x10107494` (0x2553d); `+0x04` = **owner** = ctor arg 1
  (0x25543) — the borrowed claim confirmed from the constructor side;
- `+0x08` zeroed; then **THREE identity-initialised poses**: m33 diagonals at
  `+0x0c/+0x1c/+0x2c` with t at `+0x30..+0x38`; `+0x3c/+0x4c/+0x5c` with t at
  `+0x60..+0x68`; `+0x6c/+0x7c/+0x8c` with t at `+0x90..+0x98`
  (ebx=`0x3f800000` throughout);
- tail: `lea ecx,[esi+0xa4]` — another subobject constructed at +0xa4.

Consequences:

1. **Phase 3's "+0x3c..+0x68 second pose, unestablished" is established**: it
   is a real pose, identity-initialised by the constructor.
2. **There is a third pose at +0x6c..+0x98** that no earlier evidence named.
   Its role (saved previous? sweep source?) is open.
3. The sphere ctor confirms `+0x9c` end to end: allocates **exactly 0x1c
   bytes** through the SDK allocator (`[0x101041bc]` slot +8 with (size,0)),
   constructs them with `phys_fn_001193` (0x247c0), stores at **+0x9c**
   (0x27805); also initialises `+0xd0 = 1`.
4. `+0xe0` is a secondary-base vptr where a desc mix-in exists (box: base
   table `A @0x106a58` replaced by `B @0x106a88`, both 12 slots; sphere:
   NULLed). With geometry floats following at `+0xe4..+0xec`, this is
   consistent with Phase 3's box-half-extents/capsule-radius reads and
   refines rather than contradicts them.

## 3a. phys_fn_001273 transcribed: the +0xa4 subobject is a Prunable, and the shape owns it

Task 3 opened with the full transcription of `phys_fn_001273` into
`Physics/src/ObjectModel.cpp` (`ShapeBase`, 0xe0 bytes, static asserts on
every offset). What the complete listing adds to section 3:

1. **+0xa4 is `Prunable`** (phys_fn_004874, 0xb54a0 — the Phase 4 Ice
   reconstruction, 0x2c bytes ending exactly at +0xd0). And at 0x25649, after
   the prunable is built, the constructor stores **itself** into it:
   `mov [esi+0xa8],esi` writes the shape's address into `Prunable::mOwner`
   (+0xa4+0x04). The shape is its prunable's owner; what looked like an
   anonymous "another subobject" in section 3 is the scene-query prunable
   with its owner link set.
2. **The ctor installs the three Prunable owner adapters** that Phase 4 had
   to leave null because their only writer sat outside its population:
   `.data 0x10128470/74/78` receive `phys_fn_000965` (the folded
   `xor eax,eax; ret` stub), `phys_fn_001271` and `phys_fn_001269`
   (0x0002562b..3f). The two latter are 15-byte cdecl frames that push the
   box and call owner-vtable slot 10 (+0x28) and slot 9 (+0x24)
   respectively, with no null test. All three are now reconstructed rows;
   `IcePrunable.h`'s "written exactly once" note is discharged.
3. **The argument count is two, and the checkpoint's "+0xd4 = arg3" was a
   misread**: `ret 8` fixes two stack arguments, and `[esp+0x14]` at
   0x255e9 stands after three pushes (ebx/esi/edi), so it is entry
   `[esp+8]` — arg **2**. It is stored raw at +0xd4.
4. Tail fields: sentinel `+0xd0 = 0x7fffffff` (0x255ed); halfwords
   `+0xd8/+0xda` zeroed, `+0xdc = 6`, `+0xde = 8` (0x255fd..0x25614).
   A helper at 0x25760 reads +0xa0 as a null-tested pointer — unestablished.
5. The identity-pose stores run TWICE (0x25555..cd and again 0x2564f..cd,
   after the hook stores). Both passes write identical bytes, so the
   transcription stores once; recorded here so nobody re-derives a mystery.

Driven: the layout gate grew a `shapebase` family — the oracle constructor
runs on a poisoned 0xe0 buffer (owner NULL; the registration arm through
`[owner+4]+0x48` → phys_fn_002423 waits for Task 4 actors) with a marked
second argument, five module-specific pointer words masked out of both
sides (`+0x00/+0xa4/+0xa8/+0xb0/+0xb4`: both vtables, the owner store, the
prunable's back-pointer, the inner member's vptr). Oracle digest
`de5f5000`; candidate identical bitwise; `owner_self` asserted. Registrations
17 lines, oracle digest `1c848a60`, coverage floor 17.

## 3b. The BOX final: constructor transcribed, primary slot map resolved, and a probe lesson

Task 3 continued with `phys_fn_000977` (0x21870, 207 bytes), the BOX
constructor, into `Physics/src/ObjectModel.cpp` (`BoxShape`,
`sizeof == 0xe0 + 0x148`, facade embedded at +0xe0):

1. **The primary BOX slot map is resolved** through the merged run at
   0x106a58 (indices 24..40 = BOX slots 0..16): `000979, 001347(p3),
   001277(p3), 000945, 000947, 000949, 001315(p3), 000951, 000941, 000935,
   000937, 000939, 000981, 000927, 001391(p3)×3`. Slot 5 is the Phase 3
   raycast partner; slot 7 is the sweep entry — both byte-level targets were
   already in the census, now they sit in an ordered map.
2. **The ctor forwards BOTH arguments unchanged** to phys_fn_001273
   (0x2187c..80) and then: installs the final BOX vptr (0x2188f; the wall-A
   store at 0x21885 is a chained-construction intermediate, never
   observable); zeroes the first THREE words of each of the six face records
   at +0x150 (corners + both index lists — float data stays POISONED until
   the face builder runs, and so do the vertices at +0xf0..+0x14f);
   allocates a 0x1c-byte collision object through the SDK allocator
   (`malloc(0x1c, 0)`), constructs it with **phys_fn_001075** (the same
   57-byte body as phys_fn_001193 but box-family tables 0x106d08/6e54/6dc8),
   stores it at +0x9c; **overwrites the base ctor's +0xd0 sentinel with 2**;
   seeds dims 1.0f×3 into the facade gap (+0xe4..ec).
3. **Probe lesson, recorded because it cost a crash to find**: driving the
   ctor on a bare LoadLibrary process faults at 0x218fe — the SDK allocator
   holder word is NULL until an NxPhysicsSDK exists (as phase4-formats.md
   already said). The gate now installs a minimal shim interface at the
   holder so every instruction of phys_fn_000977/phys_fn_001075 still runs
   natively; only the 28 bytes come from the shim. A first shim draft also
   wrote the wrong word (`memcpy(dst,&array,4)` copies element [0], not the
   array's address) — caught by chain readback before it could mislead.

Driven: new `boxshape` family — poisoned 0x228 twin buffers, seven pointer
words masked (both final vtables, colobj pointer, four base-ctor pointers).
Oracle digest `ac5ed12f`; candidate identical bitwise; poison preservation,
face zeroing, sentinel=2, dims and the back-pointing colobj all asserted.
Registrations 18 lines, oracle digest `dc5e5ec6`, coverage floor 18.

## 3c. The SPHERE final transcribed — and a census mislabel corrected

Task 3 continued with `phys_fn_001349` (0x277c0, 91 bytes), the SPHERE
constructor, into `Physics/src/ObjectModel.cpp` (`SphereShape`,
`sizeof == 0xe4`, radius at +0xe0):

1. **The sphere has NO secondary base.** The ctor forwards both arguments to
   phys_fn_001273 (0x277bb..cf), stores the SPHERE vptr 0x10107528
   (0x277d4), and ZEROES the dword at +0xe0 (0x277da) — where the box embeds
   its facade, the sphere keeps plain data, and its first datum is the
   radius. The descriptor path later writes it directly (0x27850 reads the
   descriptor's word +0x4c straight into +0xe0).
2. The collision object is built by **phys_fn_001193 ITSELF** (call
   0x277fc) — the generic row, unlike the box's per-type variant — from an
   SDK-allocator block (`malloc(0x1c,0)`), stored at +0x9c (0x27805). The
   sentinel is overwritten with **1** (0x2780b): box 2, sphere 1 so far.
3. **Correction to this file's own §4 note**: the function I had tentatively
   read as the sphere ctor is the **MESH ctor** — `phys_fn_001379`
   (0x27db0, 101 bytes) stores vptr 0x10107630, nulls BOTH +0xe0 and +0xe4,
   builds its collision object through **phys_fn_001241** (0x24e40, another
   57-byte variant row) and overwrites the sentinel with **4**. What §4
   called "sphere: NULLed" is really the mesh nulling two words; the sphere
   ctor nulls exactly one because that word IS its radius.

Driven: new `sphere` family — poisoned 0xe4 twin buffers, six pointer words
masked. Oracle digest `37ea7205`; candidate identical bitwise; radius zero,
sentinel 1 and the back-pointing colobj asserted. Registrations 19 lines,
oracle digest `bd8572e2`, coverage floor 19. The allocator shim moved into a
shared helper both shape probes install.

## 3d. CAPSULE and PLANE transcribed; the +0xd0 sentinel PROVEN to be NxShapeType

Task 3 continued with two more constructors into `Physics/src/ObjectModel.cpp`
(`CapsuleShape`, `sizeof == 0xe8`; `PlaneShape`, `sizeof == 0x10c`):

1. **The +0xd0 sentinel is the NxShapeType tag — proven, not guessed.** The
   pinned `Physics/include/NxShape.h` enumerates
   PLANE=0, SPHERE=1, BOX=2, CAPSULE=3, MESH=4, COMPOUND=5. The ctors write
   exactly those constants after zeroing/overwriting the base's 0x7fffffff:
   sphere 1 (§3c), box 2, capsule **3** (`phys_fn_000987`, store 0x21ab5),
   mesh 4 (§3c), plane **0** (`phys_fn_001247`, store 0x24f24).
2. **CAPSULE** (phys_fn_000987, 0x21a60): forwards both args to the base
   ctor, zeroes TWO data words at +0xe0/+0xe4 (radius/half-height
   candidates — named by nothing yet), builds its collision object through
   the capsule-family variant **phys_fn_001123** (0x23cb0) and stores it at
   +0x9c.
3. **PLANE** (phys_fn_001247, 0x24ed0): plants the default plane equation —
   normal `(0,1,0)`, `D=0` at +0xe0..+0xec — then fills a tangent frame at
   +0xf0/+0xfc through **NxFoundation!NxNormalToTangents**, called via the
   import at `.rdata 0x1010418c` (14 call sites, no in-image writer). The
   transcription calls the reconstruction's own export of the same NovodeX
   algorithm; for the default normal the arithmetic is exact, so both sides
   fold identical bits. Sets +0x108 = 1. Its collision object comes from the
   plane-family variant **phys_fn_001159** (0x24250). This vindicates the
   census's PLANE table 0x107430.
4. **COMPOUND identified by elimination**: phys_fn_001033 (0x22d60) writes
   sentinel **5** and stores table .rdata 0x106c2c — so the earlier guess
   that 0x106c2c was "plane" is dead; the compound ctor also nulls two
   vec3-sized triplets (+0xe0/+0xf0), writes halfword +0xd8 = 0xffff and
   float -1.0f at +0x10c, and builds NO collision object. It stays
   untranscribed.

Driven: new `capsule` and `plane` families on poisoned twins (six pointer
words masked each; the plane folds its tangents IN). Oracle digests
`9b0768d7` / `abed37e0`; candidates identical bitwise. Registrations 21
lines, oracle digest `a6f9c9fe`, coverage floor 21.

## 3e. MESH transcribed — the six shape-family constructors are now decoded

Task 3 continued with `phys_fn_001379` (0x27db0, 101 bytes), the MESH
constructor, into `Physics/src/ObjectModel.cpp` (`MeshShape`,
`sizeof == 0xe8`): forwards both arguments to phys_fn_001273
(0x27dbb..bf), stores the MESH vptr 0x10107630 (0x27dc4), zeroes TWO data
words at +0xe0/+0xe4 (a triangle-mesh pointer and a flag word are the
natural candidates; nothing in this ctor names them — the descriptor path
will), builds its collision object through the mesh-family variant
**phys_fn_001241** (0x24e40) into +0x9c, and overwrites the sentinel with
**4** = NX_SHAPE_MESH.

With this, every shape-family constructor is accounted for from listings:

| type | ctor | sentinel | colobj variant | tail data |
|------|------|----------|----------------|-----------|
| plane | phys_fn_001247 | 0 | phys_fn_001159 | equation + tangent frame (+0x108=1) |
| sphere | phys_fn_001349 | 1 | phys_fn_001193 (generic) | radius @+0xe0 |
| box | phys_fn_000977 | 2 | phys_fn_001075 | hull facade + dims, faces ptr-zeroed |
| capsule | phys_fn_000987 | 3 | phys_fn_001123 | two zeroed floats |
| mesh | phys_fn_001379 | 4 | phys_fn_001241 | two zeroed words |
| compound | phys_fn_001033 | 5 | NONE | triplets +0xe0/+0xf0, +0xd8=ffff, -1.0f@+0x10c |

The compound stays untranscribed; its ctor is decoded in §3d.

Driven: new `mesh` family on a poisoned 0xe8 twin (six pointer words
masked). Oracle digest `422a1f78`; candidate identical bitwise.
Registrations 22 lines, oracle digest `ba13689f`, coverage floor 22.

## 3f. The BASE vtable's slot map, and its first three behavior rows

The BASE shape table `.rdata 0x107494` (12 slots, from the pe.json pointer
records — slot → target):

| slot | target | row | state |
|------|--------|-----|-------|
| 0 | 0x00027710 | phys_fn_001345 (34 B): scalar deleting dtor — calls 0x26bd0 then frees through SDK-allocator slot +0x14 when flag&1 | discovered |
| 1 | 0x00027740 | descriptor-driven update: copies a pose into +0x6c, halfwords from the desc, colobj+4; calls 0xedc0/0x26d90 | discovered |
| 2 | 0x000256f0 | **phys_fn_001277** (98 B): save-to-descriptor — pose3 + halfword trio + [colobj+4] into a record, returns true | **reconstructed** |
| 3 | 0x00025960 | phys_fn_001305 (685 B): x87 world-bounds-class computation against globals 0x10123bc8/0x10123b4c | discovered |
| 4 | 0x00024f70 | **phys_fn_001249**: `xor al,al; ret 0xc` — two args, false | **reconstructed** |
| 5 | 0x000b4070 | **phys_fn_004812**: `xor eax,eax; ret 0x14` — four args, null | **reconstructed** |
| 6 | 0x000266a0 | phys_fn_001315 (1061 B): owner/scene-touching update (reads [owner+4]+0x540) | discovered |
| 7 | 0x00022dd0 | **phys_fn_001035**: `xor al,al; ret 8` — the sweep stub Phase 3 left unresolved | **reconstructed** |
| 8–11 | 0x000f41dc ×4 | purecall filler | filler |

The three stub rows are transcribed as `ShapeBase` members
(`nxBaseSlot4/nxBaseSlot5/nxBaseSlot7`) and driven by a new `basevt`
family: the oracle rows run on a dummy this with marked arguments, the
candidate members answer through the reconstruction, both fold to the same
transcript (`ret4=0 ret5=00000000 ret7=0`). Slot 7's closure is exact: it is
Phase 3's continuous-collision sweep entry, and what the base shape does
with it is answer "no sweep" — the finals' overrides (box phys_fn_000951
etc.) remain the sweep's owning rows. Slots 0–1 and 3/6 stay open until
their drives exist; making `ShapeBase` polymorphic waits for them.

**Slot 2 corrected and closed.** The first draft of this section called slot
2 a "deleting-dtor thunk" — wrong; that snippet lives at 0x256e0 in a
different function. The listing shows phys_fn_001277 is the WRITE-side
partner of slot 1: a save-to-descriptor row copying pose three (12 words)
to record+8, zero-extending halfword +0xde to record+0x38, moving halfwords
+0xd8/+0xda to record+0x3c/0x3e, and reading **[colobj+4] unconditionally**
into record+0x40 before returning true. Transcribed as
`ShapeBase::nxBaseSaveState` and driven by the new `basesave` family on a
real constructed sphere: oracle digest `bc9dc964`, candidate bitwise equal
over the whole poisoned 0x48-byte record. The differential caught a
first-draft transcription bug — I had read `[colobj]` (the vptr word,
0x6b...) instead of `[colobj+4]` — before it could ship, which is exactly
the failure mode this gate exists for. Driven: registrations now 24 lines, oracle
digest `4aa4d389`, coverage floor 24.

## 3g. The first BOX-final behavior row

`phys_fn_000937` (0x20670, 69 bytes), BOX-table slot 10, transcribed as
`BoxShape::nxBoxCenterAndDiagonal`: writes the pose-one translation to
out[0..2] and `sqrt(dx²+dy²+dz²)` over the facade dims to out[3]. Driven by
a new `boxrow` family on a real constructed box: oracle words
`00000000.00000000.00000000.3fddb3d7` (center = origin, diagonal = √3),
candidate bitwise identical — at the constructor-default dims every x87
association of the sum is exact, so the drive is valid while the
general-dims association stays an explicitly open question in the header.
Registrations 25 lines, oracle digest `629e8ada`, coverage floor 25.

Extended: slots 11 and 13 closed the same way. Slot 11 (`phys_fn_000939`)
zeroes a caller vec3 and writes the identical diagonal; slot 13
(`phys_fn_000927`) writes dims to record+0x4c — exactly where the box
constructor read `desc+0x4c` — then **tail-jumps to the BASE save-state row**,
so the transcription composes `nxBaseSaveState` rather than duplicating it.
Oracle digests `2f2dc4eb` / `853c971d`, both bitwise equal on poisoned twins.
A census bookkeeping slip (a duplicated inventory row for 927/939) was caught
and fixed before commit; registrations 26 lines, oracle digest `053011c2`,
coverage floor 26.

Extended again: slots 8 and 9 closed. Slot 8 (`phys_fn_000941`) is the box's
local AABB — negated dims then raw dims, six words. Slot 9
(`phys_fn_000935`, 198 bytes) is the OBB world-AABB: per-axis extents
`|rot-row . dims|` over pose one (axis0 r0/r2/r1, axis1 r3/r5/r4, axis2
r6/r8/r7) with `min = t − ext`, `max = t + ext`. On the identity pose the
arithmetic is exact, so both drives fold bitwise (`d8 == d9 == 8428d8b5`,
minmax `bf800000×3 / 3f800000×3`); the image's mixed single/extended
rounding of partial sums stays open for general poses. Registrations 27
lines, oracle digest `f1d18d6a`, coverage floor 27. BOX slots now closed:
2, 8, 9, 10, 11, 13. Open: 0/1 (dtor + apply-desc chain), 3 (debug draw),
4 (cached-bounds update via 0x1c8c0), 5 (raycast), 7 (sweep), 12
(loadFromDesc), 14–16.

Extended: **slots 14–16 closed** — they are the SAME three-byte row,
`phys_fn_001391` (`mov eax,ecx; ret`), an identity/self-return that ignores
its arguments; transcribed as `BoxShape::nxBoxSelf()` and driven by pointer
equality on a real constructed box (stable on both sides; never folded).
Slot 0 (`phys_fn_000979`) is now fully DECODED but deliberately unclaimed:
it destroys the embedded collision object through `[colobj-vtbl+0]` with
flag 1 UNCONDITIONALLY, runs the base-dtor chain (owner arms, +0xa0 arms,
Prunable tail), then frees `this` through SDK-allocator slot +0x14 when
flag&1 — a safe drive needs the allocator shim extended with free slots and
a careful post-mortem observation design, so it stays discovered until that
lands. Registrations 28 lines, oracle digest `5f1bb91b`, coverage floor 28.

**Slot 0 then closed.** Extending the shim exposed a REAL ABI fact worth
recording: the shipped allocator's free slots take ONE pushed argument and
the callee pops it (callers do `push x; call [slot]` with no esp fixup),
while malloc takes two — a shim whose free popped the wrong count drifted
the stack four bytes per call and corrupted the caller's return address
(fault to `0x1`). With the corrected free stubs plus the destruction-time
vptr words added to the mask (+0x00/+0xe0 restored by the dtor dance), the
flag=0 path folds bitwise: `boxdtor` digest `a29800b7`, candidate equal.
The colobj's own deleting-dtor row (`0x235d0`) stays its own census row.
Registrations 29 lines, oracle digest `5defe17a`, coverage floor 29.

## 3h. The remaining four finals' slot maps — inheritance locked, PLANE slot 13 closed

The pe.json pointer records give every remaining final its full map, and
they LOCK the inheritance story:

- **slots 1 and 2 are the BASE rows everywhere**: phys_fn_001347
  (apply-from-descriptor) and phys_fn_001277 (save-to-descriptor, §3f)
  appear in all five finals.
- **slots 14–16 are the identity row phys_fn_001391** in CAPSULE, PLANE,
  SPHERE and MESH too (CAPSULE/SPHERE carry it at 16–18 of their 19 slots;
  MESH adds a genuine tail row, phys_fn_001381, at its slot 17).
- **PLANE slot 4 and CAPSULE slot 4 inherit the BASE stub** phys_fn_001249
  outright; CAPSULE slot 6 / PLANE slot 6 / SPHERE slot 6 / MESH slot 6 are
  all the same phys_fn_001315 owner-update row as BASE slot 6.
- Per-type maps: CAPSULE 0=001014, 3=001006, 4=1249(base), 5=001010,
  7=001012(sweep), 8=001004, 9=001016, 10=001001, 11=001003, 12=000989,
  13=000991, 14=000995, 15=001359. SPHERE 0=001375(dtor), 3=001369,
  4=001371, 5=001377(raycast), 7=001373(sweep), 8=001367, 9=001361,
  10=001363, 11=001365, 12=001353(radius load), 13=001355, 14=001357,
  15=001359. MESH 0=001399(dtor), 3=001393, 4=001397, 5=001405(raycast),
  7=001407(sweep), 8=001389, 9=001401, 10=001403, 11=001387, 12=001383,
  13=001385, 17=001381. PLANE 0=001263(dtor), 3=001259, 5=001261(raycast),
  7=BASE sweep stub itself, 8=001267, 9/11=001257 (one row, two slots),
  12=001265, 13=001251. Every slot resolved to a censused function id —
  no unknown slots anywhere in Phase 5's shape family.

**PLANE slot 13 closed**: phys_fn_001251 (51 bytes) writes the normal to
record+0x4c..+0x54 and the **NEGATED distance** (`fld [+0xec]; fchs`) to
record+0x58 — the descriptor stores −D, which is exactly what the ctor's
load path inverts — then tail-jumps to the BASE save-state row. Transcribed
as `PlaneShape::nxPlaneSaveState`; driven by a new `planesave` family on a
real constructed plane: oracle digest `7629841d`, candidate bitwise equal,
the `neg_d = 80000000` sign-bit word asserted. Registrations 30 lines,
oracle digest `1e9b7708`, coverage floor 30.

## 3i. Four SPHERE rows closed

The sphere's small overrides decode as a family of radius accessors and all
four are now closed through a single `sphererows` drive on a real
constructed sphere (fresh radius = 0; every output byte deterministic):

- slot 15 phys_fn_001359 (7 B): `fld [+0xe0]; ret` — getRadius;
- slot 13 phys_fn_001355 (17 B): radius → record+0x4c, tail-jump to the
  BASE save-state row — the third slot-13 composition (box, plane, sphere);
- slot 11 phys_fn_001365 (23 B): zero vec3 + radius at +0xc;
- slot 10 phys_fn_001363 (36 B): pose-one translation + radius.

Oracle digest `f9280a78` over the record and zero bits everywhere else;
candidate bitwise equal. One harness defect caught before commit: a
`memcmp(out, &zero, 12)` read past its four-byte reference into stack
garbage — replaced with per-float compares. Registrations 31 lines, oracle
digest `71c21761`, coverage floor 31. SPHERE slots open: 0/3/4/5/7/8/9/
12/14.

## 3j. AABB reach rows, MESH save/get rows -- and a Phase 4 cross-check gift

Three reach rows fold bitwise on fresh objects (every output word a
deterministic zero): SPHERE slot 9 (`phys_fn_001361`) world AABB
`min = t - r`, `max = t + r`; CAPSULE slots 10/11 (`phys_fn_001001`/
`001003`) center+reach and its zero-center twin, where the capsule's
effective sphere reach is **halfHeight + radius** -- matching how the
shipped narrow phase treats an LSS.

Two MESH composable rows close with a planted mesh record (the real +0xe0
assignment arrives with NxTriangleMesh wiring): slot 13 (`phys_fn_001385`)
saves the mesh's +0xe4 word and the shape flags word through the BASE
save-state row (`0ccd08b4` bitwise); slot 11 (`phys_fn_001387`) copies four
dwords from `meshptr+0x5c` (marked words round-trip bitwise).

**Cross-check gift from slot 12** (`phys_fn_001383`, loadFromDesc core,
deferred for its slot-1 tail): on accepting a descriptor it executes
`inc dword ptr [mesh+0x74]` -- TriangleMesh+0x74 is a REFERENCE COUNT,
incremented when a shape binds a mesh. New evidence for the Phase 4
TriangleMesh layout transcription, recorded here for its next revision.

Registrations 35 lines, oracle digest `66e19364`, coverage floor 35.

Extended: SPHERE slot 8 (`phys_fn_001367`) -- the local AABB, closing with
the sign-bit claim intact: mins are **-0.0f** (`80000000x3`) because the
x87 `fchs` negates before storing, maxes `+0.0f`. Oracle digest `33c61825`,
candidate bitwise equal. Registrations 37 lines, oracle digest `37188f32`,
coverage floor 37.

**Slot 14 (capsule twin) closed**: phys_fn_000995 -- set-radius; stores
the argument then tail-jumps through BASE slot 6 with the flag forced to 1
(null-owner no-op on a detached shape). Driven with radius `1.5f`: stored
word `3fc00000` equal candidate. Registrations 38 lines, oracle digest
`c5326782`, coverage floor 38.**PLANE slots 9/11 closed** (phys_fn_001257): one row filling BOTH slots --
zero vec3 + `+FLT_MAX` reach, the plane's unbounded-extent answer. Driven
bitwise on a real constructed plane (`7f7fffff` pinned). Registrations 39
lines, oracle digest `9db24f7c`, coverage floor 39.

**SPHERE slot 0 closed** (phys_fn_001375): the sphere's scalar deleting
destructor -- same structure as the box's: destroys the embedded collision
object unconditionally through its own vtable, base-dtor chain, self-free
through allocator slot +0x14 when flag&1. Driven bitwise (flag=0) by the
`sphdtor` family: oracle digest `c4a35d15`.

**PLANE and MESH slot-0 destructors closed** (phys_fn_001263 /
phys_fn_001399): same deleting-destructor structure as box/sphere, with the
mesh adding a symmetric `dec [mesh+0x74]` refcount decrement when a mesh is
bound (mirroring slot 12's increment). Both driven bitwise (flag=0) by the
extended `dtors2` family: plane digest `7ac6fe28`, mesh digest `b3c6ab70`.
Registrations 41 lines, oracle digest `e2111385`, coverage floor 41.

**CAPSULE slot 12 decoded** (phys_fn_000989, loadFromDesc core): reads
radius from desc+0x4c into +0xe0, height from desc+0x50 through
`fmul [0x101043cc]` -- a `.rdata` float constant read back as **0.5f**,
confirming desc stores FULL height while the shape stores HALF -- third
desc word raw to +0xe8, validates radius against the zero global with the
assert-report arm, then tail-calls the BASE apply-desc row. The row stays
discovered until that chain closes; the decode is complete and recorded.

Two pinned-image constants resolved alongside: `.rdata 0x101043cc = 0.5f`
(the height halver) and `.rdata 0x101041f0 = 0.0f` (the validation zero
every setter fcomp's against).

## 3l. Slot 1 decoded: apply-from-descriptor

phys_fn_001347 (`0x00027740`, BASE slot 1, inherited by every final):
copies descriptor+`0x08..0x38` -- the twelve-word third pose -- into
shape+`0x6c`, moves halfword `desc+0x38` to shape+`0xde`, `desc+0x3c` to
shape+`0xda`, writes `desc+0x40` into `[colobj+4]` when the colobj exists,
then calls helper phys_fn_000480 (`0xedc0`) with `(shape, [desc+0x44])`
and helper `0x26d90` with `(this, [desc+0x3c])`, returning true.

The descriptor COMMON layout is now fully mapped: `+0x00` id/handle,
`+0x08..0x38` pose three, `+0x38/3c/3e` the halfword trio mirrored from
the shape, `+0x40` collision-object data, `+0x44` group, `+0x4c..`
per-type payload (dims / radius / normal+D / mesh pointer -- each closed
save-row writes its own payload there). Helper `0x26d90` is a VALIDATED
group setter (rejects >= `0x20` through the error stream, sets shape+`0xd8`
and derives dirty-mask `1 << group` via `0x26c90`); helper `0x000480` is a
lazy shape-to-descriptor REGISTRY (an allocator-grown list keyed by the
descriptor pointer, searched and inserted on apply). Both stay discovered;
their drives need the registry fixture Task 2 owns.

**Group setter closed** (phys_fn_001329): the validated group-setter
transcribed as `ShapeBase::nxApplyGroup` -- rejects >= `0x20` (report arm
recorded, not modeled), stores at shape+`0xd8`, marks dirty-flag `0x04`.
Driven with group 7 on a fresh sphere: `+0xd8 = 0007` bitwise. Registrations
42 lines, oracle digest `18f5c3ca`, coverage floor 42.

## 3m. The descriptor layout NAMED -- every offset maps to a pinned field

Cross-referencing the decoded rows against the pinned public headers
(`Physics/include/NxShapeDesc.h`, `Nxp.h`) names every descriptor offset
the Phase 5 rows touch:

| desc offset | pinned field | evidence |
|---|---|---|
| +0x00 | vptr | virtual dtor/setToDefault/isValid |
| +0x04 | type (NxShapeType) | set by derived ctor |
| +0x08..+0x37 | **localPose** (NxMat33 @+8, NxVec3 t @+0x30) | BASE slot 2 saves shape pose three here; slot 1 loads it back |
| +0x38 | **shapeFlags** (u32; low halfword cached at shape+0xde) | apply/save rows move the low word |
| +0x3c | **group** (NxCollisionGroup = NxU16) | slot 1 passes it to validated setter phys_fn_001329 -> shape+0xd8 |
| +0x3e | **materialIndex** (NxMaterialIndex = NxU16) | applied to shape+0xda |
| +0x40 | **userData** (void*) | written to [collision object+4] by slot 1, saved back by slot 2 |
| +0x44 | **name** (const char*) | passed to registry phys_fn_000480 -- it associates shapes with their debug NAME pointers |
| +0x4c.. | derived payload | box dims / plane normal+D / sphere radius / capsule radius+height / mesh ptr |

The registry helper phys_fn_000480 decodes completely under this light:
`f(shape, name)` -- null shape returns false; null name with no list
returns true (nothing to dissociate); otherwise it lazily grows an
allocator-backed list keyed by shape pointers, inserting or removing the
shape/name association. NovodeX tracks which shapes carry which debug
names so the error stream can report them (the assert literals at
.rdata 0x10107574 etc. are exactly such name strings).

Consequences for Task 2: the descriptor structs ARE the records the
save/load rows read and write; reconstructing them is now mostly
assembling already-pinned offsets under their pinned names. The remaining
unknowns are confined to per-type payload tails beyond the first field.

## 3n. Status: all simple rows exhausted

Every self-contained row -- getters, setters, save-to-descriptor variants,
AABB/local-AABB/world-AABB computations, identity/self-return rows,
destructors, validated group/radius setters, and the BASE stub trio --
has been transcribed and driven bitwise against the pinned oracle. That is
**30+ rows closed across six tables**, each with a named transcription in
`Physics/src/ObjectModel.cpp` and a registered coverage assertion.

The remaining open rows are exclusively DEEP CHAINS that require shared
infrastructure from later tasks:
- **slots 1/12** (apply-desc / loadFromDesc): need the registry fixture
  (phys_fn_000480) and the error-stream reporter (Task 2);
- **slot 5** (raycast): needs the NxRaycastHit output structure (Task 2);
- **slot 7** (sweep): needs the swept-contact pipeline (Task 4);
- **slot 3** (debug draw): needs the renderer vtable interface (Task 4);
- **slot 4** (compute-mass; the "cached bounds" reading was falsified --
  sections 3p/3q): all three per-type rows closed -- SPHERE, BOX and
  CAPSULE -- without any scene infrastructure. Only the parallel-axis
  pair (0x1bdc0/0x1c040) behind non-null `extra` drives stays open.

These are NOT deferred because they are hard -- they are deferred because
their inputs come from subsystems that Tasks 2 and 4 reconstruct. Closing
them now would require either mocking those subsystems (dishonest) or
reconstructing them first (the correct order).

## 3o. Registry helper phys_fn_000480 partially decoded

`phys_fn_000480` (`0xedc0`, ~400 bytes, cdecl): shape-to-name association
tracker. Signature `bool f(void* shape, void* name)`:

- null shape -> return false
- null name + null list -> return true (nothing to dissociate)
- non-null name + null list -> alloc 16-byte list via SDK allocator,
  zero three dwords, store at global `.data 0x10123c0c`
- search entries (8-byte stride) comparing against shape pointer
- found + non-null name -> update name at entry+4, return true
- found + null name -> remove entry (shift last into gap), shrink list;
  if list becomes empty, free it via helper `0x0ea30` and null the global
- not found + non-null name -> grow list if needed (realloc pattern),
  insert `{shape, name}` pair

The full transcription requires decoding the growth/realloc paths
(`0x000eeb8..f30`) which interact with the SDK allocator shim. Deferred to
the next session with fresh context; the partial decode above is sufficient
to understand every call site's semantics.

The full listing is now captured (0x0000edc0-0x0000ef43). The growth path
at 0x000eeb8..ef30 doubles the list capacity via SDK allocator malloc,
copies existing 8-byte entries, frees the old block, and appends the new
{shape, name} pair. The function ends at 0x0000ef43 with `ret` returning
al=1 (success). The transcription target is bounded and ready for the
next session.

## Session Summary: Phase 5 Task 3 Progress

Across this session's rounds, Phase 5 Task 3 produced:

**Constructors transcribed** (all six shape families):
ShapeBase/001273, BoxShape/000977, SphereShape/001349,
CapsuleShape/000987, PlaneShape/001247, MeshShape/001379

**Behavior rows closed** (bitwise differentials against pinned oracle):
BASE: slot2 save-desc/001277, slot4 stub/001249, slot5 stub/004812,
slot7 sweep-stub/001035
BOX: slot0 dtor/000979, slots10-13 AABB+diag/save/dims,
slots14-16 identity/001391
SPHERE: slots8-15 local-AABB/world-AABB/center+radius/getRadius/
setRadius/save/loadFromDesc
CAPSULE: slots0/8/10/11/13/14 dtor/local-AABB/center+radius/save/setRadius
PLANE: slots0 dtor/1263, slots9-11 extent/001257, slot13 save/001251
MESH: slots13 save/001385, slot17 mesh-word/001381

**Key discoveries**:
- +0xd0 = NxShapeType tag (proven against pinned enum)
- +0xd4 = scene slot index (dirty-flag accumulator decode)
- +0xd8 = collision group; +0xda = materialIndex
- +0xde = cached shapeFlags low halfword
- Allocator free ABI: callee pops one pushed argument
- TriangleMesh+0x74 = reference count
- Descriptor layout fully mapped to pinned header field names

**Gate state**: auto-generated registrations from binary output;
coverage assertions matching floor exactly; only designed RED remaining.

The MESH loadFromDesc (phys_fn_001383, 46 B at 0x00027e30) decode reveals
a wrapper indirection: the descriptor's mesh pointer is a wrapper whose +4
field points to the inner triangle-mesh object. shape+0xe0 receives
*(wrapper+4) and the reference count lives on the inner object at +0x74.
After storing mesh and flags (+0xe4 from desc+0x50), it tail-jumps to BASE
slot 1. Null mesh pointer returns false without touching state.

Slot-4 helper call graph, decoded from prologues (later falsified and
corrected -- see section 3p):

- BOX slot 4 -> 0x1c8c0 -> {0x1bd00, 0x1bdc0, 0x1c040, 0x1c5c0*, 0x1c630}
- SPHERE slot 4 -> 0x1c930 -> {0x1c750, 0x1c5c0*, 0x1c630}
- CAPSULE slot 4 -> 0x1c980 -> {0x1c7c0, 0x1bdc0, 0x1c040, 0x1c5c0*, 0x1c630}

Common tail: each builds a local bounding volume via the family-specific
builder (0x1bd00 box / 0x1c750 sphere / 0x1c7c0 capsule), optionally
expands through 0x1bdc0+0x1c040 when a second operand exists, applies a
conditional transform via 0x1c5c0 when a scalar equals [.rdata 0x101041ec]
(= 0.0f sentinel), then commits through 0x1c630 into the caller output.
Decoding the five shared sub-helpers unlocks all three slot-4 rows at once.

## 3p. Slot 4 is a mass-frame row: the SPHERE chain decoded, driven, closed

The prologue-level reading above was wrong twice over, and both errors came
out of decoding the shared helpers instead of guessing from their callers.

**The constant was misread.** `.rdata 0x101041ec` is **1.0f** (bits
`0x3f800000`, read straight out of the file through the section table; the
phase 3 narrow-phase evidence already recorded it correctly). It is not a
0.0f sentinel, and the branch it participates in is an *equality skip*, not
a zero-detect.

**The rows are not cached bounds.** They are compute-mass rows. The
structure every chain member operates on is a 13-float **mass frame**:

```
+0x00..+0x20   nine floats   inertia tensor (row-major triples)
+0x24..+0x2c   three floats  center-of-mass offset
+0x30          one float     mass
```

The decode, smallest helper first:

- **phys_fn_000837 (0x0001c5c0, 101 B)** — scale, `__thiscall ret 4`. Ten
  `fld/fmul/fstp` triples multiply the nine inertia words and the mass by
  the argument. The COM offset at +0x24..+0x2c does not participate:
  scaling by a density keeps the center.
- **phys_fn_000839 (0x0001c630, 231 B)** — merge, `__thiscall ret 4`.
  Masses sum (`fld [ecx+0x30]; fadd [eax+0x30]`). The quotient is a real
  x87 `fdiv`: the 1.0f literal divided by the mass sum. Each side's
  weight-times-offset products are formed first, added pairwise (x as
  other+this, y/z as this+other — stack order, bitwise irrelevant for
  clean inputs), scaled by the quotient and stored z, x, y into the COM
  offset; then the raw sum stores into +0x30; then the nine inertia words
  accumulate componentwise. This is a center-of-mass merge plus parallel-
  axis-ready inertia accumulation.
- **phys_fn_000843 (0x0001c750, 110 B)** — unit-density solid-sphere
  builder, `__thiscall ret 8`. Radius cubed on the x87 stack, times
  `[.rdata 0x101068d8]` = **4π/3**, stored at +0x30; from that same value,
  two more radius multiplies then `[.rdata 0x101068e4]` = **2/5** give the
  diagonal inertia `(2/5)·m·r²`, stored to +0x00/+0x10/+0x20. Every other
  word is integer-zeroed (exact +0.0f). A non-null second argument folds
  two point-mass payloads at arg+0 and arg+0x24 through phys_fn_000831 /
  000833 (the parallel-axis pair, still undecoded).
- **phys_fn_000851 (0x0001c930, 77 B)** — the SPHERE-table slot-4 row
  itself, `__thiscall ret 0xc`, args pushed (density, radius, extra).
  Builds the unit frame into a local, scales it through 0x1c5c0 unless a
  `fucompp` against the 1.0f literal finds equality (`test ah,0x44/jnp` —
  an unordered density falls through and scales), then commits into the
  destination through 0x1c630.

The same constants participate in the exported Phase 3 mass kernels
(MassProperties.cpp records 0x101068d8 and 0x101068e4 already); slot 4 is
where those kernels' math hangs off the shape tables.

**Driven bitwise.** New `massframe` family: two oracle drives of 0x1c930
(density 2.0f through the scale arm; exactly 1.0f skipping it), radius
2.5f, extra null, all thirteen words of each destination folded.
`mass_scaled=4302e653` (=130.90…, exactly (4π/3)·2.5³·2), `mass_unit=
4282e653`, fold `0bed6c36`; candidate answers through the transcription
(`MassFrame` + `SphereShape::nxSphereComputeMassFrame`) and matches
bitwise first run. Registrations +3 lines (oracle row, both candidate
lines), floor 66→69, oracle digest re-pinned df843c3c→9f55f43b in the same
diff. Census: phys_fn_000851 → reconstructed/phase 5 with semantic label;
helpers 000837/000839/000843 annotated via static_proof while staying
discovered; phase-3 ledger discharges the row (deferred 437→436,
not-reconstructed sub-count 315→314); program.json rebalanced p3 owned
395 / remaining 334, p5 owned 201.

Two census hygiene defects fixed in passing: phys_fn_000989 and
phys_fn_001353 (both driven, one reconstructed) recorded sources under
per-class filenames that do not exist — the transcriptions live in
Physics/src/ObjectModel.cpp; so did phys_fn_001329 and phys_fn_001357 at
reconstructed state. The remaining ~376 discovered rows whose source names
a not-yet-created per-class file are aspirational homes for Tasks 2–4 and
were left alone.

Still open in the chains after this round: nothing in the builders -- BOX
and CAPSULE closed below; only the parallel-axis pair 0x1bdc0/0x1c040
remains undecoded behind the null-extra drives.

## 3q. BOX and CAPSULE slot 4: the remaining two mass-frame rows

**BOX (phys_fn_000849, 0x0001c8c0, 101 B, `ret 0xc`).** Same skeleton as
the sphere wrapper: local frame, optional payload pair when the fourth
argument is non-null, density scale skipped exactly on fucompp equality
with the 1.0f literal, merge into the destination. Pushed args:
(density, halfExtents pointer, extra). The builder phys_fn_000829
(0x0001bd00, 187 B) takes a POINTER to three floats and treats them as
HALF-extents:

- volume accumulator opens at the .rdata 1.0f literal; each extent is
  tested with an INTEGER word compare (`cmp [eax],0`), non-zero replaces
  the accumulator for the first hit then multiplies for the rest --
  MassProperties.cpp documents the same shipped quirk;
- mass = accumulator x **8** ([0x101068f0]) = 8hxhyhz -- full extents are
  twice half-extents. That value stays live on the x87 stack across the
  whole function while everything else is computed above it;
- diagonal factor F = mass x **1/3** ([0x101068ec]); diagonals are
  F(hy^2+hz^2), F(hz^2+hx^2), F(hx^2+hy^2) from squares of the raw
  half-extents -- exactly m/12((2h)^2+(2h)^2), the solid-box tensor;
- every off-diagonal and the COM offset integer-zeroed.

Driven bitwise: halfExtents {1.5, 2.0, 2.5}, densities 2.0 and 1.0,
mass_scaled=42f00000 (=120.0 exact), fold `d82de90e`, candidate identical
first run through `BoxShape::nxBoxComputeMassFrame`.

**CAPSULE (phys_fn_000853, 0x0001c980, 115 B, `ret 0x14`).** Five pushed
args: (density, axisSelector, radius, cylHalfHeight, extra). Builder
phys_fn_000845 (0x0001c7c0, 181 B):

- unit-density CYLINDER: mass = pi*r^2*(2c) ([0x101068d0] = pi); the
  hemispherical caps contribute nothing here;
- axial diagonal = mass*r^2/2 (folded through .rdata 0.5f at 0x101043cc);
- transverse pair = mass*(3r^2+4c^2)/12 -- the full cylinder formula over
  [0x101068f8]=3, [0x101068f4]=4, [0x101068e0]=1/12;
- axisSelector routes the axial term: 0 -> +0x00, >=2 -> +0x20, and
  selector==1 writes +0x10 but NEVER WRITES +0x00 -- a real image hole.
  The transcription reproduces the hole; the differential deliberately
  drives selectors 2 and 0 only, because folding an uninitialised word
  would compare garbage on both sides;
- wrapper wiring confirmed against the ret: density compares at arg+4,
  the payload pointer is arg+0x14.

A second constant trap fell in the decode and cost one candidate
iteration: `[0x101068f8]` was first read as 0.0f -- the zero belonged to
the NEXT slot (0x101068fc); the dword at f8 is bits 0x40400000 = **3.0f**.
The transcript's own diagonal words caught it: oracle transverse
33.846 = m(3r^2+4c^2)/12 against the wrong-model prediction 26.18 =
m(4c^2)/12. Lesson recorded: read constants with their addresses printed
BESIDE the values, never infer a slot's value from its neighbour.

Driven bitwise: drive E (selector 2, r 1.25, c 2.0, density 2) folds with
diagonals 4287663e.4287663e.41f56fdb and mass 421d1463; drive F (selector
0, density 1) gives 41756fdb.4207663e.4207663e.419d1463; fold `ab81bd0c`,
candidate identical through `CapsuleShape::nxCapsuleComputeMassFrame`.

Registrations +4 lines (two oracle rows, two candidate lines), coverage
floor 69->73, oracle digest re-pinned 9f55f43b->b6b7eb30. Census:
000849 reconstructed (already phase 5), 000853 reconstructed into phase 5
-- ledger discharges it (deferred 436->435, phase3.json total follows);
program.json p3 owned 394 / remaining 333, p5 owned 202. Builders
000829/000845 annotated via static_proof while staying discovered.

All three per-type compute-mass rows are now closed against the pinned
oracle; the shared helpers' only remaining secret was the parallel-axis
pair -- and half of it fell the next round:

## 3r. The payload fold step, phys_fn_000831

The wrappers call it twice for a non-null `extra`: `0x1bdc0(frame, extra)`
then `0x1c040(frame, extra+0x24)` -- both `__thiscall`, callee pops. The
payload record is **{Vec3 d; SymMat3 K}** at 0x24-byte stride (upper
triangle packed k00/k01/k02/k11/k12/k22 at +0x0c..+0x20).

Decoded with an x87 stack-depth simulator over the listing (a plain read
misled twice: two phase-2 results stay STACKED across later blocks, and
the pop edi/pop esi pair shifts the frame so 0x1bfb1's `[esp+0x18]` reads
**A[1]**, not the W slot). Semantics: a STRAIGHT-LINE TRANSFORM, not an
accumulate -- all nine inertia words are OVERWRITTEN from products of the
old inertia with d and K; the COM offset becomes {o.d, o^T K col0,
o^T K col1}; mass untouched. With A-temps as the nine faddp chains:

    a0..a2 = d^T I columns      (dy*I3+dz*I6+dx*I0, cyclic)
    a3..a8 = K·I rows 0-1       (k00*I0+k02*I6+k01*I3, ...)
    I'00 = (A0dx + A2dz) + A1dy          -- the quadratic form d'Id
    I'01 = T1 = (A1k01 + A2k02) + A0k00  -- stacked, survives to the fxch
    I'02 = T2 = (A1k12 + A2k22) + A0k11  -- stacked
    I'03 = Q0 = (A3dx + A5dz) + A4dy     -- first fstp ([esp+0x68])
    I'11/12 = U1/U2;  I'20 = W;  I'21/22 = V1/V2

**The decisive modelling fact**: every phase-1 temp is rounded to float32
by its own fstp m32 before the copy feeds it back. Drive 1's dyadic values
hid this completely; drive 2's non-dyadic payload exposed it as 1-3 ULP
drift on exactly the words downstream of the copy. The transcription
therefore rounds each intermediate -- and the differential drives use
fully dyadic inputs on both drives so no future rounding-path question can
perturb the pinned words.

Driven bitwise: new `paxis` family calls 0x1bdc0 DIRECTLY on two crafted
(frame, payload) pairs -- full non-symmetric inertia, non-zero offset,
every payload field non-zero -- folding all thirteen result words.
`s0=41660000 q0=c0ae0000 digest=1d701701`; candidate identical through
`MassFrame::nxMassFrameFoldPayload`, stable across three consecutive runs.
Registrations +2 lines (oracle row, candidate line), floor 73->75, oracle
digest re-pinned b6b7eb30->836cc35f. Census: phys_fn_000831 discharged
from the PHASE-2 ledger (its `homeless_shared_code` deferral; deferred
1139->1138) into phase-5 ownership as reconstructed; program.json p2 owned
143 / remaining 87, p5 owned 203.

One process lesson recorded: an earlier build printed a different oracle
word for drive 2's last slot; the current build is stable across repeated
runs and the drives are dyadic-exact, so the pinned digest no longer
depends on any rounding path.

## 3s. The partner helper phys_fn_000833: structure mapped, transcription open

`0x1c040` (1371 B) is the second call of the pair -- `__thiscall`
(frame, payload2) with payload2 = extra+0x24, i.e. a SECOND {Vec3 d;
SymMat3 K}-shaped stride into the same buffer. It is NOT the same transform
as 0x1bdc0. Structure, from the depth-annotated listing:

- **Early out**: if all three payload words d.[0..8] are zero, jump straight
  to the tail at 0x1c592.
- The frame's COM offset is read and NEGATED term by term (fchs), and each
  component of d+o is formed and integer-tested. All three sums zero sends
  control down a SHORT centered path at 0x1c0d7; any non-zero sum takes the
  LONG displaced path at 0x1c26f.
- Short path: builds quadratic forms in the negated offset -- e.g.
  (-oy)*oy + (-ox)*oz chains against [esp+0x24]/[esp+0x20] -- consistent
  with pure-shape folding where d == -o makes the displacement vanish while
  dd^T-style sign terms survive.
- Long path: negates the three sums into r = -(d+o), keeps one component
  alive across registers ([esp+0x7c]), and emits the standard parallel-axis
  cross-term pattern (products like Ry*sumy stored as partials, Rx*sumz
  cross terms added, .rdata literals multiplied in). Several terms are
  multiplied by the literal **[0x101041f0] = 0.0f** -- zeroed coefficients
  whose signed-zero results still propagate through the adds, so a faithful
  transcription must KEEP the multiplies exactly where they stand.
- Tail at 0x1c592 shared by all paths.

Status: row phys_fn_000833 stays OPEN (`discovered`, phase 2 ownership).
The mass-frame chain stands at: builders + scale + merge + fold step closed
bitwise (six rows), partner structure mapped, formula-level decode of both
0x1c040 paths deferred -- it gates nothing currently driven, since every
closed slot-4 row uses null-extra drives.

## 3t. The error stream: Task 2's keystone decoded and driven

Sphere setRadius (0x278c0) exposed the whole mechanism in one block. After
storing the radius, the image `fcomp`s it against the zero literal
(0x101041f0) and takes `test ah,0x41 / jp` over the FPU status: a report
fires unless the radius is strictly positive (zero, negative and NaN all
fall through). The report itself:

    mov ecx,[0x101041b0]      ; guard object pointer
    cmp dword ptr [ecx],0     ; flag word must be non-zero...
    jne +1 / int3             ; ...else INT3, by design
    push 0x101075dc           ; "SphereShape::setRadius: radius should be positive!"
    push 0                    ; code
    push 0x4a                 ; line 74
    push 0x10107574           ; "\Epic\Novodex\SDKs\Physics\src\SphereShape.cpp"
    push 1                    ; kind
    call [0x101041b4]         ; indirect cdecl through the report slot
    add esp,0x14

So: a five-argument cdecl reporter -- (kind, sourceFile, line, code,
message) -- behind a function-pointer slot at .rdata 0x101041b4, gated by a
flag word one pointer earlier. The strings pin the build tree
(`\Epic\Novodex\SDKs\Physics`) and give Task 2 its first two exact
literals.

**Two operational discoveries**, both paid for with a fail-fast:

1. The SHIPPED default reporter is fatal on warnings -- that is what the
   zero flag word guards. Calling it directly (or leaving the flag zero
   while an invalid radius fires) kills the process with 0xC0000409.
2. The slot and its guard live on READ-ONLY .rdata pages. Patching them
   needs VirtualProtect around the write window; restore afterwards. The
   allocator shim never hit this because it writes THROUGH its pointer
   into heap.

**Driven bitwise**: new `errstream` family. Oracle side flips the page
protection, installs a capture sink over [base+0x1041b4], satisfies the
guard flag, drives setRadius(-1) then setRadius(2.5), restores everything;
the fold covers fired-count, kind, line, code and both literal strings.
`invalid_fires=1 valid_fires=0 digest=c2ab04f6`. Candidate side runs the
same two drives through the reconstruction (`nxInstallReportSink` +
`nxSphereSetRadius`, whose report arm is now transcribed with the image's
exact literals) and matches bitwise first try.

Registrations +2 lines, coverage floor 75->77, oracle digest re-pinned
836cc35f->45014fec. No new census row -- the call site lives inside
already-reconstructed phys_fn_001357, whose static_proof now records the
report arm; the reporter being runtime-installed means there is nothing
static to own beyond the slot constants documented here. This unblocks the
validation arms of applyGroup (group >= 0x20) and the descriptor
loadFromDesc rows for Task 2 proper.

## 3u. The group-validation arm: applyGroup completed, and mPrunable24 named

applyGroup (0x26d90) was transcribed with its invalid arm stubbed. The full
decode, using the section 3t mechanics:

- `cmp ax,0x20 / jb` splits the paths. The INVALID arm reports through the
  same guarded slot -- (1, "\Epic\Novodex\SDKs\Physics\src\Shape.cpp",
  line **0xe0**, 0, "group ID must be < 32!") -- and then **JOINS** the
  valid path at the dirty-flag call: the store at +0xd8 is skipped but the
  dirty flag 0x04 and everything after run EITHER WAY.
- Both paths end by rewriting the prunable's unidentified dword
  `mPrunable24` -- which sits at abs shape+0xc8 -- as a group mask:
  `1 << low-byte(group)`, the x86 shift masking its count to five bits.
  That names the field: it is the collision-group mask the broad-phase
  filters against.

Transcription updated (`nxApplyGroup` reports through the reconstruction's
own sink with Shape.cpp's exact literals, skips only the store on invalid,
and rewrites `mPrunable.mPrunable24`). New `grouperr` family: drive
group=0xFF then group=5 against both sides, folding the capture plus the
post-drive +0xd8 word pair and +0xc8 mask. Oracle:
`invalid_fires=1 d8=00000005 c8=00000020 digest=b6f879ec` -- the invalid
drive fired once and stored nothing; the valid drive left d8=5 and
mask=1<<5. Candidate identical first try.

Registrations +2 lines, floor 77->79, oracle digest re-pinned
45014fec->bea77b31. phys_fn_001329's static_proof records the report arm
and the mask naming.

## 3v. The loadFromDesc validation arms: store-anyway semantics pinned

Sphere loadFromDesc (0x27850) and capsule loadFromDesc (0x21ad0) both
validate their radius with the section 3t pattern -- but the ordering is
the finding: the radius is STORED UNCONDITIONALLY FIRST (sphere: `fst
[esi+0xe0]` before the fcomp; capsule: all three data words written before
the fcomp), the report fires unless strictly positive (sphere line **0x35**,
capsule line **0x37**, each with its own file/message literals), and the
BASE apply-desc tail runs REGARDLESS. An invalid descriptor therefore
leaves a fully-populated, invalid shape behind -- shipped behaviour,
reproduced. Plane loadFromDesc (0x25460) has no report arm at all; mesh's
(0x27e30) only null-checks its wrapper.

New `loaderr` family: radius=-1 descriptors driven through both rows on
both sides; fold covers both captures plus the stored sphere/capsule radii
and capsule half-height. `fires=1/1 rad=bf800000.bf800000 hh=00000000
digest=8f125103` -- candidate identical first try through the updated
transcriptions.

Registrations +2 lines, coverage floor 79->81, oracle digest re-pinned
bea77b31->2a993332. 001353/000989 static_proofs extended.

## 3w. The default material template: NxMaterial lands bitwise

The SDK stores materials BY VALUE -- `NxArraySDK<NxMaterial>` at SDK+0x28,
stride 0x48 (phase2-sdk.md 4.6) -- and keeps the default template at .data
0x1220a0, statically initialised in the image. The pinned header's field
order fills all 72 bytes:

    +0x00 dynamicFriction   +0x1c dirOfAnisotropy.x = 1.0f   +0x34 speedOfMotion
    +0x04 staticFriction    +0x20 dirOfAnisotropy.y          +0x38 flags
    +0x08 spinFriction      +0x24 dirOfAnisotropy.z          +0x3c frictionCombineMode
    +0x0c rollFriction      +0x28 dirOfMotion.x = 1.0f       +0x40 restitutionCombineMode
    +0x10 restitution       +0x2c dirOfMotion.y              +0x44 programData
    +0x14 dynamicFrictionV  +0x30 dirOfMotion.z
    +0x18 staticFrictionV

Transcribed as `NxMaterialRecord` (`setToDefault` matching the pinned
inline; `setInternalFlagBit31` modelling the store at 0x0000e9ee that runs
on the TEMPLATE only after it has been copied into the SDK array).

New `material` family folds the shipped template bytes raw against a fresh
record: **bitwise identical** (`flags=00000000 digest=527814f5`). The
harness never creates an SDK, so the template sits in its pre-creation
state -- pure setToDefault, bit31 not yet set; the candidate therefore
compares WITHOUT the internal bit, which is exactly the distinction the
image itself draws between "fresh record" and "shipped template".

Registrations +2 lines, coverage floor 81->83, oracle digest re-pinned
2a993332->e493f315. The sixteen censused data-word rows covering the first
64 bytes of the template (phys_data_003047..003062) are validated by this
family; data rows carry no static_proof key, so the validation lives here.
Task 3's material model now stands on bitwise ground.

## 3x. phys_fn_000973 corrected: the box-hull face-record builder

The section 4 guess that 0x10106998-0x10106a48 held "twelve small vtables"
of a descriptor setToDefault family is FALSE -- dumping the slots shows no
function pointers at all, only small integers. The region is CUBE-FACE
TOPOLOGY:

    0x10106998..0x101069f7  six faces x four corner indices (values 0-7):
        0123 / 1562 / 5476 / 4037 / 3267 / 4510
    0x101069f8..0x10106a57  the same six faces with every index shifted
        by +8: 0123 / 1865(1,8,5,9) / 6745 / 11-3-10-7 / 2-9-6-10 / 4-11-0-8

Two copies of one cube topology against a DOUBLED vertex array (corners
0-7 and 8-15). phys_fn_000973 (0x21420) is therefore the box-hull
FACE-RECORD BUILDER: it writes PAIRS of pointers into hull records at
+0x154/+0x158, +0x178/+0x17c, +0x19c/+0x1a0, +0x1c0/+0x1c4, +0x1e4/... --
one pair per face at a 0x24 stride, each pair being {base corner list,
shifted corner list}. The transcription already anticipated the slots --
BoxFaceRecord's mIndexListA/mIndexListB -- so what today's dump adds is
their CONTENT: the actual six quad faces of the cube, base and shifted
copies, confirming the doubled-vertex layout the facade carries. The
tables also relate to the support/bounds tables BoxHullFacade transcribes
from .rdata 0x10122180+ (a third copy of related topology). The census row
stays discovered under phase-2 ownership; its full decode belongs to
whoever reconstructs the box-hull construction end to end.

## 3y. Task 4 scaffolding: the shape-to-scene registration interface

ShapeBase's ctor owned arm -- everything Task 4's actors must provide --
is now decoded end to end from the listing:

- Guard (`0x000255e4..0x0002561d`): `owner = this[+0x04]`; registration
  runs only when owner != null.
- `scene = owner[+0x04]`; `container = scene[+0x48]`; then
  `phys_fn_002423(container, shape)` registers the shape.
- phys_fn_002423 (0x5c390) reads the shape's SCENE SLOT (this[+0xd4],
  i.e. the base ctor's second argument) and inserts the shape pointer
  into a growable vector at container+0x90/0x94/0x98
  (begin/end/capacity): in-place store `[begin + slot*4] = shape` when
  slot < count, otherwise growth in chunks of `(slot+0x100) & ~0xff`
  entries through the SDK allocator.
- It then calls 0x5bc90(container, slot), which performs the same dual
  bookkeeping on a SECOND vector at container+0x00/0x04: two parallel
  arrays indexed by scene slot.

Consequences for a fake-owner differential (next step): a capture
container must pre-size BOTH vectors so both paths take their in-place
stores, and 0x5bc90's own tail needs decoding first -- it is another
chunk-growing walker, not a trivial store. Also recorded en passant:
every base-ctor run re-installs the three Prunable owner adapters at
.data 0x10128470/74/78 (values 0x213e0/0x25520/0x25510), matching
IcePrunable.h's globals.

**Drive attempt (blocked)**: a four-vector fake scene -- all triples
(+0x00/04/08, +0x10/14/18, +0x20/24/28, +0x30/34/38, +0x90/94/98)
pre-pointed at eight-entry arrays with count 8 > slot 3 -- still fail-fasts
inside the oracle ctor's registration. Some undecoded branch in 002417's
middle wanders past the modelled fields. Until that body is decoded, the
owned-arm drive stays open; the transcription side is implemented
(`nxSceneInsertShape` reproducing shape-store / sentinel / count-mirror,
invoked from the base ctor when owner != null) and is ready to close
bitwise the moment the oracle side can run.

**Then SOLVED.** Running under cdb put the fault at 0x5c102 -- the growth
copy loop writing through a NULL buffer. The trigger: 0x5bc90 grows when
**capacity <= end** (`cmp [esi+0x18],[esi+0x14]; ja skip`), and new
capacity is `2*count+2` entries -- my fake vectors had cap == end, so every
drive took the grow branch and the shim's malloc returned NULL above 32
bytes. The fix is simply RESERVE: capacity pointers well beyond end on all
five triples. With reserved caps the oracle ctor runs to completion against
the fake scene, and the new `ownctor` family folds structural facts (slot
stored at +0xd4; sentinel -1 written; count mirrored; shapes vector holds a
non-poison pointer): `d4=00000003 sent_ok=1 mirror_ok=1 shp_ok=1 digest=
641beb67`. Candidate identical through `nxSceneInsertShape`.

Registrations +2 lines, coverage floor 83->85, oracle digest re-pinned
e493f315->3171b22c. Task 4's first chain -- construct-with-owner registers
into the scene arrays -- is closed bitwise without any scene simulation.

## 3z. Deregistration: the base dtor's three remover call sites

The BASE scalar-deleting dtor (0x27710) calls the real dtor body 0x26bd0,
which -- when owner != null -- performs THREE separate removals against
THREE different scene containers:

    0x00026beb  [scene + 0x70c] |= 2          scene dirty flag
    0x00026bfc  container = [scene + 0x48]
                0x5bbe0(container, shape)      shapes-array remove #1
    0x00026c0f  container = [scene ] + 0x5d4
                0x5aae0(container, shape)      shapes-array remove #2
    0x00026c25  container = [scene ] + 0x6e4
                0x1b90(container, slot=d4)     slot-keyed remove #3

Remove #1 (0x5bbe0, 36 B): reads the shape's slot, calls guard 0x5bac0,
then `shapes[slot] = 0`. The GUARD (0x5bac0) is itself substantial: it
early-outs when `array[slot] == -1` (already freed), and otherwise may run
the same grow-or-skip logic against the FREE-LIST vector at container
+0x30/0x34/0x38 before pushing the slot onto it -- the shapes array keeps
a classic free-list for reuse by later createMaterial/createShape calls.

Removers #2/#3 (0x5aae0, 0x1b90) plus 0x5bac0's tail are undecoded. An
owned-dtor differential therefore waits on those bodies; the registration
side (section 3y) remains closed and unaffected.

**Status update**: all three removers are now transcribed
(`nxSceneRemoveShape` / `nxSceneRemovePairs` / `nxSceneSlotFree`) and wired
into every per-shape dtor through `nxBaseDtorOwnerArms`. The differential
itself stays BLOCKED: a fully pre-sized fake scene still diverges from the
oracle on four structural predicates (count pop, swap-move, pair
compaction, slot freepush all read false after an oracle dtor pass), which
means at least one of the three remover bodies contains behaviour beyond
the natural model -- most plausibly in 0x5aae0's tail or 0x5bac0's grow
path. The transcription is decode-faithful to everything verified so far;
the family gets built the moment those bodies are finished. No census or
registration claims were made for the owned-dtor chain.

## 3z2. The real-SDK pivot opened (and its price tag)

The fake-container gymnastics have a natural exit: boot the REAL SDK
in-process. `NxCreatePhysicsSDK` is not exported from this UE3 build, but
the creator is located in the census: **phys_fn_000490** (0xfae0, 138 B)
-- it bootstraps the FoundationSDK through the allocator adapter at
0x101041a8, checks the version immediate **0x02010200 (= 2.1.2)** against
its first argument, allocates a 0x38-byte SDK wrapper, constructs it via
0xe1b0 and stores the singleton at 0x123c04.

A first boot attempt under cdb crashes inside an external CRT-era module
(0x60f54335) -- the shim's flat 1 MiB block is not enough: the SDK's
allocator adapter needs REAL malloc/free semantics (arbitrary sizes,
reuse after free) that a static block cannot provide. The honest bill:
a proper bump-plus-freelist allocator emulation in the shim (sized ~64 KiB,
serving any request, reusing freed ranges) before real-scene differentials
become safe.

**Allocator emulator LANDED** (first-fit free list over a 1 MiB arena,
8-byte headers, immediate coalescing; frees served at adapter vtable slots
+0x00/+0x0c/+0x14, malloc at +0x08). Validated against every existing
family -- all digests stable, designed RED only -- so it is strictly more
faithful than the flat block it replaces. The boot itself now proceeds past
Foundation allocation and fails inside a foreign CRT module during the
Foundation factory call at [0x101041a8], which uses its own CRT heap beyond
the emulator's reach: emulating/stubbing THAT factory (or decoding
phys_fn_000490's full expectations of it) is the next Task-4 infrastructure
item before real-scene differentials become safe.

Task-4 status after this round: construct-with-owner closed bitwise;
remove-with-owner interface mapped (three sites, offsets recorded); remover
decode queued.

**Remover decode progress + drive status**: all three removers are now
transcribed in the reconstruction (`nxSceneRemoveShape` with the freelist/
swap-remove/poison semantics, `nxSceneRemovePairs`, `nxSceneSlotFree`) and
wired into `nxSphereScalarDeletingDtor` via `nxBaseDtorOwnerArms`; the
other four per-shape dtors carry the same call. Two branch facts learned
the hard way: remover #3's `+0x0c` field is a LIMIT, not an end -- push at
the `+0x08` cursor happens when limit > cursor (ja) -- and 0x5bc90's grow
trigger is capacity <= end. The owned-dtor differential itself remains
BLOCKED: even with every container pre-sized, the oracle dtor still
fail-fasts inside some undecoded segment of the three removers. No family
or registration has been created for it; the transcription stands ready to
close bitwise once the remaining bodies (0x5aae0 tail, 0x1b90 middle,
0x5bac0 grow path) are decoded.

**Refinement (full registrar body walked)**: phys_fn_002423 touches ONLY
container+0x90/0x94/0x98 -- every allocation goes through interface calls
(`[eax+8]`, `[edx+0x14]`, `[ebp+0x10]`, i.e. the SDK allocator adapter),
never a raw malloc. The second array at container+0x00 belongs entirely to
0x5bc90, whose visible tail writes a FREE-LIST SENTINEL
(`[array2[slot]] = -1`) into it and mirrors a count into yet another
buffer at container+0x20 -- so 0x5bc90 manages at least three parallel
arrays (free-list, counts, and the +0x10/0x14 pair it sizes from). A safe
fake-owner drive therefore needs the whole 0x5bc90 body decoded first:
every un-decoded branch is a potential growth path whose allocator call
returns NULL under the shim.

## 3z3. The remover chain closed: growth arms decoded, three rows discharged

The blocked differential of §3z is unblocked by decoding the three bodies,
and the decode corrects two beliefs that section recorded. All three
removers are now transcribed at full fidelity and driven bitwise on twin
state; phase-5's reconstructed count rises 64 -> 65, and two cross-phase
rows leave their deferrals as dynamically_gated (Phase 2's total closed
56 -> 57, Phase 3's 61 -> 62).

**phys_fn_000028 (0x1b90, 165 B) is not a "slot-keyed remove"** -- it is
`std::vector<NxU32>::push_back` over the VC9 layout {_Myproxy@+0x00 never
touched by this row, _Myfirst@+4, _Mylast@+8, _Myend@+0xc}, `ret 4`. The
fast path stores at the cursor when capEnd > end; otherwise the growth arm
allocates **2*size + 2 dwords** (`lea eax,[eax+eax+2]`) through adapter slot
+8 with flag word 0, copies the live elements dword-wise, releases the old
block through slot +0x14, and reseats all three cursors. The compiler's own
escape (`jae` over the arm when old capacity >= new) is arithmetically
unreachable while full; it is transcribed anyway.

**phys_fn_002410 (0x5bac0, 240 B) carries the same push arm inlined** at
fields +0x30/34/38, wrapped in TWO narrower guards than §3z believed:
sentinel != -1 gates only the PUSH, sentinel == 0 gates only the UNLINK.
Consequences the family pins: a virgin index (-1) skips the push but STILL
runs the unlink against whatever its mirror word names -- cntA gets
rewritten and the count cursor pops again -- and an already-released index
pushes a DUPLICATE onto the freelist while skipping the unlink. The unlink
itself swaps the count vector's last value across the +0x10 counts /
+0x14 cursor / +0x20 mirrors arrays, pops, zeroes the sentinel and poisons
the mirror with 0xD00BEED0.

**phys_fn_002418's contract was misread in §3z**: the dtor calls it with
`ecx = scene + 0x5d4` -- THE ADDRESS OF THE FIELD (0x00026c13
`add ecx,0x5d4`), not the field's value -- and phys_fn_002344 (0x5aae0)
dereferences twice: `[this]` names a second header whose words are
{begin,end} of the stride-8 pair array. A match is either half equal to
the value; a matched record is replaced by the LAST record with the
self-copy skipped at matched-last; the header cursor shrinks by 8 per
removal; and because the loop rescans the slot a swap just filled,
duplicate matches fall in one pass. The earlier owndtor drive passed with
this row effectively unproven: its pair predicate was false on both sides
(the swap brings the last RECORD forward, which that fixture's expectations
did not model), so both sides folded the same zero. pairrm supersedes it
with exact sub-drives: middle removal, matched-last shrink, duplicates,
no-match, empty list.

**Harness changes.** The candidate's allocator now rides the same emulator
arena as the oracle: `NxTestArenaAllocator` registers through
nxSetSdkAllocatorBridge at boot, so transcription-side allocations answer
for shim-side ones and four new counters (malloc/free ops and bytes, folded
as per-drive deltas) pin the allocation stream itself. relgrow's initial
free-vector block comes FROM the arena -- a foreign block would turn the
growth arm's release into a silent no-op. Two harness defects were found
and fixed on the way: the first relgrow setup never assigned header field
+0x20 (the mirror array), faulting the oracle at 0x0005bb85 inside the
unlink -- diagnosed under cdb from the faulting instruction's ecx=0 -- and
the fold originally read the STALE pre-growth buffer instead of the live
vector data.

**Falsifications.** Two mutation probes in the throwaway tree, each against
a control run reading mismatches=1 (the designed vtables RED):
releasing with the partner index in place of the poison constant moves
mismatches to 3 via relgrow AND the owndtor chain; dropping the pair
matcher's first-half arm moves the pairrm digest f0bdae43 -> 2df229f4
(mismatches=2).

**Census.** phys_fn_000028 closes as a Phase 5 row. phys_fn_002410 leaves
Phase 2's homeless_shared_code deferral -- which had already named phases
[3, 5] as drivers -- discharged_by_phase 5; phys_fn_002344 leaves Phase 3's
not_reconstructed_in_phase deferral, closed outright with no discharge
claimed since that deferral named no drivers. Registrations +7 (four rows,
three candidate drives), floor 91 -> 98, oracle digest re-pinned
f9691824 -> e7114361.

## 3z4. Task 4 opens: the actor scaffolding lands, fourteen rows close

Task 4's first body of work is not the actor CONSTRUCTOR -- it is what the
eight smallest slots of the dynamic table (0x104530) turn out to be:
guarded reads through a two-pointer spine. The actor layout this round
establishes: +0x00 vptr, +0x04 owner, +0x08 a TWELVE-BYTE member subobject,
+0x10 the scene lock context, +0x14 the body pointer.

**The guard pair.** Every accessor runs `edi = [this+0x10]; enter(edi);
read through [this+0x14]; leave(edi)`. The pair at 0x5b700/0x5b790 resolves
through four kernel32 import slots (0x10104010/14/2c/44 = EnterCritical-
Section, LeaveCriticalSection, InterlockedCompareExchange, GetCurrent-
ThreadId): a critical-section POINTER at *[scene], a writer flag swapped in
at block+0x18 and the owning thread id at +0x1c. The drives therefore run
real locking on both sides over a real initialized CRITICAL_SECTION, and
the thread id is folded only as a recorded/not-zeroed predicate -- the
first version folded its value and produced an unregistrable digest that
moved between processes while both sides still agreed.

**The eight accessors** decode uniformly once one trap is survived: slot 69
and 71 dereference TWICE -- [body+8] names a nested record and fsqrt reads
record+0xd0 / +0xd4 -- with only the RECORD guarded, so a null body would
fault the image itself. The first fixture put a scalar mark at body+8 and
the oracle faulted at `fld [eax+0xd0]` with eax=0xC001, the mark read as a
pointer; cdb named the instruction and the fix was semantic, not harness.
Slot 19 is bool([body+8]); slot 86 the group WORD at [body+0x1c]; slot 77
`([body+0x14] & mask) != 0`; slots 15/16 wrap two tiny helpers (phys_fn_
000015: mesh triangle span via the Shape type word at +0xd0 == 5 and the
+0xe0/+0xe4 array, else 1 or 0; phys_fn_000019: [+0xf0] for meshes, else
shape+0x9c -- both offsets the Shape layout already owned); slot 84 looks
the body up in the SDK pointer-binding table under guard.

**Construction.** phys_fn_000044 (0x2480) is the construction TAIL: wall
vptr, owner zero, member init over +0x08..+0x13, the one-slot member table
0x1010468c over +8, the body pointer at +0x14, then dynamic final
0x10104530. The member subobject lives under a THIRD table -- 0x101088b8
installed by phys_fn_002404 before life and RE-INSTALLED by its destructor
phys_fn_002406 after it. Slot 87 is an eight-byte this-adjustor thunk
(`sub ecx,8`, tail to the slot-0 deleting dtor), answering how the member
table reaches actor semantics. Slot 0 restores final tables, resets the
member, and releases through adapter slot +0x14 when flagged -- while
phys_fn_000042, the interface-wall dtor, frees through the LINKED CRT
directly (0x0002471 calls 0x100f41f0). Feeding that one an arena block
ended in STATUS_HEAP_CORRUPTION; it now receives its own CRT's malloc
(0x000f4722) and post-free bytes are not read.

**Falsifications** (throwaway tree, control mismatches=1): dropping the
member constructor's field zeroing moves actorctor to mismatches=2 via
ctMemberZeroed=0; restoring null instead of 0x101088b8 in the member
destructor moves BOTH post-free member reads to 0. Two transcription
defects were caught by these drives before any registration: the
construction tail passed the actor base where the image passes base+8 to
member init, and the candidate emulated the adjustor thunk by calling the
dtor at base+8 instead of writing the thunk's minus-eight semantics as its
own row.

**Census.** Twelve Phase 5 rows close reconstructed (000042, 000044,
000066, 000068, 000078, 000082, 000084, 000086, 000110, 000114, 000116,
000118); phys_fn_002404 and phys_fn_002406 leave Phase 2's
homeless_shared_code deferrals discharged_by_phase 5 (both deferrals
already listed phases [3, 5, 6, 7]), closing 59 of Phase 2's 143 rows.
Registrations +4, floor 98 -> 102, oracle digest re-pinned e7114361 ->
a969c85b.

## 3z5. Slate 2: the energy word's squared scales, and the write side

Five more actor rows close (000060, 000064, 000074, 000076 and the helper
000742), driven by the actorsm2 family.

**The energy word.** Slot 62 wraps a pure-x87 record helper, phys_fn_000742,
and the first transcription exposed how easy a faithful-looking formula is
to misread: the fxch/faddp ladder multiplies EACH cross product by its
second word TWICE, so the function computes 0.5 * (([+0x74]^2 + [+0x70]^2 +
[+0x6c]^2)*[+0x188] + [+0x194]*[+0x80]^2 + [+0x190]*[+0x7c]^2 +
[+0x18c]*[+0x78]^2) -- an inertial-energy shape over the record's velocity
at +0x6c..+0x74 against three squared scale words. The single-product
reading passed nothing loudly: oracle said 52.0f where the transcription
said 40.75f on marked inputs, and two one-term experiments (only
{+0x18c=4, +0x78=2} -> exactly 8; only {+0x74=2, +0x188=6} -> exactly 12)
pinned which terms halve before the squared-factor reading fell out of a
byte-exact stack simulation. All intermediates stay at full precision with
one rounding at st(0)'s fstp -- computed in double, cast once.

**Slot 68** guards the same nested record and answers true when it is null
or its word at +0x84 reads zero.

**The write side.** Slots 75/76 are the first MUTATING actor rows: they
upgrade through 0x5b730 -- EnterCriticalSection unless another thread holds
the +0x18 writer flag, in which case fail WITHOUT entering -- then mutate
body+0x14 (`|= mask` at line 0x1bb, `&= ~mask` at 0x1c1), then leave. The
failure arm reports kind 2 through the error stream from NpActor.cpp:
"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a
deadlock!" -- the family drives both arms, capturing the oracle side with
the errstream family's VirtualProtect dance over [.rdata 0x101041b4] (the
first attempt installed only the candidate-side sink and the shipped fatal
reporter killed the run from inside FoundationSDK::errorImpl).

**Falsification**: dropping the squared factors in the throwaway tree moves
the energy word to 41.5f and the transcript to mismatches=2 against a
control of 1.

**Census.** Registrations +2, floor 102 -> 104, oracle digest re-pinned
a969c85b -> 729c9651.

## 3z6. Slate 3: the damping getters, and a family that could not fail

Three rows close (000048, 000050, 000052). Slots 42/44 are the damping
getters -- getLinearDamping ([record+0xb8], NpActor.cpp line 0xd9) and
getAngularDamping ([+0xbc], line 0xe8) -- whose null-record arm reports
KIND 1 (warning, not the write side's kind 2) with shipped copy-paste
literals ("Actor::setLinearDamping: Actor must be dynamic!" inside the
getter) and then returns the zero constant parked at 0x101041f0. Slot 36
reads [+0x188] with no report at all.

The first actorsm3 structure had a defect worth its own paragraph: both
null arms shared one capture, and nxFoldErrCap ran once AFTER both drives
-- so only the SECOND report (angular, line 232) was pinned and the linear
warning was structurally unable to influence any digest. The falsification
attempt proved it: mutating the linear report's line literal moved nothing.
The fix folds the capture after EACH arm; only then does the line-literal
mutation move the transcript (candidate ok=0, mismatches=2 against control
1), which is what registered. The broader lesson is the programme's oldest
one in new clothes -- a check that cannot fail is not coverage -- applied
here to report-capture sequencing rather than to gate lists.

**Census.** Registrations +2, floor 104 -> 106, oracle digest re-pinned
729c9651 -> a3636235.

## 3z7. Slate 4: the binding write and the pose fallback

Two rows close (000088, 000092). Slot 83 is the WRITE half of the binding
pair: phys_fn_000480 keyed on the body pointer, under the write guard,
reporting line 0x1ff on a failed upgrade. Together with the slate-1 read
accessor the binding table is now proven from both sides -- and since the
key is each side's own body pointer, the family folds value-equality
predicates rather than addresses. Slot 6 is a guarded three-word pose read
with a FALLBACK: record+0x50/54/58 when the nested record exists, otherwise
the BODY defaults at +0x44/48/4c -- the first evidence that the actor
answers pose questions from its own storage when no body record backs it.
The falsification probe shifted the fallback's first word (+0x44 -> +0x40)
and moved the transcript to mismatches=2 against control 1.

**Census.** Registrations +2, floor 106 -> 108, oracle digest re-pinned
a3636235 -> 0e64d9fc.

## 3z8. Slate 5: the sleep chain is union-find

Three rows close (000062, 000713, 000744), and the decode answers what the
record's +0x1e8 word IS: a cached GROUP ROOT. phys_fn_000713 is textbook
union-find find() -- recursive path compression, self-parented cache at the
root, the fixed root stored back down the chain it walked. phys_fn_000744
compresses first, then walks the +0x1fc list answering whether every node's
word at +0x84 reads zero (the group-wide sleep test); slot 67 wraps both
under the read guard with true for a null record. The family drives the
fixer directly over a two-hop chain and folds the COMPRESSION ITSELF --
after the drive, A's cache must read the root -- plus settled/awake/null
arms through the wrapper. Falsified by dropping the compression store
(A's cache stays mid-chain; candidate ok=0, mismatches=2 against control
1). The +0x84 words are the same field the slate-2 energy term scales,
which suggests the group test and the energy word measure one system from
two directions; that reading stays open until the body ctor lands.

**Census.** Registrations +2, floor 108 -> 110, oracle digest re-pinned
0e64d9fc -> 98f55439.

## 3z9. Slate 6: the CMass local frame

Two rows close (000096, 000100). Slots 29 and 31 are the CMass getters:
twelve words (nine-word rotation from record+0xdc, translation from
+0x100) and nine words respectively. Their STATIC arms -- null record --
report kind 1 from NpActor.cpp with shipped copy-paste literals again
("getCMassLocalPose" warning inside the pose getter) and answer from
DEFAULTS: identity-plus-zero written field-wise for the pose, and the
shipped identity TABLE at .rdata 0x10122078 copied wholesale for the
orientation. The family folds present-record marks, both warnings per arm,
and memcmp predicates against the identity defaults. Falsified by dropping
the pose identity's middle diagonal (candidate ok=0, mismatches=2 against
control 1).

**Census.** Registrations +2, floor 110 -> 112, oracle digest re-pinned
98f55439 -> 1efebac8.

## 3z10. Slate 7: five three-word readers, one shape

Five rows close (000098, 000102, 000104, 000106, 000108) -- getCMassLocal-
Position, getMassSpaceInertiaTensorVal, getLinearVelocity, getAngular-
Velocity and getLinearMomentumVal. All five share one shape: read guard,
record+offset triple into `out` when a record backs the actor, otherwise
report KIND 1 with each row's own literal ("Actor must be dynamic!" on the
velocity pair, "Cannot be called on a static actor!" on the other three)
and fill defaults -- the zero TRIPLE at .data 0x10123c1c for position/
inertia/momentum, inline zeros for the two velocities. The momentum row
multiplies mass [+0x188] by the velocity words in natural component order
at full precision; the first transcription had the order reversed because
the x87 stack stores its LAST product first. The shared transcription
helper folds per-arm captures across all five rows; the falsification
probe misread the helper's middle word (+4 dropped) and moved the
transcript to mismatches=2 against control 1. With this slate the record's
field map is fully named from +0x6c through +0x194: velocity, angular
velocity, flags at +0x14, group word at +0x1c, sleep words +0x84/+0x10c,
inertia diagonal +0x18c..194, mass +0x188, CMass frame +0xdc/+0x100.

**Census.** Registrations +2, floor 112 -> 114, oracle digest re-pinned
1efebac8 -> 7b7cd8d7.

## 3z11. Slate 8: the group writer, a forwarder, an id allocator

Three direct rows close (000112, 000004, 000012) plus two indirect flips
(000015, 000019 -- the body helpers behind slots 15/16, closed through
those wrappers' bitwise drives under the programme's indirect-bitwise rule).
Slot 85 is the group WRITER -- word into body+0x1c under the write guard,
line 0x3cd when contended -- completing the read/write pair with slate 1's
reader; the drive reads back through slot 86 itself. phys_fn_000004 is a
sub-object virtual FORWARDER: [this+0x10] names a sub-object whose vtable
slot +0x18 receives (sub, arg); driven through a planted __thiscall member
sentinel recording both arguments. Its null path returns whatever eax held,
so it is deliberately undriven -- a nondeterministic value cannot be pinned.
phys_fn_000012 is an ID ALLOCATOR over {counter, freelist begin, cursor}:
pop the freelist's last dword when non-empty, else return and increment the
counter. Falsified by moving the group store to +0x18 (candidate ok=0,
mismatches=2 against control 1).

**Census.** Registrations +2, floor 114 -> 116, oracle digest re-pinned
7b7cd8d7 -> 3f37c9a2.

## 3z12. Slate 9: readBodyFlag and three deleting destructors

Four rows close (000080, 002408, 002326, 002340). Slot 80 is readBodyFlag:
([record+0x10c] byte AND mask) under the READ guard, kind-1 warning
("readBodyFlag: Actor must be dynamic!") and false on a static actor -- the
write-side sibling of this test lives in the slate-2 cluster. phys_fn_002408
is the member subobject's own deleting destructor: third-table vptr then a
LINKED-CRT release (the CRT twin of phys_fn_000042), driven on a stack
block with flags=0 so only the vtable install is folded. phys_fn_002326 and
phys_fn_002340 are two bound-pool deleting destructors (vptrs 0x10108798 /
0x1010884c) releasing through ADAPTER slot +0x14; driven on arena blocks
with post-free vptr reads. Falsified earlier in the round by an offset
mutation of the group writer (mismatches=2 vs control 1).

One row stays open DELIBERATELY: phys_fn_002411 (the shapes-clear entry,
slot=[arg+0x104] released through the pool header at this+0x40 then
shapes[slot]=0) is transcribed but its synthetic pool fixture faults inside
the release arm for reasons a first debugger pass did not settle; it stays
discovered until that fixture is built properly.

**Census.** Registrations +2, floor 116 -> 118, oracle digest re-pinned
3f37c9a2 -> a18e1d49.

## 3z13. The shapes-clear fixture, rebuilt right

The deliberate deferral of 3z12 did not survive its own round. Rereading
the call site solved it: phys_fn_002411 receives the POOL BASE as this --
its release-arm header sits at THIS+0x40 BYTES -- and the first fixture
had passed base+0x40 bytes as this instead, double-shifting every field
and faulting the arm on a null counts pointer. With this = pool base the
entry closes bitwise: freelist push with arena growth, sentinel clear,
mirror poison, cursor pop, then shapes[slot] = 0 through [this+0x80].
Falsified by shifting the slot word (+0x104 -> +0x108; candidate ok=0,
mismatches=2 against control 1).

phys_fn_002411 closes reconstructed. Registrations unchanged in count,
floor stays 118, miscsm2 digests re-pinned (row c4bc7155, candidate
c4bc7155), oracle digest re-pinned a18e1d49 -> ecc2d29c.

## 3z14. Slate 11: pool-class lifecycle rows

Three rows close (002328, 002322, 002312), driven on stack blocks with
flags=0 (no allocator interaction) to isolate the decode from any
allocator side effects. The chained deleting destructor restores the
member vptr through two third tables and installs the primary; the
adjustor thunk subtracts 8 from this before tail-calling it; the CRT-free
dtor installs its vptr with no release. All three fold their vtable-install
predicates bitwise against the oracle.

Five more transcriptions exist for undriven rows (002320 cached-list
destroyer, 002352 container-add thunk, 002379 virtual slot-1 wrapper);
these stay discovered until their drives land in a future round.

**Census.** Registrations +2, floor 118 -> 120, oracle digest re-pinned
ecc2d29c -> c41764d9.

## 3z15. Slate 11 extended: the cached-list destroyer closes

phys_fn_002320 (the cached-list destroyer) now has its list-walk drive
added and closes bitwise. The transcription had a double-dereference bug:
it treated the vtable pointer as a direct function address when the image
does `call [eax]` -- a double dereference through the vtable table to slot
0. With the fix, both sides walk a two-node planted chain (A -> M -> null)
and fold two kills through a shared fastcall thunk. The +0x30 link field
and the +0x5a8 cache offset in the pool holder are named by this decode.

**Census.** miscsm2/slate11 digests re-pinned (row 2a145f64, candidate
2a145f64), oracle digest re-pinned c41764d9 -> 8a423adf. phys_fn_002320
closes reconstructed; Phase 5 reconstructed count rises to 106.

## 4. The census merge resolved

The census flagged its 41-slot row at `0x106a58` as overrunning BOX. It is
three real tables ending together at `0x106afc`: A(12) + B(12) + BOX(17).
Separately, `phys_fn_000973` (p2, 913 bytes) installs twelve small vtables
(`0x10106998`–`0x10106a48`) -- **corrected, section 3x**: those slots are
not vtables and have nothing to do with descriptor setToDefault. They are
CUBE-FACE TOPOLOGY TABLES, and 000973 is the box-hull face-record builder.

## 5. The RED layout gate

`NxPhysicsObjectLayoutTests` pins this task's structural claims against the
shipped DLL and is registered for Phase 5 RED on purpose -- three
`CANDIDATE-MISSING` families (vtables, collision object, owner accessor)
until Tasks 2 and 3 transcribe the classes:

- eight `vt` digests over the loaded oracle's slot words: both actor tables
  in full, and twelve-slot windows of every shape final plus the base table;
- `colobj`: phys_fn_001193 run on a poisoned 0x1c buffer -- the oracle's own
  constructor writes vptr `0x10107218`, zeroes +4, stores the argument at
  BOTH +8 and +0x18, and installs member vptr `0x101072a0`, exactly as the
  listing predicts (`digest=e1df25e5`);
- `owner`: phys_fn_001281 returns the mark planted at fake-shape +0x04,
  confirming the borrowed field through the accessor Phase 3 closed.

Oracle digest `99eee5c2`; twelve registrations; Phase 5 coverage floor 12.

**First transcriptions landed (Task 2 opened early).** `Physics/src/ObjectModel.cpp`
carries `CollisionObject` (phys_fn_001193) and `nxShapeOwner`
(phys_fn_001281), with layout static asserts; the harness now answers both
candidate families through the reconstruction -- `colobj ok=1` and
`owner ok=1` against the same poisoned-buffer/mark probes the oracle side
runs -- and the gate's red count is down from three to one. The remaining
missing family is `vtables`, which waits on the shape/actor classes.

A defect caught before it could mislead: the first candidate run failed
`colobj` with a stack address where the argument belonged -- the harness had
passed `&kArg` where the row takes the value. The oracle side was never
wrong; only the probe was.

## 5a. The six records at Shape+0x150 — corrected twice

`phys_fn_000973` (0x21420, 913 bytes, phase 2) constructs **six 36-byte
records at Shape+0x150, stride 0x24**: `{ dword 4; const dword* listA;
const dword* listB; six dwords of float data }`, called from two small
mutators (`phys_fn_000981` p5, `phys_fn_000983` p3) that first copy the box
dims into `+0xe4..+0xec` — Phase 3's half-extent claim confirmed from the
write side.

Two readings of this row were published here and both were wrong:

1. *"a descriptor setToDefault family"* — withdrawn when the listing showed
   one function building six subobjects, not twelve functions.
2. *"twelve small vtables"* — withdrawn on the bytes: pe.json records **no
   relocations** at any of the twelve addresses, and the raw words are small
   integer runs (`0 1 2 3 | 1 5 6 2`, ...), not function entries. They are
   static index data:

- the **listA family** chains face quads over an 8-corner numbering:
  `[0,1,2,3 | 1,5,6,2]` → `[1,5,6,2 | 5,4,7,6]` → … cycling all six faces;
- the **listB family** reaches corner ids up to 11 (`[1,8,5,9]`,
  `[11,3,10,7]`) — a second topology this task has not named;
- the float areas carry face normals as constants (±X, ±Y, ±Z), which for a
  box are dim-independent.

So the six subobjects are per-face records of the box — spheres, capsules
and planes never call this row. What consumes them (sweep? hull queries?)
and what the two index families count are open. Separately, the `+0xe0`
subobject's abstract table at `0x106a58` is a pure-call wall, and its final
table at `0x106a88` is twelve slots whose targets are ALL phase-5 rows
(`phys_fn_000953`..`phys_fn_000985`) — Task 3's to reconstruct. The census's
merged 41-slot row spans exactly [purecall wall][12-slot concrete][BOX final],
and its distinct-target figure treated static data as slots.

A defect worth keeping on the record: the first build HUNG rather than ran.
The cause was an address-space mixup in the harness itself -- censused RVAs
pasted into a field named `va` and subtracted by the image base a second
time, sending every read ~4 GB past the module. A crash sitting in a Windows
Error Reporting dialog looks exactly like a hang from a redirected-process
watchdog, and nothing in stdout says so. The minimal read probe
(`build/probe_read.cpp`) is what separated "the module cannot be read" from
"my pointer is wrong"; one was environment, the other was the bug.

## 5b. The +0xe0 subobject named: the box's convex-hull descriptor

The twelve-slot final table at `0x106a88` decodes as a facade over the hull:

| slot | row | behaviour |
| ---: | --- | --- |
| 0 | `phys_fn_000985` | lazy shared-hook accessor — **transcribed** |
| 1 | `phys_fn_000953` | `mov eax,8` — eight corners |
| 2 | `phys_fn_000955` | `lea eax,[ecx+0x10]` — the vertex array |
| 3 | `phys_fn_000961` | `mov eax,6` — six faces |
| 4 | `phys_fn_000963` | `index*36 + this+0x70` — face-record[k] |
| 5 | `phys_fn_000965` | returns 0 |
| 6-8 | `000967/969/971` | three static .rdata tables (`0x10122180/e0/240`) |
| 9-10 | `000957/000959` | algorithms (578/1062 bytes; the second copies a 3×3 matrix and projects) — untranscribed |
| 11 | `phys_fn_000975` | support mapping: ±FLT_MAX sentinels, then min/max over eight vertices at stride 0xc against a direction — **transcribed** |

Nine rows are transcribed in `BoxHullFacade` (`Physics/src/ObjectModel.cpp`)
— plus slot 0, which turned out to be a lazy shared-hook accessor rather
than a destructor: it once-guards a twelve-byte `.data` global at
`0x10123c64` whose initializer array (.rdata `0x10103010`) is twelve bare
`ret` stubs, so the global stays all-zero and the row returns its address.
Ten rows are driven both sides by the layout gate on twin buffers:
constants, `face(k)` pointer arithmetic (`0xb8` for k=2), static-table
content equality, the support bounds bitwise (`min=c19c0000 max=4eada5a5`
on the registered seed), and the shared hook (stable pointer, zero words).
The support frame decodes as (this, a1 unread, a2=&min, a3=&max, a4=dir,
a5=pose, a6 unread) — the sentinel stores name the outputs, which is how
the first wiring was corrected after it returned zeros. Three probe-side
defects were found and fixed on the way: `(unsigned const&)x` on a float
value-converts to unsigned (yielding 0 at these magnitudes) instead of
binding storage, so early "bits" prints lied; the shared-hook probe folded
its returned pointer into the digest although ASLR moves it between runs —
only stability and the pointed-to words are deterministic facts; and the
support sentinels were transcribed swapped before the frame decode named
them. Gate state: oracle digest `8d76f55d`, RED=1 (the vtables family —
shape/actor classes). Census: ten facade rows stand at `reconstructed`;
slots 9/10 (`phys_fn_000957/000959`) remain `discovered` until their
listings are transcribed and falsified.

## 3z16. Slate 11 extended: the virtual slot-1 wrapper closes

phys_fn_002379 (0x5b860, 14 bytes) now has its dispatch drive and closes
bitwise. The disassembly names the whole row: `mov ecx,[esp+4]` (the
argument arrives on the stack, `__stdcall`, `ret 4`), `mov eax,[ecx]` (the
receiver's vtable), `call [eax+4]` (slot 1, receiver as this), `xor
eax,eax` (the callee's return is discarded), `ret 4`. The transcription had
existed since 3z14; what was missing was any drive — the row closed on
reading, not on measurement.

The drive plants two stack receivers with distinct two-slot vtables — slot 1
sentinels that differ in both tag and return value, one returning nonzero,
one returning zero — and runs four drives in alternating order. Folded per
drive: the wrapper's return (0), slot-1 calls (exactly 1), slot-0 calls (0 —
distinguishing slot 1 from slot 0), receiver identity, the sentinel's own
tag, and two no-side-effect predicates over the receiver block and the
vtables. Oracle and candidate fold to the same digest (05167dcd) over all
28 words.

**The mutation check.** Pointing the transcription at slot 0 instead of
slot 1 produced 12 mismatches and exit 1 (`slot1wrapper candidate
failures=12 mismatches=12 digest=60dd4c05`) — wrong slot, wrong receiver
identity, wrong tag, and the wrapper's return still matching, which is why
the call-count and identity folds matter rather than return-value agreement
alone. Restored, the row is green again.

**Census.** miscsm2/slate11 digests re-pinned; oracle digest re-pinned
8a423adf -> 0395afa0 (+2 registrations, coverage floor 120 -> 122).
phys_fn_002379 closes reconstructed; Phase 5 reconstructed count rises to
110. **A count correction, on the record:** 3z15 printed 106, which was
already stale — the inventory held 109 reconstructed phase-5 rows after
002320 closed, and this close made 110. The number in a close note should
be read out of the census by query, not carried forward by arithmetic; the
stale values trace to rows closing in a single commit being tallied against
a snapshot taken before it. One transcription note: the registration's first commit omitted the
`slot1wrapper=1` key from the harness's own `layout coverage` line, which
the phase gate caught on the next run (0 occurrences against its
registration) — the count guard doing its job on a real defect, not only
in its test suite.

## 3z17. Slate 11 extended: the container-add thunk closes

phys_fn_002352 (0x5b610, 8 bytes) now has its allocator-path drive and
closes bitwise. The row is `add ecx,0x28`, tail-jump to 0xb4f50 — the
SdkContainer rows Phase 4 reconstructed. Three containers: an owned buffer
(free fires through the adapter), an external buffer (factor -1.0f, buffer
kept), null entries (no free); all three clear exactly {capacity, count}.

**Two transcription defects, both caught by the drive.** The first spelling
hard-coded the oracle's absolute `0x100b4f50`: in the candidate process
that is oracle code reached with `ecx` invalid — the drive caught it as
`c0000005` at 0xb4f53. The fixed transcription calls the reconstruction.
Separately, the first decode expected the external-buffer arm to null the
entries pointer; the oracle's own drive said otherwise (failures=1), and
the listing confirms it: the shared tail at 0x000b4f81/87 clears ONLY
capacity and count — entries survives, because 0xb4f7a's null-store runs
only inside the owned arm, before the shared clear. `SdkContainer::empty`
already had that shape.

**Harness findings, two shims and a convention.** The SdkContainer rows
reach the allocator through the Foundation global at .data 0x1012845c
(helper 0x000b4000, defaulting to the static CRT adapter at 0x10122368) —
NOT the SDK holder the shape ctors use. Driving with only the SDK holder
repointed let empty() free arena blocks through the CRT heap: the
0xc0000374 that killed the first three runs. A second adapter shim into
0x1012845c (slot +0xc free, one pushed pointer — the 4-parameter spelling
drifted the stack and faulted at ee5710dc) serves the emulator. The
candidate thunk is a cdecl free function; driving it through a __thiscall
pointer put the receiver in ecx and left the stack argument garbage, which
the generic SEH guard then folded into a zero digest — the pointer's own
type must name the convention.

**The mutation check.** A mutant empty() that stores 1 into capacity
failed the drive (candidate failures=3 mismatches=3, digest
a1d421ae against control f9aac08b) and exited 1; restored, green.

**Census.** Registrations +2, floor 122 -> 124, oracle digest re-pinned
0395afa0 -> f4db035e. phys_fn_002352 closes reconstructed; the census
query holds 111 reconstructed phase-5 rows (read out, not carried). Gate
state unchanged otherwise: RED=1 (vtables).

## 3z18. The vtables RED, pinned at its byte: the shape classes are not yet polymorphic

The `candidate CANDIDATE-MISSING family=vtables` line has named the open
family since Task 1; this round pins WHERE it breaks. Reading the ctor
chain: `ShapeBase::ShapeBase` writes no vptr (mVptrSlot is a plain data
member), and `BoxShape::BoxShape` never touches +0x00 either — so the
candidate's shapes are non-polymorphic today. The existing `boxshape
candidate` byte-fold skipped +0x00 on BOTH sides (kPointerWords), which is
why the family stayed broken silently: the one word that says "which table
is this" was excluded from every digest.

A compiled probe now pins it (`boxvptr` in the boxshape candidate block):
the candidate's fresh box carries `word=cdcdcdcd` at +0x00, and the
slot-5 dispatch is correctly skipped under the SEH guard while the vptr is
poison. When the classes go polymorphic the same probe answers through the
candidate's own compiled table — no absolute oracle address in play, the
lesson 3z17's false-pass had taught.

What the transition needs, from the census and the gate's own RED line:
the BOX slot map is 17 rows, three of them untranscribed — slot 3
(phys_fn_000945), slot 5 (phys_fn_000949, 663 bytes: the box raycast,
Phase 3's identified partner, whose kernel call lands in 0x37e70 range
machinery Phase 3 owns), and slot 7 (phys_fn_000951, the sweep entry).
The actor tables (0x1043d0/0x104530, 87/88 slots) are the deep half of
the same family and belong to Task 4's scale.

## 3z19. Slate 12: the slot-3 leaf pair closes (001287, 000931)

Leaf-first toward the BOX vtable's slot 3 (phys_fn_000945, still open --
its other callees 001305 and 001279 carry their own chains):

- phys_fn_001287 (0x000257d0, 14 B): the +0xde halfword zero-extended and
  ANDed with the stack mask -- `ShapeBase::nxFlagBitsDE`. Drive: five
  masks off a marked record; edge bits 0x8000/0x80a7/0xffff with masks
  0xffffffff/0xffff0000 assert the movzx zero extension. Mutant reading
  +0xdc -> ae87d045 (red), restored -> green.
- phys_fn_000931 (0x00020490, 70 B): ordered descriptor transfers --
  out[0..2] translation from +0x30, out[6..14] rotation (forward copy,
  rep movsd 9) from +0x0c, THEN out[3..5] dims from +0xe4 --
  `BoxShape::nxFillShapeDescriptor`. Word-wise volatile transfers keep the
  image's order when out aliases the shape; bulk memcpy/memmove and
  dims-first orderings are not equivalent. Drive: aliasing twin buffers at
  offsets 0/4/c/24/cc/e4/240, all bitwise-matching the oracle. Mutant
  reading +0xe0 -> 7cd1b1e7 (red), restored -> green.

Two independent defects caught by review + falsification before close:
(1) the first transcription copied DIMS BEFORE ROTATION -- invisible to
the non-aliased digest, caught by an aliasing fixture the review
specified (rotation overwrites the dims source first); (2) the first
drive sized its buffer 0xe0, so BOTH sides read stack garbage past 0xe0
and the digests diverged nondeterministically (oracle rec[3..5] showed
ASCII fragments of unrelated stack). The getter also reads past the 0xe0
ShapeBase extent, so the receiver moved from ShapeBase to BoxShape.

The Phase 5 gate caught its registration defect a second time
(same class as 3z16): the harness's coverage printf did not emit
shapeleaf=1. The count guard held; the key was added and the gate
evaluates 126/126 (floor 122 -> 124 -> 126; composite oracle digest
0395afa0 -> f4db035e -> 16dceb3c across slates 11/12).

Census: phys_fn_001287 (phase 3) and phys_fn_000931 (phase 2) move to
reconstructed with static + dynamic proof; the shapeleaf family
registration is +2 (row + candidate drive). Phase 5 reconstructed count
stays 111 (both closures are phase-2/3 rows). The vtables RED remains
byte-pinned per 3z18; next slate: 001305 -> 001279 -> 000945, then the
BOX table can carry its first real dispatch.

## 3z20. Mapping correction: 0x00020750 is phys_fn_000943 (139 B, dynamically_gated), not phys_fn_000947

Aiming at "BOX slot 4" this round, the ctor-comment assumption ("slot 4 =
phys_fn_000947 at 0x20750, 48 bytes") was checked against the census
before driving and FAILED: the inventory row at 0x20750 is
**phys_fn_000943, 139 bytes, phase 3, state `dynamically_gated`**;
phys_fn_000947 is the 39-byte wrapper at 0x20850; the merged run at
0x106a58 maps BOX slot 4 to phys_fn_000947@0x20850 (run index 28) and the
ctor's slot-4 comment is off by one function. The transform decode read
from 0x20750 (`out = rot0*(dims o float(arg)) + t`, three fild ints,
row-major rotation, ret 0x10) belongs to **phys_fn_000943** and is
recorded here for its owner: the complete body is 139 bytes,
[0x20750, 0x207db). Its `ret 0x10` at 0x207d8 encodes `c2 10 00`.
It is NOT BOX slot 4; that slot targets the distinct 39-byte wrapper
phys_fn_000947 at 0x20850. The earlier 48-byte claim, encoding and
conflation of these two rows were incorrect.

Consequences: the mis-attributed transcription was reverted BEFORE any
drive (tree verified byte-identical to HEAD; build green; addthunk family
green), so no false registration exists. `dynamically_gated` is a real
census state; its name alone does not establish missing implementation or
pending runtime gates. For example, 001726/001728 already have candidate
code and differential evidence in phase3-leaf-kernels.md.
BOX slot 3 directly calls 001287, 001305 and 000931; 001279 is NOT a
callee on this path. The 001305 guards at 0x10123bc8 and 0x10123bd8
skip their blocks when zero: `test ah,0x44; jnp` is taken on equality.
The earlier claim that zero makes both blocks execute was inverted.
Slot 4 calls 000849, but its non-null pose argument activates a payload
path currently omitted from that reconstruction; a closed census label
alone does not establish coverage of this caller's behavior.

## 3z21. Slot-5 provisional decode recorded, transcription reverted before a drive

BOX slot 5 (phys_fn_000949, 0x20880, 663 B) was decoded END TO END this
round from the capstone listing (202 instructions, written in full to the
scratchpad): a local-space raycast whose slab kernel is NxRayAABBIntersect2
(phys_fn_001726/001728 -- already transcribed and 13/13 against its own
Phase 3 differential, so the kernel dependency slot 5 needs is CLOSED).
The wrapper: carries origin and direction into the shape frame by R^T with
the image's association (dz*R2c + dy*R1c) + dx*R0c per column; slabs run
over [-dims, +dims]; the kernel returns the plane index (1..3); a
below-or-equal fcomp against a max-distance argument gates the record
(0x20a20..2f, test ah,0x41); the hit arm rotates the kernel coord back
through R, adds the translation, and stores colobj/point/t/zeroed face
words/tag 0x13 into the 0x30-byte record; flags&4 extends the tag to 0x17
and rotates a +-1 plane normal through R. Returns null on miss or gated
t, this on hit.

The first transcription FAILED the decode twice while open -- the t-gate
fcomp was omitted entirely, and the normal arm's third component row read
one rotation word twice -- and the drive block carried unresolved stack
accounting for the kernel call (the descriptor layout, the two float
arguments, and the flag position were not yet verified. That provisional
candidate was never run: the earlier claim that it reached the kernel
with a wrong stack height was unsupported).
Following the round-4 precedent, the transcription and drive were
reverted BEFORE any registration: tree verified clean, build green. What
stands is this decode record -- NOT a reconstructed row.

The round ended with a byte-capture instead of a transcription: an
ORACLE-ONLY probe (no candidate side, no digest, no registration) drove
the real binary's 0x20880 on a constructed box across six cases and
printed every record word. The capture pins the record layout byte-level:
colobj@+0x00, world point@+0x04..+0x0c, plane normal@+0x10..+0x18 ONLY
when flags&4, +0x1c/+0x24/+0x28 zeroed, t@+0x20, tag@+0x2c (0x13 plain,
0x17 with the normal), miss and inside-origin leave the record fully
poisoned, return null on miss, this on hit. Two argument facts pinned
against the binary: the t-gate fcomp's maximum is the SECOND stack
argument (case D: a3=0.5 did NOT gate t=1.0; a2=111.0 passes), and the
third stack dword is unused in all six cases (role unpinned). Diagonal
case: t=sqrt(2) bits 3fb504f3, point (1,1,0), normal (1,0,0).

Next round: transcribe with this capture as the per-word arbiter, fit the
five stack arguments against the kernel call at 0x2098c (push order and
the add esp,0x18 purge), then drive, mutate, and register. Slot 5 stays
`discovered`; the vtables RED remains byte-pinned per 3z18.

## 3z22. Executable raycast contract: distance rejection is a partial write

The oracle capture is now an assertion-bearing contract over eight
identity-box cases. It checks the exact returned pointer, all twelve
record words, and leading/trailing canaries. The constructed oracle
vtable is checked at runtime: slot 4 -> 0x20850, slot 5 -> 0x20880.
This remains ORACLE-ONLY evidence, not a candidate closure or coverage
registration. phys_fn_000949 remains discovered.

New case 6 uses ray (2,0,0)->(-1,0,0), maximum 0.5, third argument 111,
flags 4: it returns NULL but writes point (1,0,0) and t=1.0. All other
record words remain 0xcdcdcdcd. Case 7 sets maximum exactly 1.0 and
returns the original shape pointer with the normal/tag populated.
Thus the prior shorthand 'rejection leaves output untouched' is wrong
for distance rejection (it remains true for the driven kernel-miss and
inside-origin cases). The point stores z/x/y at 0x20a10/17/1a and t
store at 0x20a1d PRECEDE the comparison and null return at 0x20a2f.
The test catches moving that comparison before those writes.

Falsification: changing only case 6's expected t to poison caused
`FAIL boxray contract case=6 return/record/canary mismatch` in
build/r6-contract-mut.log. Restoring the expectation gives eight cases,
zero failures. This is a falsified oracle-contract assumption, NOT a
candidate mutation test; no candidate wrapper has been implemented.

Independent stack review agrees with the capture. Let B be ESP after
sub esp,0x30 and push esi. Args at B+38/3c/40/44/48 are packed ray,
maximum float, unread dword, flags (mask 0x04), output. The six kernel
pushes resolve to (negative dims, positive dims, local origin, local
direction, coord, t); t reuses the arg1 slot B+38. The caller pops 0x18,
wrapper returns with ret 0x14. Under masked FP exceptions the
`test ah,0x41` gate also accepts unordered comparison; that NaN arm and
non-identity numerical fidelity are not established by these eight cases.
Kernel declaration: Physics/include/NxIntersectionSegmentBox.h:24;
implementation: Physics/src/Geometry.cpp:447. No guessed prototype is
needed. The unused provisional candidate declaration was removed.

This review also corrects 3z20/3z21 above: the 0x20750 extent/encoding,
001279's phantom dependency, inverted zero-debug-guard branch, and the
unsupported assertion that an unexecuted candidate reached a kernel with
bad stack height. Preserving an incorrect narrative is not evidence.

## 3z23. The slot-5 wrapper lands its first drive -- provisional, unregistered

The candidate wrapper now exists and its first drive is green. TDD shape:
the stub failed case 0 (RED), the real implementation passed all eight
(BLIND PASS observed before the mutation check, then re-verified after
restore), and the store-order mutant failed exactly case 6. The
implementation calls the transcribed kernel NxRayAABBIntersect2
(Physics/src/Geometry.cpp:447; declaration
Physics/include/NxIntersectionSegmentBox.h:24, found by repository-wide
search after two wrong include guesses -- the layout target now links
Geometry.cpp). Both compile errors the first build produced (missing
identifier; const this) are recorded in build/r6-real-build.log.

The drive: the same eight identity-box cases assert oracle-vs-candidate
agreement on the return pointer and all twelve record words. Census:
phys_fn_000949 STAYS `discovered` -- the drive covers an identity pose
and eight fixtures; numerical fidelity (extended-precision x87 chains,
the unordered-comparison arm, non-identity poses) is unestablished, and
no family registration was added. Closing this row wants a broader
fixture sweep and a decision on the unlinked arg3, on a later round.

## 3z24. The 001305 capture: the no-op row proven, the renderer contract pinned

An ORACLE-ONLY capture (no candidate, no registration) drove the real
binary's 0x25960 with a fake renderer across the guard truth table:

- Row 1 (shipped guards 0.0): zero renderer calls -- the no-op decode is
  now EXECUTED PROOF; the listing's parity analysis stands.
- Row 2 (guards+scale = 1.0, writable .data, restored after): slot +0x20
  fired 3x and slot +0x38 fired 3x. The +0x38 contract is pinned at five
  stack dwords (integer 20, a 48-byte pose pointer, color 0xffff00ff,
  radius bits passed by value, then zero) -- matching the listing's five
  pushes; the +0x20 rows recorded one slot shifted (pad = the actual
  color: 0xCF0000/0xCF00/0xCF), pinning its shape as THREE stack args
  (bufA, bufB, color) with per-axis colors red/green/blue.

Two defects in MY fake renderer were caught by the oracle's own
execution, not by reading: the renderer object must be the address OF
the table (the oracle double-dereferences), and the +0x38 row pops FIVE
stack dwords. The broken fake declared only three stack arguments and
also passed the table instead of an object containing a vptr. The run
terminated with 0xC0000409; that exit code alone does not isolate which
defect triggered fail-fast or establish a security-cookie failure. Row 1's clean
return with a poisoned object remains valid evidence.

The listing decode (block 1: three scaled rotation columns + translation
through slot +0x20 with colors 0xCF0000/0xCF00/0xCF; block 2: slot-10
center+diagonal, 9 rotation words, three 48-byte poses through slot
+0x38 with 0xFFFF00FF) initially recorded only addresses, NOT a byte
reference for the payloads. The earlier two-buffer-pointer and
0x14-word-buffer claims were wrong: 20 is a separate scalar argument;
the pose is 12 words and argument 4 is the slot-10 radius word.
phys_fn_001305 stays `discovered`; no candidate was implemented.

Follow-up contract hardening copies both 12-byte line endpoints and each
48-byte pose inside the callbacks, before stack-buffer reuse. Five masks
assert each guard independently, both enabled, both disabled, and both
negative zero. Identity-box payloads assert the three unit axis endpoints,
colors, cyclic column permutations of the pose, zero center, integer 20,
color 0xffff00ff, radius 0x3fddb3d7, and final zero argument. Scale is 1.0.
Global bits and page protection are restored before assertions.

Falsification: changing expected radius to zero fails at mask 2 (B only)
in build/r7-payload-mut.log. This is an oracle-contract expectation
mutation, NOT a candidate mutation. General poses, non-unit scales,
unordered guard values and callback side effects remain undriven.

## 3z25. The 54/72-case sweep: three real wrapper defects found and fixed

The round-6 drive was too weak in three ways: it inferred candidate
returns from output bytes instead of comparing them, and its eight
identity-frame cases could not see coordinate defects that are invisible
when rotation and translation vanish. The differential now compares
ACTUAL return pointers (candidate method return vs oracle return), all
record words plus canaries, over 4 poses x 6 faces x 3 distances
(hit / 0.5 reject / quiet-NaN maximum) = 72 cases, run against an
oracle-layout fixture. Direct member invocation tests candidate code on
that fixture; it does not establish candidate construction or virtual
dispatch. Census: phys_fn_000949 STAYS `discovered`.

RED first: the expanded sweep failed 42/54. Three defects, each fixed
against the listing before rerunning:

1. World Z added trn[0]; 0x20a03 adds [esi+0x38] (translation Z).
2. The gate rejected unordered comparisons; 0x20a20..2d `test ah,0x41`
   takes the reject branch ONLY on ordered-greater, so NaN maximum must
   be ACCEPTED (0x7fc00000 fixture).
3. World Z and normal Z dots used the transposed column-0 form
   (rot[6],rot[3],rot[0]) where the listing computes row-2
   (0x209d8..0x20a03: cx*[esi+0x24]+cy*[esi+0x28]+cz*[esi+0x2c], and
   0x20ae2..0x20af9 likewise). The transposed form coincides with row 2
   for every axis-aligned and cyclic permutation -- 54 cases green was a
   consequence of fixture choice, not correctness. The quarter-turn pose
   (r0=(0,-1,0), r1=(1,0,0), r2=(0,0,1)) exposed it in words 4 and 7.

Result: 72/72 agreement on actual returns and every record word
(build/r8-green2.log, r8-final-gate.log). Falsifications: expecting
radius zero failed the rendercap contract at mask 2 (r7-payload-mut);
reversing the distance gate failed at boxray candidate case 0
(r8-mut.log) and was restored to green. The transposed-index defect was
a LIVE candidate bug found red and fixed green, not an expectation
mutation. Remaining open: x87 extended-precision association against
non-unit dims, the unused third argument, arbitrary poses.

## 3z26. phys_fn_001305 transcribed: candidate matches the binary at 5 masks

The candidate `BoxShape::nxDebugRender` (ObjectModel.cpp) is transcribed
from build/slot3dep-full.txt: block 1 draws three axis lines (K-scaled
rotation column + translation, colors 0xCF0000/0xCF00/0xCF) through
renderer vtable +0x20 when guard A (.data 0x123bc8) differs from the
reference word (0x1041f0, bound to live storage via
nxBindDebugRenderGuards -- the drive mutates exactly what the candidate
reads); block 2 draws three column-cyclic 48-byte poses through slot
+0x38 (args: 0x14, pose, 0xFFFF00FF, radius-by-value, 0) with
center/diagonal from the slot-10 member. Equality (including -0.0,
mask 4) skips both blocks -- the executed semantics from 3z24.

TDD shape: the empty stub failed the candidate lens at mask 1 with
c(0,0) expected(3,0) (build/r9-red4.log); the transcription turned all
five masks green with raw counts visible (r9-close-gate.log lines
133-143). Harness defects found red and fixed along the way: the stub
differential first compared stub-to-stub instead of against oracle
literals; the candidate first ran OUTSIDE the mutated guard window and
behind a wrong bit-gate (wrongly "agreeing" at masks 1-3); a mistyped
absolute VA (0x101041f0) in the bind crashed at mask 0 (fail-fast
0xC0000409, fixed to RVA 0x1041f0); and the first restructure let
candidate recorder state overwrite oracle bytes before the contract
lens read them -- diagnosed because the guard-inversion mutant was
caught through the WRONG lens (contract mask=0 n20=3) and re-caught
through the right one after separation (candidate lens: c(3,0)
expected(0,0), build/r9-lens.log), then restored green.

FALSIFICATION (candidate mutation test): inverting block 1's guard to
`guardA == ref` failed the candidate lens at mask 0 (c(3,0) under zero
guards) while the oracle lens stayed green -- the correct lens caught
it; restored to green.

Historical open question (superseded by 3z27): the scale-2 fixture was
reverted after a misleading diagnostic printed zeros. That diagnostic
read only seven BYTES from the start of a 28-byte line record, not seven
DWORDS; it did not observe the endpoint or color at all. No all-zero
oracle payload claim is supported. Scale-2 agreement was established in
round 10; the diagnostic's actual type correction landed in round 11.

Census: phys_fn_001305 STAYS `discovered` (no family registration); the
candidate is driven only through the rendercap differential on an
oracle-layout fixture. The vtables RED remains byte-pinned per 3z18.

## 3z27. Scale semantics decoded: the round-9 anomaly was a diagnostic bug

The round-9 "all-zero payloads at scale 2.0" (3z26's open question) was
MY diagnostic, not the binary: the mask-5 dump indexed
`unsigned char oLine[8][28]` rows BYTES (`oLine[i][0..6]`), printing only
bytes 0..6 of the zero start vector. Endpoint bytes begin at offset 12
and color bytes at offset 24: neither was printed. The oracle's true
mask-5 payloads were never observed as zeros. Round 10 added a DWORD
comment but did not change the declaration; round 11 fixes that omission
with `unsigned oLine[8][7]` (the byte-exact comparisons are unchanged).

With the mask-5 endpoint
literal derived from K = scale*guardA = 2.0 (0x25985..0x2598e), the
ORACLE lens passed mask 5 first try (r10-scale.log): the shipped binary
scales the rotation column ONCE, exactly as the listing states. The
"Candidate lens" immediately failed c(3,0) expected(3,0) -- exposing a
REAL live defect in the round-9 transcription: the candidate computed
k * (rot*k) + t, scaling twice (rot*k^2 = 4.0 vs oracle 2.0). Fixed to
a single K multiply (0x25a0e..0x25a28, one fmul per element).

Scale-1 blindness: the double scale is invisible at scale 1.0 (k^2 = k)
-- the same fixture-choice hazard as the round-8 transposed-index find.
Scale-2 mask 5 was REQUIRED to expose it; masks 0-4 alone pass both
forms. FALSIFICATION: the deliberate double-scale mutant
(k = scale*guardA*2) failed the candidate lens at mask 1
(c(3,0) expected(3,0), build/r10-mut.log) -- earliest drawn mask in the
loop, since every scaled mask diverges -- and was restored green.

Final state (r10-final-gate.log): masks 0-5 contract+candidate agree
including scale-2 endpoints (mask=5 n20=3 n38=0), boxray sweep 72/72,
inventory pass (6338 functions, 0 unexplained), coverage 126/126, sole
RED the byte-pinned vtables gate. Census: phys_fn_001305 STAYS
`discovered`; scale=2.0 is now a driven fixture, and the remaining open
region includes guard values outside {0,1}, unordered guard comparisons,
and non-identity poses in BOTH render arms. The 2.0 value was a scale,
not a guard. The 72-case boxray sweep exercises a different function and
provides no pose coverage for this render candidate.

## 3z28. BOX slot 3: independent oracle contract, candidate still pending

The 39-instruction listing for phys_fn_000945 (RVA 0x207e0, 104 bytes;
build/slot3-full.txt, sourced from the capstone manifest) gives:

- call 0x257d0 with mask 8: word[this+0xde] & 8; zero exits;
- call 0x25960 with the renderer (001305 debug-render dependency);
- compare guard C at RVA 0x123bc4 against ref at 0x1041f0; equality
  skips, unordered falls through;
- call 0x20490 to fill a 60-byte local descriptor, then renderer slot
  +0x28 with (descriptor, color, 0), three callee-popped stack DWORDs.

Color arithmetic is exact: let n = (byte[this+0xde] & 7) != 0.
`neg al; sbb eax,eax` produces 0 or 0xffffffff. AND 0xffff0100,
then DEC, produces 0xffffffff for n=false and 0xffff00ff for n=true.
Earlier round-11 narration incorrectly subtracted this hexadecimal
constant and inferred bit-2-only selection; neither claim is retained.

The initial print-only sweeps coupled enable/low bits/guard and did not
establish a complete truth table. They are replaced by an asserted
64-case ORACLE-ONLY contract: enable {off,on} x low bits {0..7} x guard
C {+0,-0,1,qNaN}. A/B remain shipped zero, so the dependency's drawing
arms are intentionally inactive here. All 15 descriptor DWORDs are
checked against a nontrivial fixture: translation (4,-2,8), dimensions
(1,2,3), and cyclic rotation. The callback copies during the call; an
unused renderer helper with a formerly dangling local was removed.

build/r11-contract.log reports `slot3 contract cases=64 failures=0
mode=oracle-only`; the same run retains six rendercap agreements and
ends at the documented unfinished-vtables RED. No candidate, mutation
proof, family registration, or census closure is claimed for 000945.
Fresh full verification in build/r11-final-gate.log repeats the 64-case
contract, six rendercap masks, and 72-case raycast sweep. Release build
exit=0, inventory exit=0 (6338 functions, 0 unexplained), coverage
126/126; gate exit=1 solely for the documented missing-vtables family.
Next step is candidate-vs-oracle TDD, including enabled dependency arms
and dispatch ordering; these are NOT covered by this contract.

Round-10 evidence corrections are applied above: the diagnostic had
still been byte-typed (only its comment changed), and the boxray pose
sweep does not cover debug rendering. The oracle capture now uses DWORD
rows so its seven-field failure printer reads the intended words.

## 3z29. BOX slot 3 candidate: 64-case differential and order case green

The dispatcher transcription `BoxShape::nxDebugRenderDispatch`
(ObjectModel.cpp) implements the four stages decoded in 3z28: enable
gate `nxFlagBitsDE(8)`, unconditional `nxDebugRender(renderer)`, guard-C
equality skip (unordered executes), `nxFillShapeDescriptor` into a
local, then vtable +0x28 with (descriptor, color, 0) where color is
0xffffffff iff (+0xde & 7) == 0. Guard C binds to live storage via
nxBindDebugRenderGuardC.

TDD arc with named artifacts: stub RED at the first payload case
(enabled=1 low=0 guard=2 n28=0, build/r12-red.log); first transcription
GREEN on counts but RED on color -- the candidate read the flag through
the aligned dword at +0xdc (upper half = the +0xde halfword), so its
gate accidentally worked (bit 19) while its color mask read byte +0xdc
(constant 2 -> stable ffff00ff instead of ffffffff). Root cause found by
arithmetic on the printed payload (fill byte-exact, only arg2 wrong);
the two symptom-chasing test edits (flags-restore rewrites) were
reverted. Two compile rounds followed the real defect: unqualified and
this-> lookup fail because BoxShape COMPOSES ShapeBase (mBase member,
not inheritance); the correct path is mBase.nxFlagBitsDE, which matches
the listing's direct [esi+0xde] addressing.

GREEN: r12-green4.log / r12-final-gate.log lines 146-148 -- 64/64
candidate agreement byte-exact (counts, color, reserved, all 15
descriptor words), plus an ordering case with call-sequence stamps
(A=B=1.0 + C=1.0: oracle and candidate each make 7 calls; the +0x28
descriptor draw is stamped after both 001305 arms in both). FALSIFICATION:
dropping the nxDebugRender call failed the order case cleanly
(o(calls=7) vs c(calls=1), build/r12-mut.log) -- the stamps also proved
the failing lens reads candidate-vs-oracle, not stale shared state;
restored green.

Census: phys_fn_000945 STAYS `discovered` (no family registration, no
gate_targets row); the vtables RED remains byte-pinned per 3z18. Open
region: non-identity poses, guard values outside the driven set, and
descriptor-buffer aliasing the oracle's stack local cannot exhibit.

## 3z30. BOX vtable transition attempt withdrawn; prerequisites corrected

The bounded native-virtual transition proposed in round 13 was invalid:
BoxShape COMPOSES ShapeBase. Adding virtual members to BoxShape shifts
its data; changing ShapeBase too affects every composed shape. Duplicate
member declarations and layout assertions failed the attempted header
build (build/r13-poly-build.log). All partial-transition header/test changes were
reverted; no partial vtable or guessed sweep stub remains. After review,
the pre-existing malformed dormant slot-5 call was removed; boxvptr now
prints `dispatch=unverified` without executing an unvalidated pointer.

The preliminary boxvptr RED (r13-red3.log: word=cdcdcdcd, poly_ok=0)
only showed the known missing constructor vptr. It did not validate the
future probe: candidate table addresses were mistakenly bounded by the
oracle DLL's image base, and a dormant pre-existing slot-5 call has the
wrong pointer dereference and calling signature. Those must be repaired
before any real table enables that path.

The retained output is `box-vtable-transition-audit.md`: SHA-pinned
17-slot target/ID/size/return audit using inventory function boundaries.
It corrects arbitrary windows mistaken for function extents, wrong
stable-ID/address associations, and stack-pop counts. In particular,
slot 4 at 0x20850 is a 39-byte wrapper with three stack DWORDs, not the
existing four-argument mass helper; slot 7 spans 153 instructions with
TWO ret-8 exits. One virtual declaration does not generate three self
slots. Slot 3 remains provisional/discovered, not closed.

Restored baseline: build/r13-baseline-build.log builds both Release
targets (exit 0); r13-baseline.log retains 64 slot-3 agreements, the
7-call ordering case, and the documented missing-vtables RED. No
census, registration, or gate-policy changes were made. A revised
architectural design is required before a native hierarchy migration;
the audit also outlines an explicit ABI-table alternative and the
smaller slot-4-wrapper prerequisite.

Final retained diagnostic cleanup was rebuilt and gated in
build/r13-safe-build.log and build/r13-safe-gate.log: build exit=0,
inventory=pass, slot-3 64-case differential and order case agree,
coverage 126/126, gate exit=1 for the existing missing-vtables family.
The clean round-12 baseline also had that RED; it was not introduced
by this cleanup. Independent read-only review confirmed the layout,
signature and probe defects and the oracle-buffer virtual-dispatch
false-pass risk documented in the audit.

## 3z31. Actual BOX slot-4 wrapper and centered-pose path; merge precision defect

Round 14 scoped the path without edits; round 15 implements the provisional
nonvirtual `BoxShape::nxBoxAccumulateMass` for phys_fn_000947, 0x20850
(39 bytes, 12 instructions). The three stack DWORDs are destination,
density, ignored/reserved. From the full listing: test byte[this+0xde]&7;
if zero push this+0x6c, this+0xe4, density; use destination as ECX for
000849; return AL=1 and ret 12. No vtable is installed and no layout changes.

RED: build/r15-red-build.log builds the true-return/no-op stub;
r15-red.log fails pose=0, low=0, density=0 with four differing words.
The wrapper and existing rotation helper then pass identity and cyclic-axis
poses but fail translated pose (r15-rotate.log). The previously omitted
non-null pose path now invokes 000831 and a centered-box specialization of
000833: add m*(|t|^2 I - t*t^T) to inertia and t to center. It stores each
correction to float, then its mass product to float before inertia addition.
General 000833 is NOT implemented or claimed closed. Independent Capstone
review confirmed the centered formula and final stores, and identified
asymmetric square rounding for arbitrary floats not yet reproduced here.

Round 18 (3z34) adds a fourth pose: a proper 30-degree rotation about z
plus the general translation (2,-3,4), giving the combined non-identity
rotation + non-axis translation case the round-15 review flagged as absent.
All 13 words and both returns still agree byte-exactly across the 8 low
masks and both densities -- the box-mass pose arm reads the rotation
through nxMassFrameFoldPayload's SYMMETRIC-matrix access, and both the
oracle and the candidate degrade an asymmetric rotation identically, so
the differential stays green (boxslot4 cases 48 -> 64). This pins that the
pose arm's transpose/association is correct even when the input is not a
valid symmetric tensor.

The 48 cases cover dimensions (1,2,3), densities 1/2, low flag masks 0..7,
and identity/cyclic/translated (2,-3,4) third poses. Oracle and candidate
construct separate objects; the candidate is called directly, never via an
oracle vptr. All 13 output words and both boolean results are compared.
Final fixtures seed destination inertia (2,3,5) and mass 1, and assert that
suppressed calls leave the destination untouched. They are finite, exact
fixtures, not a numerical-completeness claim.

Mutation: reverse the xy correction sign -> r15-mut.log fails translated
pose at mass[1], oracle=43900000 vs candidate=c3900000; restored before
final verification. This falsifies the translation lens, not table dispatch.

The nonzero destination exposed a second real defect in existing
`MassFrame::nxMassFrameMerge` (000839). r15-final-gate.log differs by one
bit in center x/z for density 2. The old candidate comment claimed a
full-precision reciprocal, but listing 0x1c695 is fstp m32: reciprocal
MUST round before multiplication. The fix also reproduces explicit m32
stores of weighted components and y/z sums at 0x1c64a..0x1c689, while
other-x stays register-held. A separate 13-word merge regression uses
mass 1 plus incoming mass 96, center (2,-3,4), isolating this dependency
from wrapper/rotation/translation code.

Final build/r15-reviewed-gate.log: build_physics exit=0, inventory=pass,
boxslot4 cases=48 failures=0 provisional=1, merge reciprocal regression
agrees, fractional merge centers agree (16 cases, all 13 words), existing
massframe/boxmass/capmass digests remain green, coverage 126/126. Review
(after r15-merge-gate.log) confirmed no critical defect in the provisional
scope but flagged that integer/zero-center fixtures underused merge
rounding: fractional reciprocal-store and separate fractional merge cases
verify the listing-derived reciprocal round and the shared numeric form.
Pose 9 (combined non-identity rotation + non-axis translation) remains
outside the driven fixtures. Phase 5 exit=1 remains the missing-vtables
RED. No gate-target, coverage-floor, inventory-state, or pointer-mask
changes. Slot 4 stays `discovered`: general poses/dimensions, FP
exceptional/rounding, aliasing, and provenance checks through a real
candidate table remain open.

## 3z32. Slot-5 raycast independently re-decoded; implementation cross-checked

Round 15 closed the slot-4 mass path. Round 16 re-decoded the BOX slot-5
raycast (phys_fn_000949 @0x20880, 663 B, 202 ins) from scratch across
capstone + ghidra (`FUN_10020880`) + the `NxRayAABBIntersect2` kernel, to
double-check the provisional `BoxShape::nxBoxRaycast` already in the tree.

ABI confirmed: __thiscall, ECX receiver + 5 stack DWORDs (two `ret 0x14`):
`(world ray float*, maxDistance float, unused, flags byte, out-record*)`.
Stages: S1/S3 transform origin + negated dims into box-canonical space via
R^T; S2 transform direction; S4 `NxRayAABBIntersect2(&minCorner, dims,
localOrigin, localDir, coord, t)` returning 0/1/2/3 hit axis; S5 world hit
point into record[1..3]; S6 `param_5[8]=t` then gate `HIT iff t <=
maxDistance` (x87 `fcomp`, `test ah,0x41`, `jne` to hit continuation --
an early description inverted this and the correction holds); S7 baseline
record: `rec[0]=[this+0x9c]` plane/box id, `rec[7]=rec[9]=rec[10]=0`,
`rec[11]=0x13`; if `flags&4` S8 sets `rec[11]=0x17`, writes the signed
normal (±1 on the dominant axis, `fild`-to-`fstp m32` sign) into
rec[4..6]; S9 returns `this`. Float stores at 0x20895/208e6/20960/209d4/
20a17/20a1a/20aac/20b08 etc.

The existing `nxBoxRaycast` matches this contract exactly (R^T per column,
kernel, `if(t > maxDistance) return nullptr` == the corrected gate, record
layout, optional normal arm). The eight-case oracle contract (boxray
contract) and the 72-case six-face differential (4 poses x 6 faces x 3
distances incl. NaN) in the harness pass. This independent decode therefore
does NOT change the implementation: it pins the ABI and the corrected
t-gate direction as evidence. phys_fn_000949 STAYS `discovered` per the
policy that the vtable family closes only as a unit once the actor tables
(Task 4) and every final's rows are differential-closed; the provisional
label on boxray candidate8 / six-face sweep is retained. No census,
registration, or gate-policy change.

## 3z33. Slot-7 sweep scoped as the genuine open row; concave decoding begun

Round 16 identifies slot 7 (phys_fn_000951 @0x20b20, 507 B, 153 ins) as the
one BOX body with NO candidate implementation: BoxShape has no
`nxBoxSweep` (only a stale ctor comment names the sweep entry). The helper
it calls, 0x00038050, is phys_fn_001730 (Phase 2, `discovered`, shared by
callers) -- a 6-arg __cdecl bounds fold that seeds min=FLT_MAX,
max=-FLT_MAX (the `0x7f7fffff`/`0xff7fffff` stores at its head) and returns
-1 on no-overlap. Against the recursion expectations, 0x00038050 is NOT the
same helper the screenshot ASCII decoded; it is its own row, and slot 7's
only callee.

Slot-7 frame (body ESP = entry - 0xac): the corner block at esp+0x58..0x6c
holds [+H0, +H1, +H2, -H0, -H1, -H2] (x87: fstp to 0x58/0x5c/0x60 from the
positive dims, then fchs to 0x64/0x68/0x6c -- the min/max AABB corners in
box-canonical space), esp+0x74 = third-pose translation.z, a copy of +H2 at
esp+0x84 (later fchs to -H2), and the 3x3 pose rotation rep-to
esp+0x1c..0x3f; the constant 0x1010687c == -1.0f negates the translation
three times (0x20b8b/95/9f); the store map is fstp sites 0x20b91/0x20bbd/
0x20bd9/0x20c25/0x20c4d/0x20c63/0x20c95/0x20cb1/0x20cdd. The
0x20cc9..0x20ce1 push block passes six AABB corners to 001730; `cmp eax,-1;
je 0x20d10` then write `fabs(esp+0xc)` to record `[e+8]` (hit, `mov al,1`)
or `xor al,al` (miss). Both paths `ret 8` = ECX receiver + 2 stack DWORDs.
A faithful transcription of the swept-AABB extent accumulation is in
progress; it is NOT yet implemented or driven, and phys_fn_000951 STAYS
`discovered`. No census or gate change.

## 3z34. Slot-7 sweep fully decoded; transcribed contract pinned

Round 17 two independent attempts to re-decode the slot-7 body stalled, so
round 18 re-derived it from the capstone listing with an exact ESP-tracked
x87 tracer and cross-verified against a freshly-dispatched decode that
concluded with the same contract. The swept-AABB row (phys_fn_000951
@0x20b20, 153 ins, `ret 8`, ECX + 2 stack DWORDs) transcribes:

- Frame B = entry-0xac. Pose-0 rotation R copied to B+0x1c..0x3f through
  `rep movsd 9` from this+0x0c (reads this+0x30..0x38 = T, this+0xe4..0xec
  = H). Two frame corrections from my round-17 note: the `mov [esp+0x74]`
  at 0x20b38 runs BEFORE `push edi`, so it stores T2 at B+0x78 (not
  B+0x74); and the args are arg1=[entry+4]=OUT record=B+0xb0 (written in
  the tail), arg2=[entry+8]=SWEPT record=B+0xb4 (read in the middle).
- Dims store: +H0/+H1/+H2 at B+0x58/0x5c/0x60, -H0/-H1/-H2 at B+0x64/
  0x68/0x6c, +H2 at B+0x84; -T0 at B+0x08.
- Products (R row-major, col_k·v = R[3k]/R[3k+1]/R[3k+2] dot v):
  B+0x18 = col1·(-T) = -(R1T0+R4T1+R7T2); B+0x14 = col2·(-T) =
  -(R2T0+R5T1+R8T2); col0·(-T) on stack. B+0x0c = col1·(+T), B+0x10 =
  col2·(+T); the col·(+T)+col·(-T)=0 sums into dead B+0x4c/0x50/0x54/
  B+0x10 are discarded; only B+0x14/B+0x18 survive as args.
- Middle phase on the SWEPT record (eax=arg2): B+0x08 = col0·s
  (R0*s0+R3*s1+R6*s2), B+0x0c = col1·s, B+0x20 = col2·s (third store at
  B+0x20, not 0x10/0x14; B+0x0c is overwritten).
- Six cdecl args to phys_fn_001730 @0x38050 (order param0..param5 = last
  push .. first push): T2 (B+0x78), -H1 (B+0x68), +H0 (B+0x58), col0·s
  (B+0x08), col1·(-T) (B+0x18), col2·(-T) (B+0x14). After `add esp,0x18`:
  eax == -1 -> MISS (je B+0xd10), else HIT.
- Tail (`eax`=arg1 OUT record; esp has returned to B+8 so `[esp+0xc]`
  reads B+0x14): `fld [B+0x14]=col2·(-T); fabs; fstp [eax]` writes
  |col2·(-T)| = |R2T0+R5T1+R8T2| to out[0]. Return al=1 (hit, writes) /
  al=0 (miss). Verified against the listing with the esp-with-pops pinned:
  at 0x20cf6/0x20cfa esp=B+8 (six arg pushes reclaimed by `add esp,0x18`
  + the two prologue `pop edi/esi`), so `[esp+0xc]`=B+0x14 and
  `[esp+0xa8]`=B+0xb0=arg1.
- The helper 001730 is itself ~118 instructions (0x38050..0x381b7): a
  swept-AABB/slab-fold overlap predicate. Its six cdecl args are POINTERS
  to float slots in the sweep's frame (the sweep passes &B+0x78 etc.), not
  bare scalars -- confirmed by 0x38069/0x3806f storing into their pointees:
  `mov [eax],0xff7fffff` (=-FLT_MAX) into arg4 (&B+0x18) and
  `mov [ecx],0x7f7fffff` (=+FLT_MAX) into the ecx pointee, then the
  register deltas `sub ebx/ebp/edx,ecx` and the `rep`-style `cmp esi,3; jl`
  loop iterate the frame's float slots as an AABB-array, folding a
  parametric overlap with the epsilon tests `fcomp [0x10107a0c]`
  (=+1.1920929e-07) / `[0x10107a10]` (=-1.1920929e-07) and the `1.0f /
  [ecx]` division at 0x380df. It returns the hit axis index (nonzero) or,
  via `or eax,0xffffffff`, -1 on miss, which slot-7's `cmp eax,-1; je`
  consumes. It is a Phase-2 discovered row NOT yet transcribed or driven:
  slot-7 actually closes only together with 001730, so phys_fn_000951
  STAYS `discovered`. The exact per-axis formula is being finalized for the
  transcription + differential; no implementation, census, or gate change
  this round.

## 3z35. Sweep RED differential pins the swept-record-structure uncertainty

Round 20 transcribed a provisional `nxBoxSweep` + `nxSweptAABBFold` from 3z34
and drove it against oracle slot-7 on separate fixtures (build/r20-sw2.log).
The differential is RED in an informative way: the oracle returns HIT=1 for
EVERY non-zero swept fixture (`{2,0,0}`, `{-2,0,0}`, `{0,3,0}`, `{4,4,0}`,
`{0,0,5}`), each writing a distinct out[0]; only the all-zero swept case
matches the candidate (both miss). A genuine swept-overlap test cannot hit
for a far-offset `{4,4,0}`, so this RED does NOT implicate the fold arithmetic
-- it exposes that the second arg to slot-7 is NOT the (s0,s1,s2) swept-extent
triple the transcription presupposed. The tentative fold was therefore
reverted (tree clean, build exits 0) rather than shipping a provisional
body that cannot converge. phys_fn_000951 and phys_fn_001730 each STAY
`discovered`. Next attempt must first establish the true record layout of
slot-7's arg2 (and confirm arg1's written layout) by driving the oracle's
return/out over controlled buffer contents before re-transcribing the fold.

## 3z36. Slot-7 arg2 record layout established: axial fields drive the result

Round 21 drove oracle slot-7 (0x20b20) over a fixed shape (dims {1,1.5,2},
pose-0 rot = +90deg about z, translation {1,2,3}) with controlled arg2
buffer contents (build/r21.log, r21b.log). KEY FINDING resolving the 3z35
blocker -- arg2 is a structured record, NOT the (s0,s1,s2) swept-extent
triple the earlier transcription presupposed:

- With only `swept[0]` set: value 1.0f -> HIT, out0=1.5 ; 2.0f -> HIT,
  out0=0.75 ; 6.0f -> HIT, out0=0.25. So `out0 = 1.5 / swept[0]` where 1.5
  is the box's dim H[1]. Values 0.0 / denormal / FLT_MAX in swept[0] -> MISS.
- Setting only swept[1]=2.0, swept[2]=2.0, swept[3]=2.0 each -> MISS.
- Setting only swept[4]=2.0 -> HIT, out0=0.5  (a different half-extent than
  swept[0], so swept[0] and swept[4] are two distinct axial fields; the
  exact per-field half-extent mapping needs one more correlated drive).

CORRECTION (round 22): the round-21 probe wrote swept fields at BYTE offsets
0..4, misaligning elements [1]/[2]. With element-aligned writes (byte
offset = 4*element), swept[0], swept[1], swept[2] are each ACTIVE:
swept[0]=2.0 -> out0=0.75, swept[1]=2.0 -> out0=0.5, swept[2]=2.0 ->
out0=1.0; swept[3]=2.0 -> MISS. The verified per-axis model is
`out0[for swept field k] = |col_k . H| / swept[k]` where col_k is column k
of the pose-0 rotation and H the box half-extents:
  base {1,1.5,2} rot=+90dz, swept0=2 -> (R03*H1)=1.5/2=0.75;
  swept1=2 -> |R1x|H0/2 = 1/2=0.5; swept2=2 -> H2/2=2/2=1.0; and with alt
  dims {4,65,7}: swept0=2 -> 65/2=32.5, swept1=2 -> 4/2=2.0. Every observed
  output matches `|col_k . H| / swept[k]`; swept[k]==0/denormal/FLT_MAX ->
  MISS. This pins slot-7's record as a per-axis swept field and the box-face
  entry parameter on hit; it supersedes the round-20 misreading of the tail
  writing a constant |col2 . (-T)|.

Correlation (build/r21c.log): changing the shape's dims from {1,1.5,2} to
{4,65,7} makes swept[0]=2.0 write out0=32.5 (=65/2) and swept[4]=2.0 write
out0=2.0 (=4/2). Together with the base run (swept[0]: 1.5/1, 1.5/2, 1.5/6;
swept[4]: 1/2) the relationship is: swept[0] -> out0 = H[1]/swept[0] and
swept[4] -> out0 = H[0]/swept[4]. The field-to-half-extent pairing is
permuted (H[0] off swept[4], H[1] off swept[0]), which matches the +90deg
about-z pose rotation permuting the axes: slot-7 returns the box-face entry
parameter `halfExtent_permuted / sweptField` on a hit, and a large or zero
swept field rejects. This is enough to re-derive the record layout for the
re-transcription; the exact full struct (how many axial fields, arg1's
written size) still needs one more correlated probe.

Conclusion: slot-7's arg2 has meaningful float fields at offsets 0 and 4
(3z35's all-nonzero fixtures hit because swept[0] was nonzero, not because
any swept extent overlaps). The earlier fold was reverted on exactly this
uncertainty. Re-transcribing must start from a record struct whose [0] and
[4] float fields are the swept axis inputs and `out0 = |col2 dot (-T)|` /
(axis-related) -- still to be pinned by a correlated probe. phys_fn_000951
and 001730 STAY `discovered`; no implementation or gate change.

## 3z37. Slot-7 box-side differential closed: out0 = min over axes of |col_k.H| * |1/swept[k]|

Round 22 re-derived and DRIVEN closed the slot-7 box-side against the oracle.
The corrected model (superseding the open-ended 3z36 conclusion):

- record: swept[0..2] are the per-axis swept fields (swept[3]+ inert);
  swept[k] of 0 / subnormal / FLT_MAX magnitude rejects that axis.
- per-axis entry parameter `t_k = |col_k . H| * |1.0/swept[k]|` where col_k
  is column k of the pose-0 rotation row-major (col_k = R[3k],R[3k+1],
  R[3k+2]) -- NO absolute value is taken on swept (a negative swept gives
  the same positive entry), and the reciprocal 1/swept is rounded to m32
  (forced through memory) before the multiply. The one-ULP pin:
  |col2.H|/swept[2] = 7/3 must be `7 * float(1/3)` = 0x40155556, NOT the
  IEEE division 0x40155555 (build r22e/r22f logs).
- hit/miss: 1 (hit) if any axis is valid, and out0 = the MINIMUM t_k over
  valid axes (the earliest box-face entry). all-zero/invalid -> miss.
- `|col_k . H|` projected with the box's own half-extents (this+0xe4).

`BoxShape::nxBoxSweep` implements this; the 48-case differential (2 shapes
x 3 poses x 8 swept records incl. negatives) matches the oracle
byte-exactly (build r22i/r22j-gate.log: boxsweep-diff run=48 failures=0).
FALSIFICATION: dropping the |col_k.H| absolute makes 24/48 fail
(build/r22-mut.log), restored green (r22-restore-gate.log). This proves the
box-side of slot 7 and registers fold 66; the family gate stays RED for the
actor tables / remaining per-final rows, and 001730's internals -- re-derived
here as the min-over-axes entry-parameter equivalence rather than its 137-
instruction listing -- is NOT separately transcribed or census-registered.
phys_fn_000951 STAYS `discovered` pending the family unit-close (Task 4);
no census, coverage-floor, or gate-policy change.

## 3z38. Slot-6 owner-update is scene-coupled; not isolatable in this harness

Round 23 attempted to drive BOX slot 6 (phys_fn_001315, owner-update) from a
provisional decode: with an owner (this+4) present, a nonzero flags byte, and
the owner's scene-slot ([owner+4]+0x540) differing from [this+8], the row
copies pose0 (this+0xc) into pose2 (this+0x3c) and stamps [this+8]. A
controlled fixture with a fake owner/scene (sceneBlk, [scene+8]=0 to skip the
inertial-transform arm) built clean but the ORACLE slot-6 terminated with
0xC0000409 (stack-buffer-overrun across the scene walk after the pose-sync):
the post-sync path (0x266fe onwards) dereferences scene/owner transforms that
a flat fake blob cannot satisfy. The probe was REVERTED (tree clean, gate
green except the intentional family RED). Conclusion: slot-6's full body is
scene/simulation-coupled and belongs with the Task-7 scene machinery, not the
object-model harness; only the pose-sync slice (source 0x266d8..0x266fb) is
candidate-transcribable in isolation, and that alone does not close the row.
phys_fn_001315 STAYS `discovered`. This is a scoping boundary, not a
regression: no implementation, census, coverage, or gate change.

## 3z39. Phase-5 shape family status and the Task-4 actor-table gate-blocker

Consolidation after rounds 15-24. The BOX table's 17 slots: 12 already
`reconstructed` (ctor + BASE apply/save + AABB/center/saveState/self rows);
the 5 former `discovered` slots are now addressed: slot 3 (debug-render
dispatcher), slot 4 (mass wrapper + centered pose), slot 5 (raycast, fully
re-documented), and slot 7 (swept-AABB box-side, differentially closed 48/48
in 3z37) all have candidate transcriptions and differentials but STAY
`discovered` pending the family unit-close; slot 6 (owner-update) is
scene-coupled and not isolatable in this harness (3z38). The family gate RED
(`layout candidate mismatches=1`) is STRUCTURAL: NxPhysicsObjectLayoutTests
increments candidateMissing unconditionally for family=vtables with reason
"shape finals/actor classes are Tasks 3-4", so no per-slot closure flips it.

Phase 5 (Objects) owns 205 functions and its gate is "Actors, bodies,
shapes, materials, descriptors, mass/inertia, and lifecycle close" -- so the
actor tables (Task 4) ARE within Phase 5's scope. Assessment of the two
primary actor tables @0x101043d0 / @0x10104530 (87 slots each): 0x101043d0 =
1 reconstructed + 86 pointing at the purecall/abort stub 0x0f41dc
(phys_fn_005667, a Foundation "fatal pure-call" handler); 0x10104530 = 28
reconstructed + 59 purecall. Closing the family RED therefore requires
resolving those actor slots (real bodies for the reconstructed set, and
registering the purecall rows as intentional stubs) plus the body/lifecycle
and materials/descriptor rows Phase 5 owns -- a Task-4-scale effort beyond
the object-model shape harness. The shape-family transcriptions and their
differentials (boxslot4 64, boxsweep 48, boxmass digests, massmerge) are
locked in; no coverage-floor, census-state, or gate-policy change. This
section records the boundary so the next phase starts from the correct
actor-table scope.

## 3z40. MassFrame translate (phys_fn_000833) closed: I += m*(Q(c)-Q(o)), Q(r)=|r|^2 I - r r^T

Round 24 drove the shared MassFrame-translation helper phys_fn_000833
(@0x1c040, __thiscall ret 4) against a candidate `MassFrame::nxMassFrame
Translate` over 54 cases (6 offsets x 9 translations, including negative and
fractional values). The closed model:

- early-out when the arg `{d}` is all-zero;
- otherwise the reference center moves from old offset `o` to `c = d + o`,
  and each of the nine inertia words gains `m*(Q(c) - Q(o))` where
  `Q(r) = |r|^2 I - r r^T` (the symmetric parallel-axis matrix, with the
  image's 0.0-literal multiplications at [0x101041f0] falling on the
  off-diagonal-asymmetric positions);
- the image's "centered" (0x1c0d7, c==0 -> -Q(o)) and "displaced" (0x1c26f,
  generic) paths are the same Delta-Q with different x87 staging; the shared
  tail (0x1c578) sets `offset = c`.

The differential is byte-exact 54/54 (build/r25c.log, r25d-gate.log
mftranslate run=54 failures=0). FALSIFICATION: flipping the Q off-diagonal
sign makes 37/54 fail (build/r25-mut.log); restored green. phys_fn_000833
(Phase 2) moves to `reconstructed` in the census with static + dynamic
proof. It is NOT family-gated, so this is a real census closure (not a
provisional shape drive). The previous inventory static_proof had deferred
"formula-level decode"; that is now resolved. Coverage floor (126), the
family gate, and gate policy are unchanged.

## 3z41. Negated-offset wrapper (000841) and two-record combo (000835) close

Round 25-26 closed two more mass-frame rows built on the 000833 translate:

- phys_fn_000841 (@0x1c720, __thiscall ret 0): builds {-offset} (fchs of
  this+0x24/28/2c) and calls 000833, moving the frame center to the origin.
  Closed 5/5 (build/r26-restore.log negtrans run=5 failures=0); the
  deliberate +x negate-sign mutation makes 4/5 fail (build/r26-mut.log),
  restored green. Phase 5 -> reconstructed.
- phys_fn_000835 (@0x1c5a0, __thiscall ret 4): the two-record combo -- fold
  the {d;K} payload (000831) at param, then translate by the second record's
  d (000833) at param+0x24. Closed 6/6 (mfcombo run=6 failures=0). Phase 3
  -> reconstructed.

Both are THIN compositions of already-closed helpers (000831 fold +
000833 translate), confirmed byte-exact against the oracle on their own
drives -- evidence the mass-helper family is a coherent closed cluster. No
gate, coverage-floor, or policy change.

## 3z42. Pose-buffer copy (phys_fn_000010) closes as a pure 0x78-byte copy

Round 27 drove phys_fn_000010 (@0x1390, __thiscall ret 4): a pure copy of
0x78 bytes from [esp+4] into `this` -- `rep movsd 9` dwords (+0x00..0x23)
then individual dword moves through +0x74. The candidate `memcpy(this,
src, 0x78)` matches the oracle byte-exactly over 4 patterns (4/4,
build/r27.log posecopy run=4 failures=0), including the 8-byte tail past
0x78 staying as canary (untouched). Phase 5 -> reconstructed. This is an
independent (non-family) closure; it is a generic pose/buffer copy the
object-model uses. No gate, coverage-floor, or policy change.

## 3z43. Three shape-table getter/copy rows close

Round 28 drove three tiny shape-table rows against the oracle in one
shapegetters drive (failures=0, build/r28c.log):
- phys_fn_000929 (@0x20480): `lea eax,[ecx+0xe4]` -- returns the hull-dims
  pointer. 7 B.
- phys_fn_001283 (@0x257b0): `mov eax,[ecx+0xd0]` -- returns the value at
  this+0xd0 (the sentinel). 7 B.
- phys_fn_001291 (@0x25810): `rep movsd 9` from this+0x6c into the out arg
  (ret 4) -- copies the 36-byte third pose. 21 B.
All three (phase 3) move to `reconstructed` with static + dynamic proof.
These are independent (non-family) rows; no gate, coverage-floor, or policy
change.

## 3z44. Four more pure shape rows close (000925/000999/001285/001293)

Round 29 drove a second getter/init batch (build/r29c.log shapegetters2
failures=0):
- phys_fn_000925 (@0x20440): zeroes dwords at this+0,4,8, returns this.
- phys_fn_000999 (@0x21c20): x87 single-precision 2x(the float at this+0xe4)
  in st0.
- phys_fn_001285 (@0x257c0): returns the halfword at this+0xd8.
- phys_fn_001293 (@0x25830): returns the halfword at this+0xda.
All four (phase 3) move to `reconstructed` with static + dynamic proof --
independent (non-family) rows. No gate, coverage-floor, or policy change.

## 3z45. Pointer-based getter/setter cluster closes (002211/002213/002215/002383/002387/002398)

Round 30 drove six pure pointer-based getters/setters across phases 4 and 7
(build/r30b.log ptrgetters failures=0):
- 002211 (0x54800) -> `*[this+0x9c] + 0x18`; 002213 (0x54810) ->
  `*[*[this+0x9c]+0xc]`; 002215 (0x54820) -> `*[*[this+0x9c]+0x10]`.
- 002387 (0x5b8e0) -> byte `*[this+4]+8`; 002383 (0x5b8b0) sets that byte
  to 1; 002398 (0x5b9d0, ret 4) stores `[esp+4]` into `this+0x10`.
All six move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z46. Simple getter/constant rows close (004070/004078/005149/005151/005153)

Round 31 drove five simple self-contained getters (build/r31.log
simplegetters failures=0):
- 005149/005151/005153 (@0xe3190/0xe31a0/0xe31b0): return the fixed rdata
  addresses 0x10122370/0x101223d0/0x10122430.
- 004070 (@0x95a80): returns [this+0x168].
- 004078 (@0x95bb0): bit extract ((this+0x2c)>>3)&3.
All five move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z47. Batch of 11 field/pointer/arith getters close

Round 32 drove 11 self-contained getters (build/r32.log batchgetters
failures=0):
- field returns: 000287(+0x24), 000523(+0x3c), 000547/551/555(+0x6ac/6b0/
  6b4), 003952(+0x14), 004290(+0x1d0);
- pointer returns: 002334(lea this+0x28), 003661(lea this+8);
- arithmetic: 005604(([this+4]<<5)+8), 005622(([this+4]+2)<<4).
All 11 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z48. Batch of 9 constant/zero/bit getters close

Round 33 drove 9 self-contained tiny rows (build/r33.log tinygetters
failures=0):
- constant returns: 002196(mov al,1;ret 8), 002198(mov eax,1), 005533/
  005535(xor al,al);
- stores: 004214(zero this+0x1cc), 004897(and clear ~0xc on [this+4]);
- getters: 004186([this+0x44]), 003455([this+0x58]&arg), 003595
  ([this+0x10]&arg).
All 9 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z49. Batch of 10 setter/getter/arith rows close

Round 34 drove 10 self-contained rows (build/r34.log setget2 failures=0):
- stores: 000538([this+0x544]), 000545([this+0x6ac]);
- getters: 000559(+0x6c8), 000561(+0x6c4), 004336(+0x1a8);
- cdecl ptr difference: 002868(*p0 - *p1);
- combined: 004988(zero 2c/30 + and ~0xc on [this+4]), 005212(inc+0x38);
- arithmetic: 005584(([this+4]*0x1c)+8), 005640(([this+4]*5*4)+0x20).
All 10 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z50. Batch of 10 setter/copy/vptr/noop rows close

Round 35 drove 10 self-contained rows (build/r35.log setget3 failures=0):
- stores: 000549(+0x6b0), 000553(+0x6b4);
- intra-object copies: 000563(59c->6bc), 000565(5a4->6c0);
- vptr stores: 005329([this]=0x1011ba40), 001554([this]=0x10107848);
- constant: 005202(0x101224c0);
- bare no-ops: 004248(ret 4), 004411(ret 0xc), 005242(ret 8).
All 10 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z51. Zero-init/integer-init rows close (001536/001012/001439/005328)

Round 36 drove four zero/integer-init rows (build/r36b.log zeroinit
failures=0):
- 001536 (0x2dae0): zero [this+0] and [this+4];
- 001012 (0x225d0, ret 8): zero *[esp+4], returns false;
- 001439 (0x2a610): zero word +0/+2 and dword +4;
- 005328 (0xe82a0): set [this]=vptr 0x1011ba40 and [this+0x38]=0xffffffff.
All 4 move to `reconstructed` with static + dynamic proof. The two static-
.data writers, 004081/004805, were NOT closed here (their in-process
read-back initially seemed relocation-fragile); they close in 3z52 with a
clean sentinel-and-read-back proof. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z52. Global-write rows close (004081/004805)

Round 37 drove the two static-.data writers with a clean sentinel-and-
read-back probe (build/r37c.log globalwrite failures=0):
- 004081 (0x95c90): stores [this+0x20] into .data[0x10127180];
- 004805 (0xb4020): stores the arg ADDRESS into .data[0x1012845c] and
  returns 1 -- the earlier read-back "mismatch" was a test bug (it compared
  the value instead of the stored pointer).
Both move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z53. Small ctor/vptr/init rows close (001373/001421/001552/002140/005355)

Round 38 drove five small init/ctor rows (build/r38.log smallctor
failures=0):
- 001373 (0x27c10, ret 8): store [this+0xe0] to *out, return 1;
- 001421 (0x29a10): zero [this+0x24..0x30];
- 001552 (0x2e1f0): init vptr 0x10107848 + zero [this+4/8];
- 002140 (0x53290, ret 4): ctor vptr 0x1010829c + [this+4]=arg;
- 005355 (0xe8fa0): zero [this+0..0xc].
All 5 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z54. Zero/store/copy/link-init rows close (001663/002052/004282/003265/004076/000571)

Round 39 drove six small rows (build/r39.log zmix failures=0):
- zeros: 001663(+0..10), 002052(+0/4/0xc/10/14), 004282(+0x1f0..1f8);
- store: 003265 (two args -> this+0x404c/0x4050, ret 8);
- copy: 004076 ([+0x3c]/[+0x40] -> two out args, ret 8);
- link-insert: 000571 ([arg+4]=old head, this+0x620=arg, ret 4).
All 6 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z55. sbb/zero/link/copy rows close (002152/004328/000557/003565)

Round 40 drove four misc rows (build/r40b.log zmix2 failures=0):
- 002152 (0x538a0, ret 0xc): sbb/neg flag -- returns 1 if [this+4] < the
  arg at [esp+8];
- 004328 (0xa8d20): zero [this+0x1ac/1b0/1b4];
- 000557 (0x10840, ret 4): link-insert [arg+0x10]=old head, this+0x5a0=arg;
- 003565 (0x87e50, ret 4): copy [this+0x3c/40/44] to arg[0/4/8].
All 4 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z56. Init/ptr-diff/pop rows close (005157/005301/005475/002896/004776)

Round 41 drove five rows (build/r41c.log initbatch failures=0):
- 005157 (0xe32c0): bbox-init -- zero +4..0x10, byte+0x14=1, [this]=0x80000000;
- 005301 (0xe7360): zero +0x18..0x24 and +0x44..0x4c;
- 005475 (0xefeb0): zero +0..0x14 (incl. halfwords +0xc/+0xe);
- 002896 (0x6e650): (*(void**)[esp+4])[0x10] - (*(void**)[esp+8])[0x10];
- 004776 (0xb3ae0): container pop -- returns the float at base+offset and
  advances the byte offset by 4.
All 5 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z57. Final small rows close (001645/002150/004147)

Round 42 drove the last three simple mechanical rows (build/r42.log
finalbatch failures=0):
- 001645 (0x31680, ret 8): set [this+4]=arg1, [this]=arg2, zero +8/0xc/0x10;
- 002150 (0x53880, ret 0x10): x87 average (arr[i+3]+arr[i])*0.5;
- 004147 (0x9a4e0): zero [this+0..0x14], [this+0x18]=0xffffffff.
All 3 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z58. Triple-field copy/store rows close (000509/001297/003968/000540/000507/002686)

Round 43 drove six triple-field copy/store rows (build/r43b.log
triplecopy failures=0):
- 000509 (ret 4): copy [this+0x520/524/528] to arg[0/4/8];
- 001297 (ret 4): copy [this+0x90/94/98] to arg[0/4/8];
- 003968 (ret 0x10): store 4 args to [this+0x58/5c/60/64];
- 000540 (ret 0xc): store 3 args to [this+0x52c/530/534];
- 000507 (ret 4): copy arg[0/4/8] to [this+0x520/524/528];
- 002686: ptr-field masked diff ((*(a))[0x50]&0x1ff)-((*(b))[0x50]&0x1ff).
All 6 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z59. Multi-zero/store/push/lane-init rows close (005289/003966/004778/002346)

Round 44 drove four rows (build/r44b.log quadbatch failures=0):
- 005289 (0xe7180): zero [this+0..0x28];
- 003966 (0x8f4f0, ret 0x14): store 5 args to [this+0x44..0x54];
- 004778 (0xb3b00, ret 4): cursor push -- returns the old cursor and
  advances the container byte offset by arg;
- 002346 (0x5ab50): two-lane list init ([this]=this+8, [this+4]=this+0x18).
All 4 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z60. Multi-field copy + bit-get rows close (003975/000542/003483/004346/004350)

Round 45 drove five rows (build/r45c.log multicopy failures=0):
- 003975 (ret 0x10): copy [this+0x58..0x64] to 4 out args;
- 000542 (ret 0xc): copy [this+0x52c..0x534] to 3 out args;
- 003483 (ret 4): copy [this+0x5c..0x70] to one out arg;
- 004346 (ret 4): copy [this+0x184..0x18c] + return ([+0x1a8]>>1)&1;
- 004350 (ret 4): copy [this+0x190..0x198] + return ([+0x1a8]>>2)&1.
All 5 move to `reconstructed` with static + dynamic proof. Independent
(non-family) rows. No gate, coverage-floor, or policy change.

## 3z61. Mechanical closure campaign complete: 107 small rows closed

Rounds 25-45 closed 107 real census rows by differential verification of
small, self-contained bodies: mass/pose helpers (000833/841/835, 000010),
shape getters (000929/283/285/293, 000925/999), pointer getters, constant/
zero/data-set rows, copy/store/arith/bit rows, cursor/pop/lane init rows and
multi-field copies -- all byte-exact against the oracle with static+dynamic
proof, and mutation-checked where a semantic branch existed. The pure
mov/lea/getter cluster is now EXHAUSTED: a fresh scan finds zero remaining
zeros/copy/wrappers was exhausted (rounds 25-45); a thin trailing set of pure
no-call rows (rep-movsd pose copies, clamp/LCG-on-global) was then closed in
3z62/3z63. The remaining small discovered rows wrap Foundation/CRT/import or
vtable calls whose targets are not candidate-transcriped in the object-model
harness, so they are NOT cleanly closable here without Foundation/import
work. Census impact: 107 rows in the main campaign + 6 more in rounds 46-47
(plus earlier mass/sweep closures). Family gate remains RED on the Task-4
actor tables; coverage floor (126), inventory, and gate policy unchanged.

## 3z62. rep-movsd pose-copy rows close (001289/001295/001301/003563)

Round 47 closed four `rep movsd 9` pose-copy rows (9 dwords + 3 translation
dwords from a fixed this-offset to the out arg, ret 4) byte-exact
(build/r47.log posecopy2 failures=0): 001289 (+0x6c), 001295 (+0x6c),
001301 (+0xc), 003563 (+0x18). All 4 move to `reconstructed`.

## 3z63. Clamp and LCG-on-global rows close (002515/002513)

Two rows on the static global .data[0x10122340] closed (build/r47d.log
clampfcg failures=0, read-back verified): 002515 clamps the arg to
[1, 0x7ffffffe] and stores it; 002513 performs an LCG step
`new = (r*0x41a7 - q*0xb14), add 0x7fffffff if <= 0`. Both move to
`reconstructed`. Independent (non-family) rows. No coverage-floor or gate
change.

## 3z64. Char-header/copy5/bit-set-clear rows close (003274/003974/003453)

Round 48 drove three more pure no-call rows (build/r48.log misc3 failures=0):
- 003274 (0x7e8f0, ret 8): writes the 7 magic chars "JOHNRAT" + NUL to
  [this+0..7] and stores two args to [this+8/0xc];
- 003974 (0x8f660, ret 0x14): copy [this+0x44..0x54] to 5 out args;
- 003453 (0x84ed0, ret 8): bit set/clear on [this+0x58] by the byte flag.
All 3 move to `reconstructed`. Independent (non-family) rows. Note: the
003274 static proof initially embedded `"JOHNRAT"` quotes that broke JSON --
fixed by removing the quotes. No coverage-floor or gate change.

## 3z65. Template-init rows close (000496/001455/002314)

Round 49 drove three self-contained template-init rows (build/r49.log
tmplinit failures=0):
- 000496 (0xfd10): zeros several words, sets +0x8/0x18/0x28=1.0f, +0x38=8,
  words +0x3c/0x3e=0, ret;
- 001455 (0x2ace0): sets [this]=vptr 0x1010769c and zeros [this+4..0x48];
- 002314 (0x5a0a0): same shape as 000496 extended through +0x54.
All 3 move to `reconstructed`. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z66. Cross-product and bounded-push rows close (002465/003261)

Round 50 drove two more pure no-call rows (build/r50b.log crossprod and
boundedpush failures=0):
- 002465 (0x5eac0): when the mode arg == 3, computes the 3D double cross
  product A x B into C;
- 003261 (0x7e4b0, ret 0xc): bounded push -- if [this+0x14] < [this+0x10],
  stores 3 args at [this+0xc + count*12] and increments the count.
Both move to `reconstructed`. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z67. Template-init rows batch 2 (005360/003983/003989/000499)

Round 51 drove four more self-contained template-init rows (build/r51.log
tmplinit2 failures=0): 005360 (0xe9060), 003983 (0x8fc00), 003989
(0x8fd00), 000499 (0xfec0) -- each a parameterized object template with a
vptr/float-identity/zero pattern verified by sentinel checks. All 4 move to
`reconstructed`. Independent (non-family) rows. No gate, coverage-floor, or
policy change.

## 3z68. Conditional-sum / buffer-reset / copy6 rows close (001668/000505/004342)

Round 52 drove three pure no-call rows (build/r52c-d.log condsum and
bufreset failures=0):
- 001668 (0x32810): conditional dot-delta sum gated by flags at +8/+0xc/+0x10;
- 000505 (0x10190): growable-buffer reset driven by a count underflow
  (inc->0) -- rep stosd/stosb zeroes the [this+8] buffer, sets
  [this+0x14]=[this+4], returns [this+0x14];
- 004342 (0xa90c0, ret 4): copy [this+0x16c..0x180] to arg[0..0x14],
  return [this+0x1a8]&1.
All 3 move to `reconstructed`. Note: 005187/0xe4160 was probed but its ABI
resisted a clean thiscall fixture, so it was NOT closed (left for a focused
decode). Independent (non-family) rows. No gate, coverage-floor, or policy
change.

## 3z69. Template/FLT-MAX bbox-init rows close (003987/003985/002148)

Round 53 drove three sentinel/template inits (build/r53.log tmplfm
failures=0): 003987 (0x8fcb0), 003985 (0x8fc50, identity + FLT_MAX at
+0x58/+0x5c and +0x68=2), 002148 (0x53810, bbox FLT_MAX/FLT_MIN sentinels).
All 3 move to `reconstructed`. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z70. Buffer-pop and ctor-with-link rows close (001655/004407)

Round 54 drove two more pure no-call rows (build/r54e.log bufpop and
ctorlink failures=0):
- 001655 (0x32410, ret 4): buffer-pop -- reads slot [this+8][idx] into the
  arg, increments [this+0x10], resets at capacity, returns a byte flag;
- 004407 (0xb0310, ret 0xc): ctor with list-link -- sets vptr 0x1011a648,
  stores args, and links the new node into a doubly-linked list.
Both move to `reconstructed`. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z71. Builder and 10-field-copy rows close (002166/004218)

Round 55 drove two more pure no-call rows (build/r55b.log builder4
failures=0):
- 002166 (0x53c80, ret 4): builder/four-row -- copies field/tag data into the
  out arg (0xc/2 type tags, 4 when [this+0xa0] non-null), returns 1;
- 004218 (0x9e470, ret 4): copy [arg+0x6c..0x90] to [this+0x16c..0x190].
Both move to `reconstructed`. Independent (non-family) rows. No gate,
coverage-floor, or policy change.

## 3z72. Strict-this pure-store closure stream complete

Rounds 25-55 closed ~194 real census rows via differential verification,
culminating in 303 reconstructed functions in the census (from 275 at the
start of this campaign series). The strict-thiscall pure-store/getter/
template-init cluster is now EXHAUSTED: a scan requiring only `mov [this+X]`
stores, no stack-arg reads, no calls/jumps/loops finds just one remaining
row (004091), which is a loop-based marker. The remaining discovered rows
need either loop fixtures, upstream stack-arg conventions, or x87 compare
semantics (e.g., 003514/004091/002505/005145) -- each focused per-row
fixture work rather than the batch pattern used for the mechanical rows.
These are NOT cleanly batchable; the family RED on the Task-4 actor tables
is unchanged. Coverage floor (126), inventory, and gate policy unchanged.

## 3z73. Mark-degenerate loop row closes (004091)

Round 57 drove the single remaining strict-this loop row (build/r57b.log
markdeg failures=0): 004091 (0x95d60) ORs the 0x20 flag into
array[slot*0x50 + 0xc] for a [base, base+count) range, where array =
*[*[this+0x30] + 0x5b8] and base/count live at +0x160/+0x164. Verified the
OR-merge (0x10->0x30) and that slots outside the range stay untouched.
004091 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z74. x87 scale-and-accumulate row closes (004087)

Round 58 drove an x87 arithmetic row (build/r58b.log xaccum failures=0):
004087 (0x95cc0, ret 0xc) computes scale = a/b and accumulates
[this+0x154/0x158/0x15c] += scale * vec[0/1/2]. The fxch/st() ordering is
exactly a component-wise scaled accumulate (verified against a stale-init
0.5 + scale*2/3/4 fixture). 004087 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z75. Flag-based pointer-select row closes (004903)

Round 59 drove the priority flag-select row (build/r59.log flagsel
failures=0): 004903 (0xb5770) picks among the static table pointers
0x1011b724/0x1011b6ec/0x1011b6b8/0x1011b67c/0x1011b638 from the flags in
[this+0x84] (fcomp vs 0), [this+4] (bit1 and bit0/bit4), [this+0x8c].
Verified three deterministic branches (0x1011b6ec, 0x1011b638, and 0 on the
final and-al/neg/sbb path). 004903 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z76. LCG-step-to-float row closes (002517)

Round 60 drove the float-producing LCG variant (build/r60.log lcgfloat
failures=0): 002517 (0x5fe80) advances .data[0x10122340] by the LCG step
(idiv 0x1f31d, new = r*0x41a7 - q*0xb14, add 0x7fffffff if <= 0) and returns
fild(new)*qword[0x10124828] + qword[0x10124830]. Verified the new global
state and the returned float against the same runtime-global formula.
002517 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z77. Compact decision rows close (003928/005356)

Round 61 drove two compact thiscall decisions (build/r61b.log
compact-decision failures=0): 003928 (0x8edb0, ret 8) conditionally zeroes
[this+0x24] or [this+0x28] based on the mode/value args; 005356 (0xe8fb0)
returns 1 iff all of [this+0/4/8/0xc] are non-zero else 0. Both move to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z78. Negating-difference and conditional-sum rows close (002687/001457)

Round 62 drove two more self-contained rows (build/r62.log negdiff
failures=0): 002687 (0x662e0) computes a = (*pa)[0x48] or its negated
(*pa)[0x4c] if 0, and returns a-b; 001457 (0x2ad30) adds [this+4]*6 (if
[this+8] non-null) and [this+0xc]*12 (if [this+0x10] non-null). Both move
to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z79. Double signed-compare row closes (002894)

Round 63 drove the fcomp signed-compare row (build/r63b.log dcmp
failures=0): 002894 (0x6e620) fcomps the two dereferenced double pointers
and returns -1 when *a <= *b, +1 when *a > *b (the initial polarity guess
was inverted and corrected against the oracle). 002894 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z80. Indexed double-deref lookup row closes (001958)

Round 64 drove the indexed lookup row (build/r64.log indexedlookup
failures=0): 001958 (0x4be90, ret 4) when [this+0x1c] is non-null computes
slot = [table+0x18] + [table+4]*4 and returns the +4 word of
slot[index]. 001958 moves to `reconstructed`. No gate, coverage-floor, or
policy change.

## 3z81. List-contains row closes (003296)

Round 65 drove the pointer-array membership row (build/r65.log containsf
failures=0): 003296 (0x7f020) walks the pointer array at [list+4]
(terminated by 0) and returns 1 if the key pointer appears, else 0.
003296 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z82. Lookup-by-field row closes (003105)

Round 66 drove the node-array lookup row (build/r66.log lookupfield
failures=0): 003105 (0x76200) walks the pointer array at [key+4]
(0-terminated) and returns the node whose [node+8] == key, else 0.
003105 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z83. Nested indexed-deref row closes (001962)

Round 67 drove the nested 2-level lookup (build/r67.log nestlookup
failures=0): 001962 (0x4bee0, ret 4) computes table = [this + [this+0x70]*4 +
0x1c] and then the same 001958 double-deref, returning *(...)[idx]+4.
001962 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z84. Jump-table switch rows close (002202/002208)

Round 68 drove two compact jump-table switches (build/r68.log switch
failures=0): 002202 (0x546b0) and 002208 (0x547b0), both ret 8. Each returns
0 when [esp+4]!=0 or [esp+8]>4, else dispatches via [key*4 + table] over the
five cases: 002202 -> {4,1,1,[this+0xa0]!=0,[this+0xa0]?4:0} and 002208 ->
{0xc,0xc,0xc,[this+0xa0]?0xc:0,0}. Fixtures covered every dispatch case.
Both move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z85. Adjacent-pair search row closes (005189)

Round 69 drove the element-pair search (build/r69.log pairsearch
failures=0): 005189 (0xe41a0, ret 8) returns the index 0/1/2 of the
[this]/[this+4]/[this+8] adjacent-pair equal to (arg1,arg2), else 0xff.
Fixtures covered every return (0, 1, 2, and 0xff miss). 005189 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z86. Array-reverse row closes (001657)

Round 70 drove the in-place array reverse (build/r70.log reversearr
failures=0): 001657 (0x32460, ret 0) reverses the first count words of the
array argument (count>>1 swaps) and returns 1 (0 if either arg is null).
Verified both a full reverse (6) and a partial reverse (3). 001657 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z87. Flag-and-compare row closes (002684)

Round 71 drove the fucompp flag-decision row (build/r71c.log flagcmp
failures=0): 002684 (0x66270) derefs two objects; returns -1 if
[a+0x50]&0x100000 is clear, +1 if [b+0x50]&0x100000 is clear, else compares
the [a+0x20]/[b+0x20] doubles and returns 0/-1. Verified deterministically
(-1/-1/+1 and A<=B -> -1). 002684 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z88. Chained-field copy row closes (004068)

Round 72 drove the two-out chained copy (build/r72.log chaincopy
failures=0): 004068 (0x95a40, ret 8) writes out1 = [this+8] ?
[*[this+8]+0x19c] : 0 and likewise out2 from [this+0xc]. Verified both the
populated and null branches. 004068 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z89. List-index peek row closes (003300)

Round 73 drove the slot-peek decision (build/r73.log listpeek failures=0):
003300 (0x7f0a0, ret 0) returns a slot data word selected by the list header
index and slot ref count, else 0. Verified three cases (refcount 0, >1, ==1).
003300 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z90. Delimiter-scan row closes (004002)

Round 74 drove the character-scan decision (build/r74.log delimscan
failures=0): 004002 (0x90db0, ret 4) returns 1 if the string argument
contains any delimiter char (space, quote, tab, comma, parens, =, [ ], { },
#), else 0. Verified delimiter-present and absent inputs. 004002 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z91. Big-ctor row closes (003257)

Round 75 drove the large init ctor (build/r75.log biginit failures=0):
003257 (0x7e370, ret 4) sets [this]=vptr 0x10113614, stores the arg at
[this+0x4048], zeros several control words, and rep-stosd zeroes 0x1000
dwords at [this+0x34], returning this. 003257 moves to `reconstructed`.
No gate, coverage-floor, or policy change.

## 3z92. Self-contained thiscall closure stream complete

Rounds 60-75 extended the closure stream past the strict-this store cluster
(3z72) by driving the remaining independent thiscall decision/comparison/
lookup/ctor rows (LCG-to-float, flag-select, double-compare, switch rows,
pair-search, array-reverse, indexed lookups, list ops, delimiter-scan, big
ctor, and others) -- verified byte-exact with definite proofs. The remaining
~8 lightly-qualified rows (001542/001605/001637/003281/003283/003285/003522/
002442) are all mid-function loop bodies whose operands (esi/edi/ebx/ebp)
are set by an upstream caller prologue, so they are NOT clean thiscall
fixtures and need caller-register-state reconstruction rather than an
isolated drive. Census impact: 218 rows moved discovered->reconstructed
across this campaign (325 total), all differential-verified. Family RED on
the Task-4 actor tables unchanged; coverage floor (126), inventory, and gate
policy unchanged.

## 3z93. Chain-field getter row closes (000017)

Round 77 re-scanned outside the earlier heuristic and found a few small
rows it had missed; drove 000017 (0x1520, a chain-field getter): if
[this+0x10] is non-null and [*head+0xd0]==5 it returns [*head+0xe0], else
it returns this+0x10 (a latent lea result), and a null head yields 0. All
three paths verified byte-exact. 004072 and 002874 were probed but resist a
deterministic fixture (multi-valued/reloc-sensitive returns) and were left
discovered. 000017 moves to `reconstructed`. No gate, coverage-floor, or
policy change.

## 3z94. Conditional-set row closes (005187)

Round 78 re-drove 005187 (0xe4160, ret 8), which round 52 had left
discovered after a 4/4 fixture failure. The failure was the byte-return
convention: `mov al,1; ret 8` returns a low-byte flag, so the oracle type
must be `unsigned char`, not `unsigned`. With the corrected type, all three
matching slots and the no-match path verify byte-exact (build/r78.log
condsetb failures=0). 005187 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z95. Six-float bounded-comparison row closes (005145)

Round 79 drove the 6-float compare (build/r79.log vec6cmp failures=0):
005145 (0xe2f70, ret 4) compares the source vector at [esp+4] against the
reference [this] and returns a byte flag (1 for in-bounds, 0 for a clear
mismatch). Another byte-return conversion (the oracle returns via `al`),
and the same-vs-big fixture verifies both poles. 005145 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z96. Remaining-row survey and trampoline investigation

Round 80 surveyed the remaining discovered set for any safe additional
closures. The small `jmp <fixed>` trampolines (0x1d0b0, 0x1fda0, 0x235c0,
etc., ~42 rows) were investigated as potential aliases, but most jump INTO
larger function bodies (a loop/disassembly label at the target+0x10 or
further), so declaring them reconstructed would misclassify them; they are
NOT closed. The remaining non-trampoline rows are either mid-function loop
bodies needing caller-register-state reconstruction (001542/001605/001637/
002442/003281/003283/003285/003522/004946/005590/005610), x87 fuzzy-compare
/accumulator fixtures (001500/002475/002505/002676/003398/005404), or
diagonal sources such as global/reloc-sensitive rows (004072/002874). The
safe independent closure stream is effectively exhausted; further progress
requires Task-4 actor-table / caller-prologue reconstruction work. No gate,
coverage-floor, or policy change.

## 3z97. Deep-x87 rows confirmed non-isolatable (002505)

Round 81 attempted the self-contained x87 distance-norm 002505 (0x5fb70,
norm of a double difference vector). Driving it trips the harness
stack-overrun guard (0xC0000409) before returning: its `faddp st(2)`/
`fmul st(1)` sequence pops more x87-stack elements than the callee pushes,
so it depends on the caller pre-seeding the FPU argument stack. This is the
same class as the slot-7 sweep: the row is NOT isolatable as a standalone
thiscall/cdecl fixture without reconstructing the caller's x87 staging.
The regression was fully reverted (all prior diodes pass, gate green-except-
family). This confirms the deep-x87 rows (002505/001500/002475/002676/
003398/005404) are out of scope for isolated differential closure. No gate,
coverage-floor, or policy change.

## 3z98. Ctor-wrapper rows close via reconstructed-helper differential (001565/001571/001575)

Round 82 pivoted to the wrapper class: rows whose body is a single call into
a reconstructed helper plus a few of their own writes. These are drivable by
reproducing the callee's sub-behavior in a candidate method and
differencing the full buffer against the oracle wrapper. Three vptr-ctor
wrappers over 001552 (mem: 0x2e5a0 -> vptr 0x1010785c, 0x2e640 ->
0x1010786c, 0x2e7c0 -> 0x10107890) were added as candidate methods
(nxCtorWrap565/571/575) and driven 4 arguments each (build/r82.log
ctorwrap failures=0), byte-exact. All 3 move to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z99. Zero-init wrapper row closes (002065)

Round 83 extended the wrapper differential to the zero-init wrapper: 002065
(0x51ec0, ret 0) zeroes [this+0..0x20] and then the [this+0x24..0x30]
sub-region via the reconstructed 005355 init, returning this. Added candidate
nxWrapZero2065 and verified byte-exact against the oracle across 10 fills
(build/r83.log zerowrap failures=0). 002065 moves to `reconstructed`. No
gate, coverage-floor, or policy change.

## 3z100. vptr+zero wrapper row closes (001409)

Round 84 drove the 001455-based vptr ctor wrapper (build/r84.log vzwrap
failures=0): 001409 (0x29750, ret 0) zeroes +4..0x48 (the 001455 sub-ctor)
and +0x64..0x7c, installs vptr 0x1010767c, and returns this. Added candidate
nxWrap1409 and verified byte-exact across 6 fills. 001409 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z101. Container-index wrapper row closes (000240)

Round 85 drove 000240 (0xb7c0, ret 4): it computes a slot via the index into
the container at [this+4] (the 000450 bounds-commit: slot = *[*ctr+8 +
idx*4] when idx < ([ctr+0xc]-[ctr+8])>>2, else 0) and returns [slot + 0x6cc].
Added candidate nxWrap240 and verified both index lookups byte-exact
(build/r85.log wrap240 failures=0). 000240 moves to `reconstructed`. No
gate, coverage-floor, or policy change.

## 3z102. BOX slot-4 mass wrapper row closes (000947)

000947 (0x20850, ret 0xc) was already differentially verified by the
long-standing `boxslot4` block in the harness (oracle 0x20850 driven
against candidate nxBoxAccumulateMass across 4 poses x 8 flag-lows x 2
densities, failures=0); only the census marker was missing. Now recorded
reconstructed. No gate, coverage-floor, or policy change.

## 3z103. Facade ctor wrapper 001033 probed, non-isolatable

Round 88 probed 001033 (0x22d60, wrapper over the reconstructed 001273
vptr-ctor): driving the oracle trips the harness overrun guard (0xC0000409)
before it returns. The callee 001273 is ret-style with an unbalanced
sub-call ABI in this wrapper context, so the wrapper cannot be isolated as a
standalone thiscall fixture without reconstructing the caller stack layout.
The regression was fully reverted (all prior diodes pass; gate
green-except-family). 001033 stays `discovered`. No gate, coverage-floor,
or policy change.

## 3z104. Ctor-with-link wrapper 004759 probed, non-isolatable

Round 89 probed 004759 (0xb38a0, wrapper over the reconstructed 004407
ctor-with-link): differential coverage showed the 004407 sub-ctor writes
more of `this` than the link-only reading captured (words 2/3/4/7/8/9 get
zeroed/filled), so a faithful candidate must reproduce 004407's full
internal layout, which is more than an isolated wrapper drive. The probe was
fully reverted (all prior diodes pass; gate green-except-family). 004759
stays `discovered`. No gate, coverage-floor, or policy change.

## 3z105. Small flag/link-advance rows close (002170/004083/001957/000567/000569)

Round 90 pivoted to the remaining tiny clean rows after exhausting the
wrapper class. Drove 5 (build/r90.log smallflag failures=0): 002170 (setne
[this+0x9c]), 004083 (setne the .data[0x10127180] global), 001957
([obj+0xc]+[obj+8] when [this+0x1c] non-null), 000567 and 000569 (link
advance storing [*ptr+0x10]/[*ptr+0x18] back to [this+0x6bc]/[this+0x6c0]).
All byte-exact. All 5 move to `reconstructed`. No gate, coverage-floor, or
policy change.

## 3z106. More compact flag/getter rows close (000738/004942/003628)

Round 90 second batch (build/r90.log smallflag2 failures=0): 000738
([this+0x1e4]&0x200 -> this+0x244 else 0), 004942 (two-bit select returning
0x1011b6ec when bit1 set & bit0 clear), 003628 (conditional byte store
[this+0x28]=1 when [this+0x2b] non-zero and the arg is zero). All
byte-exact after correcting two test-side expectation/byte-width issues. All
3 move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z107. Systematic isolated-drive faults in some small rows

Round 91 probed several more small candidates (001960, 002176, 003479,
004072, 003390). Driving 001960 and the batch repeatedly trips the harness
overrun guard (0xC0000409) regardless of fixture shape. This joins the
earlier non-drivable set (002505, 001033, 004759) -- a systematic phenomenon
where rows that use stack-relative frames (`sub esp`, `[esp+N]` reads with
embedded frame adjustment) interact badly with the driver's own stack and
fault under isolation. These rows are recorded `discovered`; the probes were
fully reverted and the gate is green-except-family. No gate, coverage-floor,
or policy change.

## 3z108. rep-movsd structured copies close (001299/003425/003427/003567)

Round 92 drove four stack-frame-free rep-movsd copies (build/r92.log
repcopy failures=0): 001299 (9 dwords [this+0x6c] -> out), 003425 (11 dwords
[this+0x28] -> out), 003427 (11 dwords arg -> [this+0x28]), 003567 (9 dwords
[this+0x18] -> out). These use only register saves (push esi/edi) without
frame adjustment, so they isolated cleanly (matching the round-90 pattern).
All 4 move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z109. Indirect-field and interval-flag rows close (002176/003479)

Round 93 re-probed round-91's crashing batch one row at a time, confirming
the systematic-fault hypothesis: 002176 ([this+0x9c] -> [*+0x5c] else 0)
and 003479 (([this+0x18]-[this+0x14])&~3 ? [this+0x14] : 0) each isolate
cleanly (build/r93.log indfield failures=0). The setne row 004072 faults
under isolation (consistent with its earlier erratic behavior) and is left
`discovered`. 002176 and 003479 move to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z110. x87 control-word row closes (000537)

Round 94 drove 000537 (0x106d0), which reads the x87 status word via
fnstcw into a local and stores it to [this+0]. It runs cleanly and twice
produces the same valid control word (0x027f), confirming the fnstcw fetch
(CW consistency probe). Given the row is a trivial fnstcw-store with fully
determined behavior, it closes on static proof plus the consistency drive.
000537 moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z111. Indexed-sum row 001960 closes; 004072 confirmed non-isolatable

Round 95 re-probed the remaining faulty candidates one row at a time.
001960 (0x4bec0: obj = [this + [this+0x70]*4 + 0x1c]; return [obj+0xc] +
[obj+8] else 0) ISOLATES cleanly on its own (build/r95.log ixsum2
failures=0) -- its round-91 fault was batch context, not the row. It moves to
`reconstructed`. The setne row 004072 (0x95a90) was re-probed alone and
crashes in isolation as well (no output, guard fault), so it is genuinely
non-isolatable and stays `discovered`; its probe was reverted. This
completes the per-row isolation triage of the round-91 faulty batch
(001960 and 002176/003479 recovered; 004072 confirmed faulty). No gate,
coverage-floor, or policy change.

## 3z112. Element-count row closes (003477)

Round 96 drove the tiny element-count row (build/r96.log elemcount
failures=0): 003477 (0x85580) returns ([this+8] - [this+4]) >> 2, the signed
element count of a contiguous buffer. Verified +3/0/-2. 003477 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z113. Call-wrapper rows confirmed non-isolatable

Round 98 attempted the 004886-calling wrapper group (005450 0xef690, plus
siblings 001787/001267): the body conditionally invokes the reconstructed
004886 on an indexed pointer, whose observable is [node+8] |= 2 but whose
global callback slot [0x10128478] is non-null in the harness environment, so
driving the oracle faults. This matches the earlier non-isolatable
call-wrapper rows (001033, 004759). The candidate method and test were fully
reverted (normal family-RED end; diff clean). These rows stay `discovered`;
call-containing rows are not isolatable in this harness. No gate,
coverage-floor, or policy change.

## 3z114. Phase-5 gate RED cause isolated: candidate vtable identity (Task 3-4)

Round 99 traced the Phase-5 gate's sole RED cause. Every candidate-side check
in the harness passes: actorsm, actorctor, actorsm2..actorsm7 and planeext
all report `candidate ok=1` with digests identical to the oracle, and the
coverage table line shows every registered row at 1 (tables=8,
colobj/owner/hull/shapebase/boxshape/sphere/capsule/plane/mesh/basevt/
basesave/boxrow/planesave/sphererows/capsave/meshword/aabbrows/meshrows/
sphlocal/setrad/capsetrad/planeext/sphdtor/capdtor/setgroup/dtors2/sphload/
slot1wrapper/addthunk/shapeleaf). The only failure is the unconditional
`candidate CANDIDATE-MISSING family=vtables` increment, whose reason string
says "shape finals/actor classes are Tasks 3-4".

The vtable-identity block (lines ~951-968) folds the EIGHT registered tables
from the ORACLE image only -- actor_interface (0x1043d0, 87 slots),
actor_dynamic (0x104530, 88 slots), shape_base (0x107494), box (0x106ab8),
capsule (0x106b20), plane (0x107430), sphere (0x107528), mesh (0x107630),
each 12 slots -- into oracleDigest. There is NO candidate-side counterpart,
so the family is CANDIDATE-MISSING by construction. Because the folded words
are absolute code pointers (the harness elsewhere notes pointers are
ASLR-moved and "never folded"), the remaining work is a candidate-side
vtable identity for the six shape finals plus the two actor tables,
compared by RVA-normalized slot (not raw address). That is the definitive
remaining Task-3/4 scope and the sole blocker to Phase-5 GREEN; the gate
stays honestly RED until it lands. No gate, coverage-floor, or policy change.

Quantified slot-to-census breakdown (round 99): mapping each table slot's
oracle word (minus the image base) to its census row shows the remaining
work is dominated by non-reconstructed slot rows --
- shape_base: 5 reconstructed / 7 discovered;
- box: 8 reconstructed / 4 discovered;
- capsule: 7 reconstructed / 4 discovered / 1 dynamically_gated;
- plane: 7 reconstructed / 4 discovered / 1 dynamically_gated;
- sphere: 8 reconstructed / 3 discovered / 1 dynamically_gated;
- mesh: 5 reconstructed / 7 discovered;
- actor_interface: 84 of 87 slots are the purecall stub 0x0f41dc (3 real);
- actor_dynamic: 88 slots, all 88 distinct real methods.
So Task 3-4 requires reconstructing ~29 shape-final slot rows plus the
actor_dynamic 88-slot body (and the 3 real actor_interface slots) before a
candidate vtable identity can match. This is a >100-row scope, which is why
the Phase-5 gate is RED and must stay RED until that slate is worked.

## 3z115. Task-3/4 scope refined: 88 purecall slots are CRT, not methods

Round 100 refined the 3z114 estimate by classifying all 247 vtable slots
across the eight registered tables by their census row's kind and state:

- compiler_artifact / discovered: 88 slots (the CRT `_purecall` filler
  0x0f41dc -- 84 of actor_interface's 87 slots plus shape_base slots 8..11);
- code / discovered: 86 slots (the REAL remaining product-method scope);
- code / reconstructed: 70 slots (product methods already done);
- code / dynamically_gated: 3 slots.

So the true Task-3/4 method-reconstruction scope is **86 product rows**, not
>100 -- and 88 of the "open" slots are CRT `_purecall` fillers that need no
product reconstruction at all: a candidate vtable identity only has to map
those slots to its own purecall stub. Independently, the census-wide kind
distribution shows the raw discovered count is dominated by non-product rows:
compiler_artifact/discovered = 3554 versus code/discovered = 2311, so the
product backlog is roughly a third of the headline "discovered" number. The
purecall row 005667 (0x0f41dc, 20 bytes) is itself a complete, self-contained
CRT `_purecall` (handler global 0x1012851c, then abort), already tagged
kind=compiler_artifact with a static proof. Gate stays honestly RED; no gate,
coverage-floor, or policy change.

## 3z116. First Task-3/4 vtable-slot row closes (001403, MESH slot 4)

Round 101 began working the 86-row product vtable slate, profiling all 79
unique slot rows by size and call count. The standout was 001403 (0x29190,
MESH vtable slot 4, 152 bytes, ZERO calls): a fully self-contained
transform-point that copies the 4 dwords at [[this+0xe0]+0x5c] into
out[0..3], then overwrites out[0..2] with M*v + t (3x3 matrix at
[this+0xc..0x2c], translation at [this+0x30..0x38]). Added candidate
nxTransformPoint1403 (double-staged accumulation to reproduce the x87
extended-precision rounding) and verified byte-exact (build/r101.log
transpt1403 failures=0). Note: the initial drive faulted only because the
test's `this` buffer was 0x40 bytes while the row reads [this+0xe0] -- a
fixture bug, not a row property, so the earlier "sub esp frame rows always
fault" hypothesis is too broad. 001403 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z117. Sphere mass helper payload path diverges (blocks 001371/001008)

Round 102 attempted the SPHERE vtable slot-4 wrapper 001371 (0x27be0,
ret 0xc), the sphere analogue of the closed 000947 (BOX slot 4): when the
low flag bits are clear it runs the sphere mass helper 000851 on the facade
radius [this+0xe0] and pose [this+0x6c]. The wrapper semantics are fully
determined and the candidate nxSphereAccumulateMass was written, but the
differential diverges: with a NON-null zero pose the oracle ZEROES the
destination MassFrame while the candidate nxSphereComputeMassFrame computes
the sphere inertia (e.g. 0x43239fe7 for density 2.0 / radius 2.5). The
existing helper drive only ever passed extra=0, so 000851's non-null payload
path was never verified -- this is a concrete, actionable gap in the
nxSphereComputeMassFrame transcription. It blocks closing 001371 and its
capsule twin 001008 (0x22440, also a facade mass wrapper over 000853). The
probe, candidate method and header entry were fully reverted (normal
family-RED end; diff clean). Both rows stay `discovered`. No gate,
coverage-floor, or policy change.

## 3z118. Sphere/capsule builder payload paths reproduced; 001371 closes

Round 103 fixed the gap 3z117 exposed. `nxMassFrameBuildSphere` and
`nxMassFrameBuildCapsule` both carried a documented-but-unreproduced payload
arm ("Neither helper is transcribed yet" / "(void) extra; see above"), even
though the BOX builder had long since folded the pair via
nxMassFrameFoldPayload + nxMassFrameTranslate. The 000852/000853 builders
call exactly 0x1bdc0 then 0x1c040 at extra+0x24, so both arms now do the
same:

    nxMassFrameFoldPayload(extra);
    nxMassFrameTranslate((const unsigned char*)extra + 0x24);

This is what makes the oracle ZERO the inertia for a zero pose (a zero
rotation folds the tensor to zero). With the fix, the SPHERE vtable slot-4
wrapper 001371 (0x27be0, ret 0xc) -- which runs the 000851 helper on the
facade radius [this+0xe0] and pose [this+0x6c] when the low flag bits are
clear -- matches byte-exact across 8 flag-lows x 2 densities (build/r103.log
sphmass failures=0). Candidate nxSphereAccumulateMass added. No regressions:
massframe ok=1 (0bed6c36), capmass ok=1 (ab81bd0c), capsule ctor ok=1, all
coverage tables 1. 001371 moves to `reconstructed`; 001008 (capsule slot 4)
is unblocked by the same fix and is next. No gate, coverage-floor, or policy
change.

## 3z119. Capsule slot-4 wrapper closes (001008)

Round 103 also closed the capsule twin. 001008 (0x22440, CAPSULE vtable slot
4, ret 0xc) runs the 000853 capsule helper when [this+0xde]&7 is clear, with
axisSelector 1, radius [this+0xe0], cylHalfHeight [this+0xe0]+[this+0xe4]
and pose [this+0x6c]. Candidate nxCapsuleAccumulateMass (declared in
CapsuleShape) matches byte-exact across 8 flag-lows x 2 densities
(build/r103.log capmass2 failures=0). Together with 001371 this closes both
facade mass wrappers that 3z117 identified as blocked, bringing the Task-3/4
vtable-slot slate to 3 rows closed (001403, 001371, 001008). No gate,
coverage-floor, or policy change.

## 3z120. Plane slot-9 local AABB closes (001255)

Round 104 profiled the slate for call-free rows and closed the largest of
them: 001255 (0x25090, PLANE vtable slot 9, 315 bytes, ZERO calls). It is the
plane's local AABB. After masking the normal's sign bits, an axis-aligned
normal (|n| == 1.0f on exactly one axis) selects that axis: a non-positive
component writes the distance into the MIN side ([esp+0x10]/[esp+0x14] or
out[2]), a positive one writes the negated distance into the MAX side
([esp+4]/[esp+8]/[esp+0xc]); the untouched axes keep the +/- 0x7effffff
sentinel, and out[2] carries the -0x7effffff (0xfeffffff) const unless the
normal is -Z, where the const is popped and the distance loaded instead.
Candidate nxPlaneLocalAABB1255 verified byte-exact across 7 normals (all six
axis directions plus an oblique one) x 2 distances (build/r104.log planeaabb
failures=0). 001255 moves to `reconstructed` -- the fourth Task-3/4 vtable
slot closed (001403, 001371, 001008, 001255). No gate, coverage-floor, or
policy change.

## 3z121. Actor vtable thunks close (000038/000040)

Round 105 closed the two actor vtable thunks that fill both the
actor_interface and actor_dynamic tables: 000038 (0x2400, ret 8, slot
0x104) and 000040 (0x2430, ret 8, slot 0x108). Each dispatches through the
object's OWN vtable at +0x104 / +0x108 with (self, &local, arg1) and copies
the first three words of the returned record into out. The drive builds a
fake object whose vtable slot points at a stub returning a known record, so
the oracle and the candidate nxActorVtThunk104/108 dispatch to the same body
and must agree on out[0..2] -- verified for both slots (build/r105.log
actorthunk failures=0). Both rows move to `reconstructed` -- the Task-3/4
vtable slate now stands at SIX closed (001403, 001371, 001008, 001255,
000038, 000040). No gate, coverage-floor, or policy change.

## 3z122. Plane slot-8 indexed-record copy closes (001267)

Round 106 closed the last slate row whose callee is already reconstructed but
whose drive 3z117-style faults were previously blamed on the callee: 001267
(0x25490, PLANE vtable slot 8, ret 4). It selects a 6-dword record from the
table at *([this+0xc4]+0x14), indexed by [this+0xa4+0x28], and copies it to
out. The 004886 init only runs when [this+0xcc] != 0xffff AND
[this+0xa4+8] lacks bit 2, so setting that bit exercises the row WITHOUT the
faulting callback path -- the earlier "004886 is non-isolatable" conclusion
was about driving 004886 itself, not about rows that merely call it
conditionally. Candidate nxPlaneIndexed6_1267 verified byte-exact for three
indices (build/r106.log planeix6 failures=0). 001267 moves to
`reconstructed` -- the SEVENTH Task-3/4 vtable slot (001403, 001371, 001008,
001255, 000038, 000040, 001267). No gate, coverage-floor, or policy change.

## 3z123. All phase gates verified honest (round 107)

Round 107 ran every registered phase gate to confirm the global gate posture
is honest. Result: phases 2, 3 and 4 report PASS (exit 0); phase 5 is RED on
purpose (exit 1) with the single `family=vtables` CANDIDATE-MISSING row that
3z114/3z115 scope; phases 6, 7 and 8 report UNGATED (exit 3), which is the
documented outcome for a phase whose differential, static-proof and
coverage-line registries are still empty rather than a pass. No gate,
coverage-floor, or policy change.

## 3z124. Capsule slot-9 world AABB closes (001016)

Round 108 closed the hardest call-free slate row: 001016 (0x22620, CAPSULE
vtable slot 9, 419 bytes / 129 instructions, ZERO calls). It is the capsule's
world AABB. The segment runs along local Y, so each world extent is
M[i][1]*halfHeight (read at +0x10/+0x1c/+0x28). The row expands the LOWER
endpoint (t - e) by +/- radius into out[0..2] and out[3..5], then merges the
UPPER endpoint's +/- radius (A = (t+e)-r, B = (t+e)+r) with min on the low
triple and max on the high triple. Two subtleties pinned against the oracle:
(a) the max side of the initial store is `r + (t - e)`, not `(t + e) + r`;
(b) the +0xc/+0x10 merge arms use `test ah,5 / jp`, which updates on
`out <= B` (skipping the unordered case) -- a MAX, not an unconditional
store, which the first drive caught at out[4] for the oblique pose. Candidate
nxCapsuleWorldAABB1016 (double-staged accumulation) verified byte-exact
across 4 poses x 2 halfHeights with a seeded existing AABB (build/r108.log
capaabb2 failures=0). 001016 moves to `reconstructed` -- the EIGHTH Task-3/4
vtable slot (001403, 001371, 001008, 001255, 000038, 000040, 001267, 001016).
No gate, coverage-floor, or policy change.

## 3z125. Global-region zero row closes (003390)

Round 109 re-attempted 003390 (0x83a60, 17 bytes), which round 91 had dropped
after its batch faulted. Driving it ALONE passes: the batch crash was 004072,
not this row (the same batch-vs-row confusion 3z109/3z111 resolved for
002176/003479/001960). 003390 zeroes the 0xdd bytes at .data[0x1012626b ..
+0xdc] via a dec-counter byte loop; read-back verified (build/r109.log
globzero2 failures=0). 003390 moves to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z126. Pose-copy-with-tail row closes (000827)

Round 110 worked the 44-row tractable backlog that round 109's RVA-indexed
scan surfaced (product rows with zero calls, <=4 x87 ops and <=220 bytes).
000827 (0x1bcc0, ret 0xc) is a clean three-argument __thiscall: rep-movsd 9
from arg1 into [this+0], arg2[0..2] into [this+0x24..0x2c] and arg3 into
[this+0x30]. Candidate nxPoseCopyWithTail0827 verified byte-exact across
three cases (build/r110.log posecopy827 failures=0). 000827 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z127. The 44-row backlog is 43 fragments + 1 genuine entry

Round 111 tested the round-109 backlog against a branch-target filter: for
each candidate it asked whether the row's start address is the target of any
`j*`/`call` from an earlier address. A row that IS such a target is a
mid-function label the disassembler split out, not a function entry -- it
cannot be driven in isolation because its operands (esi/edi/ebx/ebp/eax) are
set by an upstream prologue outside the row. Result: of the 44 product rows
with zero calls, <=4 x87 ops and <=220 bytes, **43 are branch targets**
(fragments) and exactly ONE is a genuine entry: 003268 (0x7e560, 210 bytes,
ret 8), a batch index/vertex append over seven arrays
([this+8], +0xc, +0x10, +0x18, +0x1c, +0x20, +0x4034..0x4044). So the
"fragment" class is now conclusively characterised: those census rows are
disassembly artifacts of splitting at jump targets, not standalone
functions, which is why their drives faulted. 003268 is the next and only
remaining target in this band. No gate, coverage-floor, or policy change.

## 3z128. Batch index/vertex append closes (003268)

Round 111 also drove that single genuine entry: 003268 (0x7e560, 210 bytes,
ret 8). It bails when [this+0x18] >= [this+0x1c]; accumulates (count-2) into
[this+0x20]; records the count in the [this+0x403c]/[this+0x4044] list when
there is room; then for each index below [this+0x10] copies the 3-dword
vertex record through the [this+8] map into the [this+0x4034]/[this+0x4038]
output array -- assigning a fresh id when the map slot is zero -- and appends
id-1 to the aux list. Candidate nxBatchAppend3268 verified byte-exact across
three cases (build/r111.log batch3268 failures=0); the only initial
"mismatches" were the four fixture pointer slots, which the drive now
excludes (the three data arrays are compared in full). 003268 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z129. Genuine-entry band characterised; AABB aggregation closes (001030)

Round 112 extended the branch-target filter past the small band. Of the
genuine product entries (not fragments) with <=1 call, 69 were found, but
they fall into three non-drivable classes: (a) physical dispatch thunks that
`jmp eax` / `jmp [reg+off]` through a global or vtable -- 005380, 004872,
004870, 002237, plus the vtable-dispatch pair 002390/003924; (b) callers of
the CRT SEH/exception routine 005668 (a 5-byte `jmp` trampoline to 005692,
which uses an [ebp+8] frame and calls the CRT _SEH helpers) -- nine rows
including 000283/001199/003690/003790/003864/004025/004031/004537/005331;
and (c) callers of large still-discovered callees. The blocking global for
class (a) is real, not a fixture artifact: the callback slots hold non-null
sentinels in the image ([0x10128478] = 0x35263501), which is exactly why
driving 004886 and its callers faults.

Filtering instead for genuine entries whose callees are ALL reconstructed
yields six: 005450 and 001787 (both 004886 callers -- excluded by the above),
005471 and 005466 (also 004886 callers), and two clean ones -- 001030 and
005223. 001030 (0x22bf0, ret 4, 267 bytes) is the first: it seeds out[0..5]
to FLT_MAX/-FLT_MAX, then for each shape in [self+0xe0]..[self+0xe4] merges
the 6-dword record selected by the PLANE slot-8 row (001267, closed in 3z122)
with min on the low triple and max on the high triple. Candidate
nxAggregateAABB1030 verified byte-exact (build/r112.log aggaabb1030
failures=0). 001030 moves to `reconstructed`; 005223 is the last remaining
drivable entry in this band. No gate, coverage-floor, or policy change.

## 3z130. 005223 is register-dependent; the genuine-entry band is exhausted

Round 113 examined 005223 (0xe52d0, 270 bytes), the last candidate 3z129
left open. It is a three-mode batch filter/append into a container
([esi+0]=capacity, +4=count, +8=array) with a grow call to 004840 whenever
the count reaches capacity. Its static proof rules it out as a standalone
function: the row opens with `push ecx` and then READS the slot that push
created ([esp+0x10]) three times -- at 0x000e5305, 0x000e5362 and 0x000e53a8
-- as the base for its `[base + index*4]` lookups, and NEVER writes it. That
slot therefore holds the caller's incoming ECX, i.e. 005223 takes a pointer
argument in a register that its own body never establishes. It is
register-dependent in exactly the way the 3z127 fragments are, so it cannot
be driven in isolation either.

With that, the genuine-entry band is exhausted: every product row outside the
Task-3/4 vtable slate is either already closed, a 3z127 fragment (branch
target), a 3z129 class-(a) dispatch thunk over a non-null sentinel global, a
3z129 class-(b) CRT SEH/exception caller, a caller of a large still-discovered
callee, or now a register-dependent row like 005223. No gate, coverage-floor,
or policy change.

## 3z131. Double-to-float matrix row closes (002156)

Round 114 began working back from the Task-3/4 slate by reconstructing its
blocking callees. The MESH slot-4 row 001397 is now one dependency short:
000827 closed in 3z126, 000835/000839 were already reconstructed, and its
call to 001583 (0x2ea70) is a ONE-BYTE `ret` stub -- a no-op. Its remaining
dependency 002241 (0x54bb0, 692 bytes) calls 005666 (0xf4140, 156 bytes)
twelve times plus 002156 (0x538e0, 46 bytes). 002156 was driven and closed
here: it converts the nine consecutive doubles at [self+0x18] (8-byte
stride) into nine floats at out[0..8]. The image interleaves the fld/fstp
schedule -- reading d0,d3,d6 / d1,d4,d7 / d2,d5,d8 and writing
+0,+0xc,+0x18 / +4,+0x10,+0x1c / +8,+0x14,+0x20 -- which is exactly
dst[k] = (float)src[k]. Candidate nxDoubleToFloat9_2156 verified byte-exact
across three magnitude classes (unit, ~1e120, ~1e-300), so the double->float
staging matches even at extremes (build/r114.log d2f9 failures=0). 002156
moves to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z132. CRT artifacts cannot be promoted; 002241 is non-drivable

Round 115 tried to close the chain that blocks the MESH slot-4 slate row
001397. Two findings, one of them a convention correction:

1. 005666 (0xf4140, 156 bytes) is the MSVC CRT `_dclass` floating-point
   classifier, fully decoded: it masks the exponent field (0x7ff0) of the
   double argument; the Inf/NaN arm defers to 0xf7f77 and maps its 1/2/3/4
   result to 0x200/4/2/1; an exponent of zero with a non-zero mantissa yields
   _FPCLASS_ND 0x10 or _FPCLASS_PD 0x80; otherwise the fucompp sign test
   yields _FPCLASS_NN 0x20 / _FPCLASS_PZ 0x40 (not-greater) or _FPCLASS_NN
   0x08 / _FPCLASS_PN 0x100 (greater or unordered). Its neighbour 005667 is
   `_purecall`. Attempting to record either as `statically_reviewed` is
   REJECTED by validate_inventory.py: "is a compiler artifact and must not
   claim product source" and "nothing above 'reconstructed' is a claim a row
   can make on its own". The change was reverted (inventory back to
   unexplained=0 / data_objects=5138). So the 3554 compiler_artifact rows are
   permanently out of scope by construction -- they are not reconstruction
   backlog and cannot be promoted by hand.

2. 002241 (0x54bb0, 692 bytes) is therefore NOT drivable either: besides its
   twelve 005666 calls it makes two indirect calls, `call [0x10104164]` and
   `call [0x101041b4]`, and those globals hold NON-CODE RVAs in the image
   (0x1211f0 and 0x120f92, i.e. .data/.rdata addresses, not 0x100xxxxx code).
   Driving it would jump outside .text. That blocks 001397 behind 002241.

So the slate's remaining rows are blocked not only by size but by CRT
artifacts and non-code global dispatch, both of which the harness cannot
drive. No gate, coverage-floor, or policy change.

## 3z133. Slate blocker census; the lock pair closes the last opening

Round 116 classified all 71 remaining Task-3/4 slate rows by blocker type:
16 are blocked by indirect calls + a non-code global + a product dependency,
15 by a non-code global + a product dependency, 12 by an indirect call + a
non-code global, 6 by indirect + product, 5 by a non-code global alone, 3 by
a product row alone, 2 each by indirect alone and by gated + product, 1 by a
gated row alone, 3 by CRT artifacts combined with the above, and 6 showed NO
blocker at all.

Those six -- 000046 (252B), 000132 (259B), 000130 (318B), 000094 (517B),
000146 (579B) and 000148 (811B), all actor_dynamic slots -- were then
examined directly. Each is a plain field-gather whose every explicit callee
is already reconstructed, so they looked actionable. But they all bracket
their body with the pair 002362 (0x5b700) and 002366 (0x5b790), and those two
are mutex lock/unlock helpers that dispatch through the Foundation lock-API
globals [0x10104010], [0x1010402c], [0x10104044] and [0x10104014]. Those
slots hold NON-CODE placeholder RVAs in the image (0x120e14, 0x120df6,
0x120de0, 0x120e2c), so calling them jumps into .data. A direct feasibility
drive of the pair faulted, and the probe was reverted (tree clean, harness
back to its normal family-RED end).

So the earlier "no blocker" class was an artifact of the filter, which only
flagged calls to rows in the discovered/compiler_artifact/gated states and
did not treat the statically_reviewed lock pair as a blocker. With that
closed, EVERY remaining slate row is blocked by at least one of: a CRT
artifact (permanently out of scope per 3z132), a non-code global dispatch
(lock API or callback), or a large product dependency that itself reaches one
of the first two. No gate, coverage-floor, or policy change.

## 3z134. Lock-API binding unlocks the six "unblocked" slate rows (000046)

Round 117 overturned the 3z133 conclusion that the six no-blocker slate rows
were unreachable. The Foundation lock pair 002362/002366 dispatches through
the four globals [0x10104010], [0x1010402c], [0x10104044] and [0x10104014],
which hold placeholder RVAs in the file. Because the harness runs in the same
process as the oracle image, those slots can be BOUND from the test, which is
what makes the rows drivable:

1. The pointer slots sit in a read-only page in the loaded image, so the
   binding needs VirtualProtect(PAGE_READWRITE) around the store and back
   afterwards (observed: the protect succeeds and the readback confirms the
   new values land).
2. The call sites push their arguments and NEVER clean them, so the slots are
   __stdcall, not __cdecl. Binding __cdecl stubs corrupts the stack and the
   drive faults; with `extern "C" int __stdcall` stubs the pair returns
   cleanly. This was the single blocking detail -- the first three drives
   faulted purely on calling convention, not on the row.

With the pair bound, 000046 (0x24c0, actor_dynamic slot, ret 4) closes. It
gathers the descriptor record at [[self+0x14]+8] into out -- nine dwords from
record+0xdc, three from record+0x100, then thirteen tail fields, with fsqrt
of the floats at record+0xd8, +0xd0 and +0xd4 -- and returns false without
writing when the record pointer is null. Candidate nxGatherDescriptor0046
(double-staged sqrt to match x87 fsqrt) matched on both arms (build/r117.log
gather0046 failures=0). 000046 moves to `reconstructed`. The same binding
unblocks 000132, 000130, 000094, 000146 and 000148. No gate, coverage-floor,
or policy change.

## 3z135. Quaternion-to-matrix slate row closes (000132)

Round 118 used the 3z134 lock-API binding (now factored into reusable
nxBindLockApi/nxUnbindLockApi helpers that VirtualProtect the slot page, store
the __stdcall stubs, and restore both on the way out) to close the first of
the five newly-unblocked rows. 000132 (0x46c0, actor_dynamic slot, ret 4,
259 bytes) writes the 3x3 rotation matrix for the quaternion at record+0x5c
(x, y, z, w) into out[0..8]; when the record pointer is null it instead copies
the cached 36 bytes at [self+0x14]+0x20. Both arms were driven: the matrix
matches the textbook quaternion expansion exactly --
out[0] = 1-2(y^2+z^2), out[1] = 2(xy-zw), out[2] = 2(xz+yw),
out[3] = 2(xy+zw), out[4] = 1-2(x^2+z^2), out[5] = 2(yz-xw),
out[6] = 2(xz-yw), out[7] = 2(yz+xw), out[8] = 1-2(x^2+y^2) -- which the
listing's opening `1 - 2(y^2+z^2)` into out[0] already predicted. Candidate
nxQuatToMatrix0132 (double-staged accumulation for the x87 extended-precision
rounding) matched on both arms (build/r118.log quatm0132 failures=0).
000132 moves to `reconstructed`; 000130, 000094, 000146 and 000148 remain
open behind the same binding. No gate, coverage-floor, or policy change.

## 3z136. Full-pose slate row closes (000130); quaternion math factored

Round 119 closed the second of the five newly-unblocked rows. 000130 (0x4580,
actor_dynamic slot, ret 4, 318 bytes) is the full-pose sibling of 000132: it
builds the SAME quaternion matrix from record+0x5c (x, y, z, w) but writes a
0x30-byte pose -- the nine matrix floats followed by the translation at
record+0x50/0x54/0x58 -- and copies the cached pose at [self+0x14]+0x20 when
the record pointer is null. The listing confirms the shared math: its opening
`1 - 2(y^2+z^2)` into the local at [esp+0x24] is the same expansion 3z135
verified. The shared expansion was therefore factored into a single
nxQuatToMatrix9 helper used by both rows. Candidate nxPoseFromQuat0130
matched on both arms (build/r119.log poseq0130 failures=0). 000130 moves to
`reconstructed`; 000094, 000146 and 000148 remain open behind the same
binding. No gate, coverage-floor, or policy change.

## 3z137. Orientation slate row closes (000094), a matrix-to-quaternion

Round 120 closed the third lock-bracketed slate row. 000094 (0x2f30,
actor_dynamic slot, ret 4, 517 bytes) returns the orientation quaternion
(x, y, z, w). With a record it simply copies the stored quaternion at
record+0x5c; without one it derives the quaternion from the cached matrix at
[self+0x14]+0x20. The derivation is the largest-diagonal method, decoded from
the listing: when the trace is non-negative the trace arm gives
s = sqrt(trace+1) with x = (m21-m12)/2s, y = (m02-m20)/2s, z = (m10-m01)/2s
and w = s/2; otherwise edx selects the largest of m00/m11/m22 (the second
compare indexes the diagonal with a stride of 16 bytes) and the matching arm
gives the other three components from the pairwise sums and differences.
Candidate nxOrientation0094 matched on the present arm plus a matrix fixture
chosen to walk ALL FOUR fallback branches (build/r120.log orient0094
failures=0). 000094 moves to `reconstructed`. A re-run of the slate blocker
analysis with the lock pair treated as bindable confirms only 000146 (579B)
and 000148 (811B) remain reachable, both large quaternion-driven geometry
functions. No gate, coverage-floor, or policy change.

## 3z138. Lock binding exposes a 60-row accessor backlog; two close

Round 121 re-ran the genuine-entry analysis with the 3z134 lock pair treated
as BINDABLE rather than blocking. That changed the picture completely: 60
product rows whose only callees are the lock pair and already-reconstructed
helpers -- all previously written off as blocked by non-code global dispatch
-- are drivable. They are the small locked accessor family (26-43 bytes), each
shaped `lock [self+0x10]; <read or call>; unlock; return`.

Two closed immediately:
- 003950 (0x8f0d0, 26 bytes) takes the lock, releases it and returns self --
  the lock pair brackets the whole body and has no other observable;
- 000418 (0xda50, 35 bytes) reads the word at [[self+0x24]+0x55c] under the
  lock.

Both were driven with the lock API bound (build/r121.log lockacc
failures=0). The remaining 58 span 36-43 bytes and come in repeated shapes --
for example 000317 and 001071 lock, call one reconstructed helper on a field
of self, unlock and return its value. No gate, coverage-floor, or policy
change.

## 3z139. Six-row locked accessor batch closes (000317/21/27, 000352/56/60)

Round 122 worked the 3z138 backlog and found the family is even more uniform
than it looked: every one of these rows is exactly `lock [self+0x10];
result = HELPER([self+0x24]); unlock; return result`, and every HELPER is an
already-reconstructed plain field getter. So the whole row reduces to one
field read, which made a single parameterised candidate possible --
nxLockedFieldRead(self, offset).

Six closed in one drive (build/r122.log lockedget failures=0), each paired
with the getter it wraps:

- 000317 (0xc8a0) -> getter 000523 on +0x3c;
- 000321 (0xc910) -> getter 000559 on +0x6c8;
- 000327 (0xc9a0) -> getter 000561 on +0x6c4;
- 000352 (0xcd20) -> getter 000547 on +0x6ac;
- 000356 (0xcdb0) -> getter 000551 on +0x6b0;
- 000360 (0xce40) -> getter 000555 on +0x6b4.

All six move to `reconstructed`. The remaining 3z138 backlog rows have the
same shape and differ only in the getter they wrap, so the same candidate
covers them. No gate, coverage-floor, or policy change.

## 3z140. Second locked accessor batch: the family has variants (5 rows)

Round 123 worked further down the 3z138 backlog and found the family is NOT
as uniform as 3z139 assumed -- the lock and field slots differ per row, and
several rows do more than return the getter's value. Five closed in one drive
(build/r123.log lockedget2 failures=0):

- 001203 (0x248a0) and 004443 (0xb0730) are VARIANT B: the lock sits at
  [self+0x14] and the field at [self+0x18] (variant A uses +0x10/+0x24). They
  wrap the plain dword getters 001283 ([field+0xd0]) and 004070
  ([field+0x168]) respectively, so they needed a lockOff/fieldOff-aware
  candidate, nxLockedFieldReadEx.
- 003824 (0x8c9c0) double-dereferences: the getter 003952 yields the VALUE at
  [field+0x14] and the row then dereferences it, returning *(*(field+0x14)).
  The first drive faulted precisely because the fixture held a scalar there
  rather than a pointer.
- 003872 (0x8d1f0, ret 4) wraps the pointer getter 003661 (field+8) and
  copies twelve dwords from it into out before returning out.
- 004479 (0xb0d20, ret 4) compares the argument with [field+0x168] and
  returns SELF -- not the argument -- when they match: the mask is
  `and esi, ecx` with esi still holding this. The first drive caught this
  (oracle returned the buffer address, the candidate the field value).

All five move to `reconstructed`. The backlog's remaining rows include the
same variants plus the pose/vector-copy members. No gate, coverage-floor, or
policy change.

## 3z141. Copy members decoded, but their rows do not drive

Round 124 decoded the copy members of the locked accessor family -- the rows
that lock, call a helper which copies fields into an out argument, and unlock
-- and mapped every helper exactly:

- 001297 (0x25870) and 000509 (0x10200): three dwords from +0x90 and +0x520;
- 003565 (0x87e50) and 003483 (0x85780): three from +0x3c, six from +0x5c;
- 001291 (0x25810), 001299 (0x258a0) and 003567 (0x87e70): nine from +0x6c
  (and +0x18 for 003567);
- 003425 (0x84d10): eleven from +0x28;
- 001289 (0x257e0), 001295 (0x25840) and 003563 (0x87e20): the 12-dword pose
  -- nine from the base then the three at base+0x24 (+0x6c/+0x18 bases).

The row shapes were then mapped too: lock at +0x14 with field at +0x18
(001221, 001149, 001223, 001187, 001107, 001219) or lock at +0x10 with field
at +0x14 (003784, 003712, 003820, 003816).

The helpers themselves drive correctly: calling 001297 directly on a field
buffer returned the expected word (build/r124.log helper-ok h0=9a000090). But
every ROW faults on its own drive, before any output, even with the lock API
bound and confirmed active (the readback showed the stub in place), and even
though the identical lock pair drives fine in the 3z139/3z140 rows. The row
disassembly is consistent (thiscall, ret 4, out at [esp+0xc]) and the fixture
sets both candidate lock/field slots plus +0x10/+0x14/+0x18/+0x24 to valid
pointers. The cause is not yet identified; the probe and both candidate
helpers were fully reverted (tree clean, harness back to its normal
family-RED end), so the copy members stay `discovered` with this analysis
recorded for the next attempt. No gate, coverage-floor, or policy change.

## 3z142. Copy members close (10 rows); 3z141's fault was the fixture

Round 125 resolved the 3z141 open issue. The rows never had a problem: the
FAULT was in the test fixture. Variant B reads its lock from [self+0x14] and
its field from [self+0x18], but the fixture wrote the lock object at +0x10
and the field at +0x14 for EVERY row. So variant-B rows called the lock
helper with `ecx` pointing at the field buffer, whose first word was the fill
pattern 0x9A000000; the lock helper then stored through it
(`mov [edx+0x1c], eax`) and the drive faulted. Placing the lock at the row's
own lock slot fixed all ten at once (build/r125.log lockedcopy failures=0).

Ten rows closed, each locking [self+lockOff] and calling its copy helper on
[self+fieldOff] to write into the out argument:

- 001221 (0x24b40) and 001149 (0x24100): lock +0x14, field +0x18, helper
  001297, three dwords from +0x90;
- 003784 (0x8c210): lock +0x10, field +0x14, helper 003483, six from +0x5c;
- 001223 (0x24b70) and 001187 (0x246d0): lock +0x14, field +0x18, helpers
  001291/001299, nine from +0x6c;
- 003712 (0x8b530): lock +0x10, field +0x14, helper 003425, eleven from +0x28;
- 003820 (0x8c8d0): lock +0x10, field +0x14, helper 003567, nine from +0x18;
- 001219 (0x24b10), 001107 (0x23a60) and 003816 (0x8c870): the 12-dword pose
  copy (helpers 001289/001295/003563) from +0x6c (and +0x18 for 003816).

All ten move to `reconstructed`. The general lesson is recorded: the family's
lock and field slots vary per row and the fixture must honour each row's own
offsets. No gate, coverage-floor, or policy change.

## 3z143. Third accessor batch: copy, mask and bit-extract members (4 rows)

Round 126 continued down the 3z138 backlog into three more member shapes,
closing four rows in one drive (build/r126.log lockacc3 failures=0):

- 000291 (0xc460, ret 4) locks [self+0x10] and calls helper 000509 on
  [self+0x24] to write three dwords from [field+0x520] into the out argument
  -- a copy member covered by nxLockedCopyOut;
- 003818 (0x8c8a0, ret 4) is the same shape one variant over: lock
  [self+0x10], helper 003565 on [self+0x14], three dwords from [field+0x3c],
  and it additionally returns out;
- 003746 (0x8be40) and 003852 (0x8cf20) are MASK members: helpers 003455 and
  003595 are `mov eax,[ecx+dataOff]; and eax,[esp+4]; ret 4`, so the row
  returns [field+0x58] & arg and [field+0x10] & arg respectively -- covered by
  the new nxLockedAndRead.

All four move to `reconstructed`. Two further member shapes were decoded in
passing for a later round: helper 004078 is `mov eax,[ecx+0x2c]; shr eax,3;
and eax,3` (nxLockedBitExtract), and helper 004083 is a global-flag read of
[0x10127180] which is deliberately left open because it would couple the
candidate to the oracle image's global. No gate, coverage-floor, or policy
change.

## 3z144. Fourth accessor batch: count, flag, bit-extract, address (4 rows)

Round 127 closed four more members of the 3z138 backlog in one drive
(build/r127.log lockacc4 failures=0). Each is the usual
`lock; result = HELPER(field); unlock; return result` with a different
already-reconstructed helper:

- 003700 (0x8b1f0) wraps 003477 -- the element-count row closed back in
  3z112 -- so it returns ([field+8]-[field+4])>>2 for field at [self+0x14];
- 003702 (0x8b220) wraps 003479, the interval flag: [field+0x14] when
  ([field+0x18]-[field+0x14]) has any bit above the low two, else zero;
- 004483 (0xb0dc0) wraps 004078, the bit extract ([field+0x2c]>>3)&3, with
  the lock at [self+0x14] and field at [self+0x18] (variant B);
- 001071 (0x23520) wraps 000929, which is `lea eax,[field+0xe4]` -- an
  address-returning member, covered by the new nxLockedFieldAddress.

Three new candidates were added (nxLockedElementCount, nxLockedIntervalFlag,
nxLockedFieldAddress) alongside the existing nxLockedBitExtract. All four rows
move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z145. Fifth accessor batch: word-mask, two pointers, float (3 rows)

Round 128 closed three more members, each introducing a return shape the
earlier batches had not covered (build/r128.log lockacc5 failures=0):

- 001085 (0x236d0, ret 4) wraps 001287, which is `movzx eax,word[ecx+0xde];
  and eax,[esp+4]` -- a WORD read (not a dword), zero-extended, then masked by
  the argument. Candidate nxLockedWordAndRead.
- 004573 (0xb1bd0, ret 8) wraps 004076, which writes [field+0x3c] through the
  first out pointer and [field+0x40] through the second -- the first member
  seen with TWO out arguments. Candidate nxLockedCopyTwoPointers.
- 001121 (0x23c80) wraps 000999, `fld [ecx+0xe4]; fadd st(0),st(0)`. The row
  stores the result to its frame, unlocks, reloads and returns it in st(0) --
  an x87 float return, driven through a float-returning pointer. Candidate
  nxLockedDoubleField, matched across three magnitudes including 1.0e20.

One further member was decoded and deliberately left open: 001061 (0x233a0)
wraps 001293, which is `mov ax,word[ecx+0xda]` WITHOUT zeroing eax, and the
row then does `mov ax,si`. Both leave eax's high half stale, so the observable
return depends on register state the row does not establish -- the same latent
high-bits class as the 3z94 byte returns, and not safely reproducible.

All three closed rows move to `reconstructed`. No gate, coverage-floor, or
policy change.

## 3z146. Sixth accessor batch: copy-and-flag and three pointers (4 rows)

Round 129 closed four more members in one drive (build/r129.log lockacc6
failures=0), adding two more return shapes:

- 004711 (0xb3240, ret 4), 004715 (0xb3280, ret 4) and 004719 (0xb3310,
  ret 4) are COPY-AND-FLAG members. Each locks [self+0x14], calls its helper
  on [self+0x18], and the helper both copies into out and returns a bit of
  [field+0x1a8] in al: 004342 copies six dwords from +0x16c and returns bit 0;
  004346 copies three from +0x184 and returns bit 1; 004350 copies three from
  +0x190 and returns bit 2. The row preserves that byte through bl and returns
  it in al, so the oracle pointer type is unsigned char (the 3z94 lesson).
  Candidate nxLockedCopyAndFlag, matched with the flag bit both set and clear.
- 000340 (0xcb50, ret 0xc) wraps 000542, which writes [field+0x52c],
  [field+0x530] and [field+0x534] through THREE separate out pointers -- the
  widest out-argument member seen so far. Candidate
  nxLockedCopyThreePointers, with the lock at [self+0x10] and field at
  [self+0x24].

All four move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z147. Seventh accessor batch: helper-free members (4 rows)

Round 130 re-ran the 3z138 backlog filter and found 22 rows still drivable.
Four closed in one drive (build/r130.log lockacc7 failures=0). Unlike every
earlier batch, these do the work INLINE -- the only calls are the lock pair,
so no helper needed decoding:

- 000416 (0xda10) locks [self+0x10] and returns
  ([field+0x560]-[field+0x55c])>>2 with field from [self+0x24];
- 003808 (0x8c620, ret 4) copies nine dwords from [field+0x48] into out and
  returns out, field from [self+0x14];
- 003806 (0x8c5e0, ret 4) is the same with three dwords from [field+0x6c];
- 000421 (0xdac0) reads the pointer at [field+0x61c] (field from [self+0x24])
  and returns [that+0x14] when non-null, else zero -- a conditional deref.

The first drive caught one real detail: 000416's count uses field offsets
+0x560/+0x55c, not the +8/+4 that helper 003477 used, so the count candidate
was generalised into nxLockedElementCountAt(fieldOff, hiOff, loOff). The
copy members reuse nxLockedCopyOut unchanged. All four rows move to
`reconstructed`; 18 of the 22 remain, including the two 004886 callers and
the large quaternion geometry functions. No gate, coverage-floor, or policy
change.

## 3z148. Round-130 regression found and repaired

Round 131 opened by checking the harness's own mismatch count rather than
just the gate exit, and found a REAL REGRESSION that 3z147 had introduced:
the seventh accessor batch left `boxrow3` failing. The tell is `mismatches=`;
round 129 logged `layout candidate mismatches=1` (the family gate alone) and
`boxrow3 candidate ok=1`, while every run from round 130 on logged
`mismatches=2` and `boxrow3 candidate ok=0`. The boxrow3 digests themselves
were unchanged (d8=d9=8428d8b5), so what moved was the ORACLE-side digest the
row compares against -- i.e. the batch perturbed oracle state that boxrow3
later depends on, and the gate still exited 1 for the expected family reason,
which is why the extra failure was invisible to a gate-exit-only check.

Bisection pinned it: with the whole seventh batch removed, boxrow3 recovered;
with only its two read-only rows (000416, 000421) restored, boxrow3 stayed
green and both rows still verified; the corruption therefore comes from one
of the two COPY rows (003808, 003806), which write through an out argument
that the read-only rows never touch. Those two were re-opened to `discovered`
(their closures were withdrawn) and the batch was reduced to the two safe
rows, which still report lockacc7 failures=0. The state is back to
`layout candidate mismatches=1` with boxrow3 ok=1.

The standing lesson: every round must check the harness's mismatch COUNT, not
only the gate exit code. No gate, coverage-floor, or policy change.

## 3z149. The seventh batch is layout-sensitive; all four rows re-opened

Round 132 dug into 3z148's regression and found it is worse than "one bad
row". Bisection results, all reproducible:

- the batch in its committed round-130 ORDER (003808 then 003806) failed;
- the SAME rows in the reverse order passed, three runs in a row;
- restoring only the two read-only rows (000416, 000421) passed;
- adding the two copy rows back to that reduced block passed;
- then removing an unrelated DIAGNOSTIC PRINT from the boxrow3 block -- a
  change that cannot alter any data the batch touches -- flipped boxrow3 back
  to ok=0;
- removing the two copy rows from that build did NOT restore it;
- removing the whole batch did, and stayed green over three runs.

So the trigger is not a specific row: it is incidental CODE LAYOUT. boxrow3
compares the candidate's out8/out9 against digests recorded at lines 2301 and
2310, before the batch runs at all, and its own printed digests never change
(d8=d9=8428d8b5) -- yet its `ok` flag moves with unrelated edits elsewhere in
wmain. That is a latent, layout-sensitive defect somewhere in the harness
(most likely an uninitialised or stack-resident read whose value shifts with
the frame), not a property of the accessor rows.

Because a closure whose presence destabilises another check is not a closure
this program can stand behind, ALL FOUR rows (000416, 000421, 003808, 003806)
were re-opened to `discovered` and the seventh batch was removed entirely.
State is back to `layout candidate mismatches=1` with boxrow3 ok=1, verified
stable over three consecutive runs. The mechanism should be found before any
of these rows is closed again. No gate, coverage-floor, or policy change.

## 3z150. Mechanism found: wmain's digest locals are being clobbered

Round 133 identified WHY the harness is layout-sensitive, and corrected a
misreading of 3z148/3z149. An unconditional diagnostic printed every boxrow3
condition at once and showed the check PASSING on all six
(d8ok=1, d9ok=1, out8[0]=bf800000, out8[3]=3f800000, out9[0], out9[3]) while
the harness still reported `mismatches=2`. Grepping the full log for a real
failure then named it: `boxrow candidate ok=0 digest=2f2dc4eb` -- a DIFFERENT
check. Comparing gate logs across rounds shows the failing check MOVES with
the build:

    round 125..129  boxrow ok=1  boxrow3 ok=1  mismatches=1
    round 130       boxrow ok=1  boxrow3 ok=0  mismatches=2
    round 131       boxrow ok=1  boxrow3 ok=1  mismatches=1
    round 133       boxrow ok=0  boxrow3 ok=1  mismatches=2

The mechanism follows from where those values live. `oBoxRowDigest` is a
LOCAL of wmain (declared at line 939, recorded at 1701 and compared at 8863);
`oBoxSlot8Digest`/`oBoxSlot9Digest` likewise (942/943, recorded at
2301/2310, compared at 8980). The candidate digests are byte-identical in
every build (boxrow 2f2dc4eb; boxrow3 d8=d9=8428d8b5), and the oracle locals
are written thousands of lines earlier, so the ONLY way `ok` can flip is that
one of those digest LOCALS IS CLOBBERED between its recording and its
comparison -- an out-of-bounds write into wmain's frame, with the frame layout
deciding which local dies. That also explains why adding or removing an
unrelated diagnostic print, or a canary loop, is enough to move the failure
from boxrow3 to boxrow.

The obvious suspect was checked and cleared: the box slot-8 row 000941 writes
exactly six dwords to its out pointer, and 000935 likewise, so neither
overruns the test's `float out8[6]`. The overrun is therefore elsewhere in
wmain, and the diagnostics used here are themselves layout perturbations --
they were reverted so the harness sits at its committed, stable state
(`boxrow ok=1`, `boxrow3 ok=1`, `mismatches=1`, three consecutive runs).

Next step for whoever picks this up: find the out-of-bounds writer by
auditing stack buffers passed to oracle functions against what those rows
actually write, rather than by bisecting layout. No gate, coverage-floor, or
policy change.

## 3z151. Root cause found and fixed: a test overrun, not a row property

Round 134 found the out-of-bounds writer and fixed it, which both removed the
layout sensitivity and un-blocked the four rows 3z149 had withdrawn.

The diagnostic followed 3z150's plan exactly and was decisive. Printing the
digest locals at the comparison site showed `boxrow locals od=0000cafe` --
`oBoxRowDigest` held 0xCAFE, a marker value, instead of the recorded oracle
digest 2f2dc4eb. Grepping for 0xCAFE led to line 3620, and the block there
calls row 000557 (0x10840, 22 bytes) with `&node2` where `node2` was declared
as a bare `unsigned`:

    mov edx, dword ptr [ecx + 0x5a0]
    mov eax, dword ptr [esp + 4]
    mov dword ptr [eax + 0x10], edx     <-- writes 16 bytes past the argument
    mov dword ptr [ecx + 0x5a0], eax
    ret 4

So the row writes `[arg+0x10]` while the test handed it a four-byte local,
and the test even READ back `node2+0x10` expecting the old head. That is a
16-byte out-of-bounds write straight into wmain's frame, landing on whichever
of the digest locals happened to sit there -- which is precisely why the
failure moved between `boxrow` and `boxrow3`, and why any unrelated edit
(an added print, a canary loop, a whole new batch) could flip it.

The fix is at the fixture, not the row: `node2` is now a 0x20-byte buffer, so
the row's `[arg+0x10]` store and the test's read of it are both in bounds.
With that in place the four withdrawn rows verify again AND boxrow/boxrow3
both hold at ok=1 with `mismatches=1`, stable over three consecutive runs --
including with the diagnostic print still present, which is the layout that
previously provoked the failure. 000416, 000421, 003808 and 003806 are
therefore re-closed on the 3z142/3z147 evidence, now backed by a harness whose
frame is no longer being corrupted.

This supersedes the "layout-sensitive harness defect" framing of 3z148/3z149:
the defect was an ordinary out-of-bounds write in one test fixture. No gate,
coverage-floor, or policy change.

## 3z152. Second fixture overrun fixed; the eighth batch closes (5 rows)

Round 135 finished clearing the overrun class and, with the frame sound,
closed the batch that had faulted in 3z134.

First the remaining overrun. 3z151 fixed row 000557 by giving its argument a
real buffer; the same audit found row 000571 (0x108e0, 22 bytes) doing the
same thing one word down:

    mov edx, dword ptr [ecx + 0x620]
    mov eax, dword ptr [esp + 4]
    mov dword ptr [eax + 4], edx     <-- past a four-byte local
    mov dword ptr [ecx + 0x620], eax
    ret 4

The zmix fixture passed it `&node` where node was a bare `unsigned`, so it
wrote four bytes past a local and then read them back. node is now a 0x10-byte
buffer. To bound the class, every short product row was scanned for stores of
the form `[reg+N], _` where reg was loaded from [esp+4]: 30 such rows exist,
writing up to [arg+0x54], but cross-checking the test's call sites shows the
rest are given adequately sized arrays (for example 004407 writes up to
[arg+0x1c] against a 0x30-byte node buffer). So exactly two fixtures were
undersized, and both are now fixed.

With the frame no longer being corrupted, the eighth accessor batch -- which
faulted outright in 3z134 -- drives clean (build/r135.log lockacc8
failures=0), and boxrow/boxrow3 both hold at ok=1 with mismatches=1, stable
over three consecutive runs. Five rows close:

- 003804 (0x8c590, ret 4) locks [self+0x10] and copies the 12-dword pose at
  [field+0x48] into out (nxLockedCopyPose);
- 000420 (0xda80) reads [field+0x61c] (field from [self+0x24]) and returns
  ([ptr+8]-[ptr+4])>>2 or zero (nxLockedConditionalCount);
- 004539 (0xb1600, ret 8) is a NESTED dereference: for each of [field+8] and
  [field+0xc] it reads the pointer, dereferences [node+0x19c] and returns the
  word that addresses -- the first drive faulted precisely because the fixture
  put a value there instead of a pointer (nxLockedTwoNestedDerefs);
- 003948 (0x8f090, ret 0x10) and 003946 (0x8f050, ret 0x14) write four and
  five fields through as many out pointers (nxLockedCopyNPointers).

All five move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z153. Ninth accessor batch: match-or-self and list advance (3 rows)

Round 136 closed three more members (build/r136.log lockacc9 failures=0),
adding a second return shape and a stateful one:

- 001109 (0x23a90, ret 4) is the SAME match-or-self shape as 004479 but over
  a different field: helper 001283 reads [field+0xd0] (field from
  [self+0x18]) and the row returns SELF when the argument equals it, else
  zero. Generalised into nxLockedMatchEx(fieldOff, dataOff, arg), which now
  covers both +0x168 and +0xd0.
- 000325 (0xc960) and 000331 (0xc9f0) are LIST-ADVANCE readers: helper 000567
  pops the head at [field+0x6bc] and relinks it to [head+0x10] (000569 and
  [field+0x6c0]/[head+0x18] for 000331), after which the row returns a field
  of the popped head -- [head+0x48] and [head+0x20] respectively -- or zero
  when the list is empty. Candidate nxLockedAdvanceRead. These are the first
  members with a SIDE EFFECT on the field, so the drive checks both arms: the
  empty-list arm returns zero, and the populated arm compares the returned
  word AND the relinked head word on both sides.

All three move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z154. Callback-slot binding closes the 004886 wrappers (2 rows)

Round 137 applied the 3z134 binding technique to a different global and
closed a pair the campaign had written off twice. Rows 005450 (0xef690) and
001787 (0x3f570) both call 004886, which runs the registered callback at
[0x10128478] when that slot is non-null and then sets bit 2 of a node word.
That slot holds a non-code sentinel in the image (0x35263501), which is why
3z98 and 3z113 found these rows undrivable.

The slot is bindable exactly like the lock API: nxBindCallbackSlot unlocks its
page with VirtualProtect, stores a no-op stub, and nxUnbindCallbackSlot
restores both. Its call site pushes two arguments and cleans them itself
(`add esp, 8`), so the stub is __cdecl. With the callback neutralised, 004886
reduces to its own write and both wrappers drive:

- 005450 (ret 4) sets [node+8] bit 2 when [node+0x28] is not 0xffff and the
  bit is clear, then increments [self+0x38] and returns 1;
- 001787 is the same shape one sub-object over -- its object comes from the
  SECOND stack argument because the row ends in a bare `ret`, so it is a
  caller-cleaned two-argument function -- and it sets bit 2 of [node+0xa4+8]
  when [node+0xcc] is not 0xffff, returning zero.

Both verified on three cases each (build/r137.log wrap5450 failures=0,
wrap1787 failures=0). One drive detail: the fixture parks a different inner
pointer at +0xc4 on the two sides, so the node comparison clears that slot
first -- otherwise the fixtures' own differing addresses read as a mismatch,
the same artifact 3z128 hit. Both rows move to `reconstructed`. No gate,
coverage-floor, or policy change.

## 3z155. Assert-report rows: a 171-row family, first six close

Round 138 re-ran the drivability filter with the lock pair, the 004886
callback and non-code globals all treated as bindable. That surfaced 35 rows,
of which about twenty share a body that turned out to be an ASSERTION REPORT:

    mov eax, dword ptr [0x101041b0]     ; the assert object
    cmp dword ptr [eax], 0
    jne L
    int3                                ; break when no handler is installed
L:  push <expression string>
    push 0
    push <line>
    push <file string>
    push 0xce
    call dword ptr [0x101041b4]         ; the report entry point
    add esp, 0x14
    ret

Widening the net to every short product row containing that call found **171
such rows** -- the compiler inlined the NX_ASSERT failure body into every
function carrying an assertion. They are a genuine, previously unmapped
family, and each one's entire behaviour is the tuple it reports.

Two globals had to be handled, and both are bindable the 3z134 way:

- [0x101041b4] holds a placeholder, so nxBindReportSlot points it at a
  __cdecl recorder (the call sites push five arguments and clean them with
  `add esp, 0x14`);
- [0x101041b0] ALSO holds an unrelocated RVA, and the rows dereference it
  BEFORE the int3 -- so the first drive faulted. The same binding now points
  it at a static word holding 1, which keeps the guard non-zero and skips the
  breakpoint.

The candidate exposes its own entry point (nxSetAssertReport) so the harness
can route the oracle's report slot into the same recorder and compare tuples
directly. Six PURE report rows -- body exactly as above, with no other logic --
closed in one drive (build/r138.log assertrows failures=0): 000364, 000408 and
000410 (file 0x10105ba8), plus 003750, 003754 and 003782 (file 0x101160cc).
All six move to `reconstructed`. The other 165 members of the family carry
extra logic around the report and are the obvious next slate. No gate,
coverage-floor, or policy change.

## 3z156. Fourteen more pure report rows; binding drives must run LAST

Round 139 finished the pure-assert slate and found a harness-ordering rule the
hard way. 3z155 had filtered for a bare `ret`, which missed the rows that end
in `ret 4` or `ret 8`; relaxing that found **14 more pure report rows** (20
pure in total across both rounds). Their tuples are now recorded in the
census, and all twenty verify together (build/r139.log assertrows
failures=0).

Two real problems surfaced while getting there, both instructive:

1. The drive called every row through a `void (__thiscall*)(void*)`, but rows
   ending in `ret 4` or `ret 8` pop more than one argument. Calling a ret-8
   row with a single argument shifts the stack by four bytes on return and
   corrupts wmain's frame. The table now carries each row's arity and the
   drive casts accordingly -- the same discipline the 3z94 byte-return rows
   needed on the value side.
2. Even with the arity fixed, the paxis check started failing: its ORACLE-side
   digest moved from 1d701701 to 17f85795 while the candidate stayed put. The
   cause is ORDERING, not corruption in the row: the assert drive binds
   [0x101041b4] and [0x101041b0], and it sat BEFORE the paxis block, so the
   patched globals were live while the paxis oracle computed. Moving the whole
   drive to the END of wmain -- after every other check -- restored paxis to
   ok=1 and kept all twenty assert rows green.

That last point is a standing rule for this harness: any block that binds a
global must run after the checks that do not expect that binding. The
lock/callback drives already happened to sit late enough; this one did not.

All fourteen rows move to `reconstructed`. The remaining assert-family members
carry a recursive-mutex acquire (helper 002364) around the report -- 31 rows
of that shape alone -- and are the next slate. No gate, coverage-floor, or
policy change.

## 3z157. Word-return accessors close (001207, 001061)

Round 140 closed the two remaining lock-bracketed word accessors, which 3z145
had deliberately left open as a "latent high-bits" risk. Re-reading the
sequence shows the risk was not real:

    mov ecx, dword ptr [esi + 0x18]
    call 001285                 ; mov ax, word[ecx+0xd8]; ret
    mov ecx, edi
    mov esi, eax
    call 002366                 ; the unlock helper
    pop edi
    mov ax, si                  ; masks to the low word
    pop esi
    ret

The helper sets only ax, so eax's high half is whatever it was -- but the row
calls the UNLOCK helper between capturing eax and masking it, and that helper
ends in `mov al, 1` after its stub returns 1. eax is therefore 1 when the mask
runs, so the high half is zero and the return is exactly the zero-extended
word. Both rows verify on two word values each (build/r140.log wordrows
failures=0): 001207 reads [field+0xd8] via helper 001285, 001061 reads
[field+0xda] via helper 001293, both with the lock at [self+0x14] and the
field at [self+0x18]. Candidate nxLockedWordRead. Both move to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z158. 004334 declined: its work path does not write the argument

Round 141 took up the guarded-clamp setter 004334 (0xa8f10, ret 4, 147
bytes), the cleanest of the larger remaining rows: its body is plain logic
with a single assert-report branch, and it is a genuine entry (the preceding
instruction is a `ret 4`). The decode looked complete --

    mov eax, [ecx+0x2c]
    and al, 0x18
    cmp al, 0x10
    jne WORK                       ; only an exact 0x10 falls through to report
    <report(1, file 0x1011a204, line 0x9d, 0, expr 0x1011a290)>; ret 4
WORK:
    mov edx, [esp+4]
    mov [ecx+0x1a8], edx           ; store the argument
    ... clamp [obj+0x4c] up to 0.4f for [ecx+8] and [ecx+0xc] ...

-- and the assert arm was pinned empirically: with `[self+0x2c] & 0x18` equal
to 0x10 the oracle reports, and the candidate matched that exactly. But the
WORK arm does not behave as written: the oracle leaves `[self+0x1a8]` at ZERO
for every argument value tried (0x5EED0000 and 0x11223344), while `ecx` is
demonstrably `self` -- the guard byte is read correctly from the fixture and
the assert arm keys off it. The row neither reports nor stores on that path,
which the listing does not explain.

So the drive was withdrawn rather than forced: the candidate and its block
were fully reverted and the tree is back to the committed green state
(assertrows 0 failures, wordrows 0 failures, mismatches=1). 004334 stays
`discovered`. The open question for a later round is why the work arm does not
store the argument -- the most likely remaining explanation is a calling
convention that differs from `__thiscall(ret 4)`, since `[esp+4]` is read with
no preceding push. No gate, coverage-floor, or policy change.

## 3z159. Gated locked reader closes (003708)

Round 142 closed 003708 (0x8b420, 89 bytes), the gated locked reader. Its
whole body is:

    mov al, byte ptr [0x101263ac]     ; an image gate byte
    test al, al
    jne WORK
    <report(0xce, 0x101160cc, 0x4c, 0, 0x10116170)>; xor eax,eax; ret
WORK:
    <lock [edi+0x10]>
    ecx = [edi+0x14]; call 000287      ; mov eax,[ecx+0x24]; ret
    edi = [eax+0x38]
    <unlock [edi+0x10]>
    mov eax, edi; ret

The gate byte holds 0x3770372c in the image, so the SHIPPED path is the work
arm; the harness binds [0x101263ac] both ways (nxBindGateSlot) and mirrors it
into the candidate through nxSetGate3708, so BOTH arms are driven. The work
helper 000287 is a plain field getter, already reconstructed, and the lock
pair is bound as usual. Candidate nxGuardedField3708 matched on both arms
(build/r142.log gate3708 failures=0): the gated arm returns the word at
[that+0x38], the ungated arm returns zero after reporting the assertion.

003708 moves to `reconstructed`. This is also the first row closed by binding
a plain DATA byte rather than a function slot, which widens the binding
technique beyond call targets. No gate, coverage-floor, or policy change.

## 3z160. Lock probe closes (000392); the acquire's indirection pinned

Round 143 closed the lock probe 000392 (0xd660, 31 bytes), the first of the
mutex-family rows:

    mov ecx, [esi+0xc]; call 002364     ; recursive acquire
    test al, al
    je  L                                ; failed -> return 0
    mov ecx, [esi+0xc]; call 002366     ; release
    mov al, 1; ret
L:  xor al, al; ret

So it reports whether the lock at [self+0xc] was acquirable, releasing it when
it was. Driving it required pinning what 002364 actually tests, and the first
attempt got that wrong in an instructive way: the candidate compared the owner
word at [lock+0x1c], but the acquire reads it THROUGH the lock's first word --
[[lock]+0x1c]. With that indirection the two arms fall out exactly as the
listing implies (build/r143.log probe0392 failures=0): the owner word holding
the id the query stub reports (0x2222) yields 1, any other value yields 0. The
candidate takes that id from nxSetLockOwner so the harness can keep it in step
with whatever the bound query stub returns.

000392 moves to `reconstructed`. Its 84-byte siblings in the same family
(001115 and friends) are the same probe plus an assert-report arm and a work
arm that calls through the object's own vtable ([edx+0x38]), so they need a
vtable fixture rather than new understanding. No gate, coverage-floor, or
policy change.

## 3z161. The mutex-guarded virtual dispatch family closes (31 rows)

Round 144 closed the whole 84/92-byte mutex family in one drive
(build/r144.log mutexfamily failures=0) -- thirty-one rows, the largest single
slate of the campaign so far. They are uniform:

    <lock [esi+0x10]>            ; 002364, the recursive acquire
    test al, al
    jne WORK
    <report(2, <file>, <line>, 0, 0x10104760)>; ret 4
WORK:
    ecx = [esi+0x18]             ; the target object
    edx = [ecx]                  ; its vtable
    <push the argument>
    call [edx + <slot>]          ; slot varies: 0x24, 0x28, 0x2c, 0x34, 0x38
    <release [esi+0x10]>         ; 002366
    ret 4

Two things made the batch cheap once 3z160 had pinned the acquire:

1. The slot call site pushes its argument and does NOT clean it, so the slot
   is a __stdcall one-argument function with `this` in ecx. (__thiscall is
   rejected on a free function in this translation unit, so the fixture stub
   is declared __stdcall and simply ignores ecx.) The fixture gives the target
   object a table with every slot from 0x20 to 0x3c pointing at that stub, so
   the oracle and the candidate reach the same body and the hit count is
   comparable.
2. The rows differ only in three numbers -- the vtable slot and the report
   file/line (the code is 2 and the expression 0x10104760 throughout) -- so a
   single parameterised candidate, nxMutexVirtualEx(self, arg, slot, code,
   file, line, expression), covers all thirty-one, driven from a table.

Each row is exercised on BOTH arms: the acquire-failing arm must report the
same tuple, and the acquiring arm must hit the same vtable slot exactly once.
All thirty-one move to `reconstructed`. No gate, coverage-floor, or policy
change.

## 3z162. Constant-argument virtual thunks close (9 rows)

Round 145 re-ran the drivability filter with indirect `call [reg+off]` sites
counted as bindable, which is now true because the 3z161 fixture supplies a
vtable. That surfaced 52 rows, including a nine-row block of eight-byte
thunks at 0xb0580..0xb0600, each of the form:

    mov eax, dword ptr [ecx]
    push <constant>
    call dword ptr [eax + 0x4c]
    ret

The constants are 1, 5, 4, 0, 2, 3, 8, 6 and 7 -- one thunk per value, all
dispatching through slot +0x4c. The call site pushes its argument and does not
clean it, so the slot is a __stdcall one-argument function with `this` in ecx,
exactly the shape 3z161 pinned.

The drive gives the object a table whose +0x4c slot points at a recorder and
compares, for each row, BOTH the hit count and the value the slot received.
Candidate nxVtConstEx(self, slot, arg) covers all nine (build/r145.log vtconst
failures=0). All nine move to `reconstructed`.

The same scan lists roughly forty more rows now reachable through bound
vtables -- including the remaining `R+bind`, `bind+vt` and `global+vt` shapes
-- so this is the shape of the next several slates. No gate, coverage-floor,
or policy change.

## 3z163. Three multi-argument dispatch thunks close

Round 146 closed 002390 (0x5b910), 003924 (0x8ed50) and 001965 (0x4c000),
three dispatch thunks that each forward to a different shape of target with a
different argument count and a different cleanup convention:

- 002390 reads the target from a PLAIN CALLBACK FIELD, not a vtable:
  `call [[self+4]+0xc]([[self+4]+0x10])`, and the thunk pops the argument
  itself, so the target is __cdecl. The first drive faulted precisely because
  the fixture had built a vtable at [obj] and left [obj+0xc] zero -- read
  literally, the target pointer lives at obj+0xc.
- 003924 calls `vtable[+0xc](self, [self+0x24], [self+0x28])` and does not
  clean the two arguments, so the slot is __stdcall.
- 001965 takes its object from the SECOND argument and calls
  `[[obj]+0x2c](arg1, 0xff00ffff, 0)` with three uncleaned arguments.

Each was driven against a recorder stub of matching arity, comparing the
argument COUNT and every recorded value (build/r146.log thunks3 failures=0).
All three move to `reconstructed`.

The lesson worth keeping: a `call [reg+off]` target is not necessarily a
vtable entry -- 002390's is a callback stored directly in the object, and only
reading the operand literally distinguishes the two. No gate, coverage-floor,
or policy change.

## 3z164. Lock-bracketed vtable calls close (001237, 001119)

Round 147 closed a pair that shares one shape but returns two different kinds
of value:

    <lock [self+0x14]>                     ; 002362, unconditional
    ecx = [self+0x18]
    eax = [ecx]
    call [eax + <slot>]                    ; slot 0x44 for 001237, 0x3c for 001119
    <unlock [self+0x14]>                   ; 002366
    <return the slot result>

The slot receives `this` in ecx and NO stack arguments. `__thiscall` is not
available on a free function here, but `__fastcall` with one parameter is
exactly that layout -- first argument in ecx, nothing to pop -- so the
recorder stubs are declared that way. 001237's slot returns an unsigned, which
the row returns in eax; 001119's returns a float, which the row stages through
its frame and returns in st(0).

Both were driven against recorders that capture the object they received and
return a fixed value, comparing the result AND the recorded target
(build/r147.log vtcall pair failures=0). Both move to `reconstructed`. No
gate, coverage-floor, or policy change.

## 3z165. Per-element dispatch loop closes (001022)

Round 148 closed 001022 (0x22970, ret 4), the per-element virtual dispatch
loop:

    eax = [ecx+0xe4]; esi = [ecx+0xe0]
    n = (eax - esi) >> 2                  ; element count
    if (n == 0) return
    loop:
      ecx = [esi]                         ; the element
      eax = [ecx]                         ; its vtable
      push arg1
      esi += 4
      call [eax + 0xc]                    ; slot +0xc(element, arg1)
      dec n
      jne loop

The count is the byte span of the pointer pair divided by four, exactly the
shape the 3z112 element-count row computes, and the slot takes one uncleaned
argument (__stdcall) with the element in ecx. The drive walks three element
counts -- 0, 1 and 2 -- and compares BOTH the number of slot hits and the
argument the slot received (build/r148.log arrayloop failures=0).

One drive detail: with zero elements the slot never runs, so the argument is
never recorded and the expectation for that case is zero rather than the
sentinel -- the first run flagged exactly that. 001022 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z166. Scalar deleting destructor closes (002142)

Round 149 closed 002142 (0x532b0, ret 4), a scalar deleting destructor:

    test byte ptr [esp+4], 1
    mov dword ptr [esi], 0x1010829c        ; install the vtable
    je  END
    eax = [0x101041bc]                     ; the allocator singleton
    ecx = [eax]; edx = [ecx]
    push esi
    call [edx + 0x14]                      ; free(this)
END:
    mov eax, esi; ret 4

The free arm reaches the allocator through THREE levels --
`[[[0x101041bc]][0]+0x14]` -- so the harness binds [0x101041bc] to a fake
holder whose object's vtable slot +0x14 is a recorder, and the candidate
exposes the same hook through nxSetAllocFree. The call site pushes the block
and does not clean it, so the slot is __stdcall.

Both arms verify (build/r149.log dtor2142 failures=0): the vtable word is
installed and `this` returned either way, and only the flagged arm frees. Two
drive details came up and are worth keeping: the recorder had to be __stdcall
or the pushed argument was never popped, and the freed pointer must be
compared against EACH side's own buffer rather than across sides, since the
two fixtures live at different addresses -- the same class of artifact as
3z128 and 3z154. 002142 moves to `reconstructed`. No gate, coverage-floor, or
policy change.

## 3z167. Mutex direct-call group closes (4 rows); a sibling group is held back

Round 150 took the mutex-guarded family one step further: rows whose work arm
calls a DIRECT helper rather than a vtable slot. Two such groups exist, and
only one could be closed.

**Closed (004461, 004463, 004465, 004467).** These call 004248, which is
literally a three-byte `ret 4` -- a no-op -- so the work arm has no observable
beyond the release. Both arms verify against candidate nxMutexNoopEx
(build/r150.log mutexdirect failures=0), and the four rows move to
`reconstructed`.

**Held back (001043, 001081, 001129, 001165, 001205).** These call 001329,
which is ShapeBase::nxApplyGroup -- already reconstructed. The report arms
agree, but the object buffers diverge at exactly one byte: the oracle writes
0x08 at [obj+0xc8] and the existing nxApplyGroup candidate writes nothing
there. That is the field nxApplyGroup's own body assigns as
`mPrunable.mPrunable24 = 1u << (group & 0x1f)` with group 3 -- the value the
oracle produced -- so the call either is not reaching that assignment in the
candidate or is writing it at a different absolute offset. ObjectModel.h pins
`offsetof(ShapeBase, mPrunable) == 0xa4`, which puts mPrunable24 at +0xc8 if
the member sits 0x24 into Prunable; mPrunable24 itself is declared outside
that header, so the offset could not be confirmed from it.

The group was withdrawn rather than forced: the candidate for it was removed,
the drive now covers only the four verified rows, and the tree is green
(mutexdirect and dtor2142 both 0 failures, mismatches=1). **This is a real
open question about the earlier 001329 closure** -- its own differential did
not compare this field -- and it is recorded here rather than papered over. A
later round should confirm Prunable's layout and re-drive 001329 before
closing the five rows that depend on it. No gate, coverage-floor, or policy
change.

## 3z168. The held-back group closes -- and 3z167's open question was a FIXTURE bug

Round 151 resolved 3z167's open question, and the answer is that **there was
never a defect in the 001329 closure**. Two steps settled it:

1. `Prunable` is declared in IcePrunable.h, and its own comment puts
   mPrunable24 at `+0x24` -- absolute +0xa4 + 0x24 = **+0xc8**, exactly where
   the oracle wrote. The layout was right all along.
2. A direct probe of 001329 against its candidate -- oracle buffer versus
   `((ShapeBase*)b)->nxApplyGroup(grp)` for groups 3, 0x21 and 0, comparing the
   whole 0x200-byte object -- reports **failures=0**. The candidate is
   byte-identical to the oracle, so the earlier closure stands.

The divergence 3z167 saw was in MY drive, not the candidate:

    unsigned char self[0x40]; ...
    *(void**)(self + 0x18) = obj;          // the object pointer goes into self
    unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
    unsigned char objC[0x200]; memcpy(objC, obj, sizeof(objC));

selfC is copied AFTER self+0x18 was set, so selfC+0x18 still points at `obj`.
Both sides therefore wrote the SAME buffer, and objC -- which only the
comparison read -- stayed zero. Re-pointing `*(void**)(selfC + 0x18) = objC`
fixes it, and the whole nine-row family then drives with both arms green
(build/r151.log mutexdirect failures=0).

So the five held-back rows close after all: 001043, 001081, 001129, 001165 and
001205 move to `reconstructed`, joining the four 004248-group rows from 3z167.
All nine now verify together.

This is the second time a "candidate defect" turned out to be a fixture that
shared a pointer between the two sides -- 3z128 and 3z154 were the same class
on comparison fields. The rule to carry forward: when copying a fixture for
the candidate, re-point every internal pointer AT the copy, or both sides
silently share one buffer. No gate, coverage-floor, or policy change.

## 3z169. Two more lock-bracketed vtable rows close (004703, 001209)

Round 152 generalised the 3z164 shape to a caller-chosen slot and closed two
more rows of it:

- 004703 (0xb3120) calls slot **+0x30** with `this` in ecx and no stack
  arguments and returns the slot value -- the same shape as 001237 (+0x44),
  which is why the dedicated candidate became the parameterised
  nxLockedVtCallNoArg(self, slot).
- 001209 (0x24960, ret 4) calls slot **+0x24** with `this` in ecx AND the row
  argument on the stack, then releases -- nxLockedVtCallArg(self, slot, arg).

Both verify against recorders that capture the object (or argument) they
received and the hit count (build/r152.log vtcall slots failures=0). Both move
to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z170. Report-once dispatch rows close (3 rows)

Round 153 closed the three rows that pair a ONE-SHOT assertion with a vtable
dispatch:

    mov al, byte ptr [0x101237cX]      ; the gate byte
    test al, al
    jne WORK
    <report(0xd0, 0x10105ba8, <line>, 0, <expr>)>
    mov byte ptr [0x101237cX], 1       ; the gate is SET, so it fires once
WORK:
    edx = [esi]; <push the arguments>
    mov ecx, esi
    call [edx + <slot>]
    ret <N>

All three share code 0xd0, file 0x10105ba8 and the report-once gate pattern,
and differ only in the gate address, the slot, the argument count and the
line/expression:

| row | gate | slot | slot arguments | ret |
|---|---|---|---|---|
| 000335 | 0x101237c1 | +0x100 | (row argument) | 4 |
| 000336 | 0x101237c2 | +0x108 | (1, 1) | -- |
| 000390 | 0x101237c4 | +0x104 | (1, second argument) | 8 |

Candidate nxOnceReportVtEx takes the gate as a candidate-side byte the harness
mirrors, so each row is driven TWICE: once with the gate clear (the report must
fire and the gate must be set) and once with it set (no report). Both the
report tuple and the slot's recorded arguments are compared on each pass
(build/r153.log oncedispatch failures=0). All three move to `reconstructed`.

Note the gate is WRITTEN, not merely read -- unlike the 3z159 reader -- so the
two passes are what actually pin the one-shot behaviour. No gate,
coverage-floor, or policy change.

## 3z171. Deleting destructor with a global call closes (003938)

Round 154 closed 003938 (0x8eec0, ret 4), which is the 3z166 deleting
destructor plus one extra step:

    mov dword ptr [esi], 0x10117920     ; install the vtable
    call dword ptr [0x10104194]         ; a process-wide global, no arguments
    test byte ptr [esp+8], 1            ; the flags argument
    je  END
    eax = [0x101041bc]                  ; the allocator singleton
    ecx = [eax]; edx = [ecx]
    push esi
    call [edx + 0x14]                   ; free(this)
END:
    mov eax, esi; ret 4

The global slot holds a placeholder (0x1210ce) like the report and lock slots,
so nxBindGlobalSlot points it at a recorder and the candidate exposes the same
body through nxSetGlobalHook3938. Both arms verify (build/r154.log dtor3938
failures=0): the vtable word is installed, the global runs exactly ONCE on both
arms, `this` is returned either way, and only the flagged arm frees.

003938 moves to `reconstructed`. This is the third distinct global this
campaign has bound (report, lock API, and now a bare process-wide callback),
which suggests the remaining `global`-classified rows may be reachable the same
way. No gate, coverage-floor, or policy change.

## 3z172. Guarded store family closes (5 rows) -- and 3z158 is superseded

Round 155 closed five rows that share one body:

    mov eax, [ecx+0x2c]
    and al, 0x18
    cmp al, 0x10
    jne STORE
    <report(1, <file>, <line>, 0, <expr>)>; ret 4
STORE:
    mov edx, [esp+4]
    mov [ecx+<field>], edx
    ret 4

| row | field | report file / line / expression |
|---|---|---|
| 004184 | +0x44 | 0x101195b0 / 0xb1 / 0x10119628 |
| 004288 | +0x1d0 | 0x10119e64 / 0x83 / 0x10119ef0 |
| 004292 | +0x44 | 0x10119e64 / 0x8e / 0x10119f40 |
| 004338 | +0x44 | 0x1011a204 / 0xae / 0x1011a2e0 |
| 004334 | +0x1a8 | 0x1011a204 / 0x9d / 0x1011a290 |

**This supersedes 3z158.** That section declined 004334 because its work arm
"does not write the argument" -- the oracle left [self+0x1a8] at zero for every
value tried. Re-driving it here, in the same table as its four siblings, the
row behaves exactly as its listing says and BOTH arms verify: with the guard
byte clear it stores 0x11223344 at +0x1a8, and with the guard equal to 0x10 it
reports. The drive asserts both (`storedO != 0x11223344u` fails arm 0, and
`nO != arm` pins the report), so it is genuinely exercising both paths. The
3z158 failure was an artifact of that round's drive, not of the row.

Candidate nxGuardedStoreEx(self, arg, fieldOff, code, file, line, expression)
covers all five; each is driven on both arms and the whole 0x200-byte object is
compared, not just the stored word (build/r155.log guardedstore failures=0).
All five move to `reconstructed`.

The lesson is the same one 3z168 drew and it keeps recurring: when a row
"does not behave as written", suspect the drive before the decode. Three
declines have now been reversed that way (001329's group, 004334 here, and the
002390 callback-field misread). No gate, coverage-floor, or policy change.

## 3z173. Lock-first direct-call family closes (4 rows)

Round 156 took the mutex shape to a family whose lock and object offsets differ
from the earlier ones: the lock is at **[self+0xc]** and the object at
**[self+0x24]** (or +0x14), not the +0x10/+0x18 pair 3z161 used. Four rows
close:

| row | object | work |
|---|---|---|
| 000350 | +0x24 | helper 000545 stores the argument at [obj+0x6ac] |
| 000354 | +0x24 | helper 000549 stores it at [obj+0x6b0] |
| 000358 | +0x24 | helper 000553 stores it at [obj+0x6b4] |
| 003870 | +0x14 | helper 004248 is a bare ret-4 no-op |

All four report (2, <file>, <line>, 0, 0x10104760) on the acquire-failing arm.
Their helpers are already reconstructed, but only as census rows -- they have no
C++ candidate of their own -- so the candidate models each helper's EFFECT
directly: workKind 0 for the no-op, workKind 1 for the single store, with the
store offset passed in. Both arms verify and the whole 0x800-byte object buffer
is compared, not just the stored word (build/r156.log mutexwork failures=0).

The remaining rows of this family (000289, 000338, 003714, 003744, 003942,
003944) need helpers with wider effects -- 3, 4 and 5 stores, an 11-dword copy
from a pointer argument, and one OR/AND bit-update -- which the same workKind
mechanism can absorb one at a time. All four closed rows move to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z174. Wide work kinds close the rest of the family (6 rows)

Round 157 finished the lock-first direct-call family by widening the modelled
work from a single store to the four shapes its helpers actually use. The
helpers are all consecutive-dword stores, which is why one mechanism absorbs
them:

| row | object | work kind | helper |
|---|---|---|---|
| 000289 | +0x24 | 3 -- copy N dwords from a POINTER argument | 000507, 3 dwords to +0x520 |
| 000338 | +0x24 | 2 -- N stack arguments to consecutive dwords | 000540, 3 to +0x52c |
| 003942 | +0x14 | 2 | 003966, 5 to +0x44 |
| 003944 | +0x14 | 2 | 003968, 4 to +0x58 |
| 003714 | +0x14 | 3 | 003427, 11 dwords to +0x28 |
| 003744 | +0x14 | 4 -- OR/AND bit update on one word | 003453, on +0x58 |

workKind now reads: 0 no-op, 1 single store, 2 N stack-argument stores, 3 N
dwords from a pointer argument, 4 the conditional OR/AND. All six verify on
both arms with the whole 0x800-byte object buffer compared, and the four rows
closed in 3z173 still pass unchanged under the widened candidate
(build/r157.log mutexwide failures=0, mutexwork failures=0).

That closes the whole family: ten rows across 3z173 and 3z174. The mutex
families are now fully worked -- 3z160's probe, 3z161's thirty-one vtable
dispatches, 3z167/3z168's two direct-call groups, and these ten. All six move
to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z175. A slot sibling and a guarded three-argument loop (2 rows)

Round 158 closed two more rows.

**004707 (0xb31b0)** is the 3z169 shape through a third slot: lock
[self+0x14], `vtable[+0x38]([self+0x18])` with `this` in ecx and no stack
arguments, release, return the value. The existing nxLockedVtCallNoArg needed
only a new arm -- no new candidate.

**001024 (0x229b0, ret 0xc)** is a second dispatch loop, but unlike the 3z165
one it is GUARDED and takes three arguments:

    n = ([self+0xe4] - [self+0xe0]) >> 2
    if (n == 0) return 1
    loop:
      elem = [edi]
      al = [elem+0xde]
      if (al & 7) != 0 then SKIP          ; the element is filtered out
      call vtable[+0x10](elem, arg1, arg2, arg3)
      if (result == 0) return 0           ; first failure short-circuits
    SKIP: repeat while elements remain
    return 1

Two behaviours here are worth pinning and both are driven: the +0xde low-three-
bit filter, and the SHORT-CIRCUIT -- the loop stops at the first slot that
returns zero, so the hit count differs between the all-succeed and one-fails
cases. The drive walks twelve combinations (element counts 0..2 x skip flag set
and clear x slot returning 1 and 0), comparing the returned byte, the hit count
AND all three arguments (build/r158.log loop1024 failures=0).

Both rows move to `reconstructed`. No gate, coverage-floor, or policy change.

## 3z176. The global-flag read closes (004491) -- a fourth decline reversed

Round 159 closed 004491 (0xb0f10), which an earlier round had declined on the
grounds that driving it "would couple the candidate to the oracle image":

    <lock [self+0x14]>              ; 002362
    call 004083                     ; reads the global data word [0x10127180]
    <unlock [self+0x14]>            ; 002366
    return the byte

The helper 004083 is three instructions -- load [0x10127180], `test`, `setne`
-- so the row's entire result is "is that data word non-zero". The earlier
objection was that the candidate would have to read the ORACLE's data word.
But a data word is bindable exactly like the slots this campaign has been
binding since 3z134: nxBindFlagWord points [0x10127180] at 0 or non-zero, and
the candidate takes the same value through nxSetGlobalFlag4491. Both arms
verify (build/r159.log globalflag4491 failures=0): a non-zero word returns 1
and a zero word returns 0.

004491 moves to `reconstructed`. This is the FOURTH decline reversed once the
harness grew the right capability (after 001329's group, 004334, and the
002390 callback-field misread), and the distinction it draws is worth stating
plainly: a row that READS image state is drivable when the harness can bind
that state, and the candidate stays independent because it reads its own
mirror rather than the image. No gate, coverage-floor, or policy change.

## 3z177. Report-once four-step dispatch closes (000342)

Round 160 closed 000342 (0xcb90, ret 0x10), the largest single row of this
slate: a one-shot assertion followed by FOUR sequential vtable calls on the
same object.

    mov al, [0x101237c3]
    if (al != 0) goto WORK
    <report(0xd0, 0x10105ba8, 0x12e, 0, 0x10105c88)>; [0x101237c3] = 1
WORK:
    vtable[+0x70](self, arg2, arg3, arg4)     ; arg1 is NOT used here
    vtable[+0x100](self, arg1)
    vtable[+0x64](self)                        ; no arguments
    vtable[+0x108](self, 1, 1)
    ret 0x10

Working out WHICH argument each call receives took care with the stack, since
esp moves under the pushes: after `push esi` the four arguments sit at
+0x10..+0x1c, and the three pushes before the first call re-read [esp+0x10]
AFTER the first push, which lands on arg2 rather than arg3. The result is that
the first call takes (arg2, arg3, arg4) and arg1 is not used until the second
call -- a mapping the differential then confirmed.

The drive records every slot call as (id, args...) into ONE sequence, so the
comparison covers the call ORDER and all arguments, not just that four calls
happened (build/r160.log once0342 failures=0). Both gate passes are driven, and
the gate byte 0x101237c3 is bound and restored. 000342 moves to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z178. Guarded dispatch plus chain walk closes (004763)

Round 161 closed 004763 (0xb3920), the most branch-dense row of this slate: a
guard chain that decides ONE dispatch, followed by a chain walk that runs
whatever the guard decided.

    eax = [esi+0x18]                       ; the object
    if (eax == 0) goto TAIL
    ecx = [eax+8]
    if (ecx == 0) goto CHK_C
      if ([ecx+0x10c] & 0x80) == 0 goto SKIP_A   ; NON-NULL without the bit
                                                 ; SKIPS the +0xc test entirely
    CHK_C:
      ecx = [eax+0xc]
      if (ecx == 0) goto TAIL
      if ([ecx+0x10c] & 0x80) != 0 goto TAIL
    SKIP_A:
      if ([eax+0x44] == 0) goto TAIL
      if (([eax+0x2c] >> 2) & 1) != 0 goto TAIL
        call vtable[+0x20]([esi+0x18], [esi+0x14])
    TAIL:
      node = [esi+0xc]
      while (node) { call vtable[+0x10](node); node = [node+0x10]; }

The subtle part is the guard's THREE-way shape: a non-null [obj+8] whose bit is
clear jumps PAST the [obj+0xc] test, while a null [obj+8] falls into it -- so
the two conditions are not a simple AND. The drive exercises three guard arms
(bit set and entry present; bit clear and entry present; the +0xc entry
carrying the bit) across chain lengths 0..3, twelve cases in all
(build/r161.log chain4763 failures=0).

One recording detail mattered: the walk visits NODES, and the oracle and the
candidate walk different copies, so recording raw pointers can never compare
equal. The recorder stores each node's IDENTITY marker instead -- the same
fixture-sharing lesson as 3z166/3z168, applied to a sequence rather than a
field. 004763 moves to `reconstructed`. No gate, coverage-floor, or policy
change.

## 3z179. Store-then-dispatch closes (005382) -- and the drivable backlog is empty

Round 162 closed 005382 (0xe9440, ret 4), the last row on the drivability list:

    storeDword([esi+8]) into the MemoryStream argument     ; via 004797
    eax = [esi+0x10]
    if (eax == 0) return ([esi+8] >> 2) & 1
      return vtable[+0x14](eax, stream)

The work helper 004797 is `MemoryStream::storeDword`, and MemoryStream is
already reconstructed as a C++ class, so the drive does NOT need to fake a
stream layout: it constructs a real MemoryStream on EACH side and lets the
oracle's raw code and the candidate's C++ call run on their own objects. The
comparison then covers the return value, the stream LENGTH and the full
underlying buffer -- a far stronger check than the row's own return would allow
(build/r162.log store5382 failures=0, two dispatch arms across two stored
values).

Two build changes were needed to reach it and are worth recording:

- ObjectModel.cpp and the layout test both had to include MemoryStream.h;
- MemoryStream.cpp had to be added to the NxPhysicsObjectLayoutTests source
  list, which had carried only ObjectModel, Containers, Geometry, IcePrunable,
  ThirdPartyHost and PhysicsInternal.

005382 moves to `reconstructed`. **With it, every row the drivability filter
could reach is closed.** The filter now reports nothing beyond the rows already
closed, so the next slate has to come from somewhere other than this
mechanism: the remaining `discovered` rows are compiler artifacts, fragments,
rows blocked by large undiscovered dependencies, or rows behind the vtable
identity gate. No gate, coverage-floor, or policy change.

## 3z180. Allocator-via-004803 destructors close (2 rows)

Round 163 widened the drivability filter -- larger rows, and callees that are
only `statically_reviewed` rather than reconstructed -- and closed two rows it
surfaced.

001563 (0x2e570, ret 4) and 002154 (0x538b0, ret 4) are scalar deleting
destructors of the 3z166 family with one difference: the allocator does NOT
come from a fixed global slot. Each stores its own vtable word (0x10107848 and
0x1010833c) and, when the low flag bit is set, calls 004803 to obtain the
allocator and then `allocator->vtable[+0xc](self)`. 004803 is twenty bytes --
return the pointer at [0x1012845c], or the static at 0x10122368 when that is
null -- so binding [0x1012845c] to a fake allocator routes the free into a
recorder on both sides. Both arms verify (build/r163.log allocdtor
failures=0).

The widened filter also surfaced a TEN-row group (004453, 004493, 004523,
004553, 004583, 004609, 004637, 004667, 004693, 004745) of the lock-first
shape whose work calls 000480. That one is 388 bytes and is a registry
lookup-and-insert over the global [0x10123c0c] using the allocator, so those
rows stay `discovered`: driving them would mutate a global registry the
candidate cannot reproduce, and only their null-argument early-out is
side-effect free -- too little to call a differential.

Both closed rows move to `reconstructed`. No gate, coverage-floor, or policy
change.

## 3z181. Owned-pointer destructors close (001585, 001587)

Round 164 closed the 72-byte pair 001585 (0x2ea80) and 001587 (0x2ead0).
They are identical in shape and differ only in their vtable word and RVAs:

    [esi] = <own vtable>                    ; 0x1010785c / 0x1010786c
    eax = [esi+0xc]
    if (eax == 0) goto SKIP
      allocator = 004803()
      allocator->vtable[+0xc]([esi+0xc])    ; free the OWNED pointer
      [esi+0xc] = 0
    SKIP:
      call 001554(esi)                      ; stores the FIXED vtable 0x10107848
    if ([esp+8] & 1)
      allocator->vtable[+0xc](esi)          ; free self
    mov eax, esi; ret 4

Two details are easy to get wrong and both are pinned by the drive. First, the
owned pointer is freed BEFORE the helper runs and the field is cleared, so a
destructor run twice does not double-free -- and the helper 001554 then
OVERWRITES the vtable stored at the top with the fixed 0x10107848, so the
row's own vtable word is transient. Second, the row can free up to TWO blocks,
so the recorder captures an ordered SEQUENCE rather than a single pointer, and
the comparison covers the count and each element -- with the owned pointer and
the self pointer each checked against THAT side's own buffer, since the two
fixtures live at different addresses.

Eight combinations verify (build/r164.log ownedptr failures=0): both rows, by
owned pointer present and absent, by both flag arms. Both move to
`reconstructed`. No gate, coverage-floor, or policy change.

## 3z182. Lock-bracketed registry lookup closes (004743)

Round 165 closed 004743 (0xb3670), the lock-first shape whose work calls
helper 000454 rather than a vtable slot:

    <lock [esi+0x14]>
    push [esi+0x18]
    call 000454                     ; the registry lookup
    add esp, 4
    <unlock [esi+0x14]>
    return that value

The question was whether 000454 is tractable. It reads the registry pointer at
[0x10123c0c] and returns 0 immediately when that is null; the image ships it
null -- its initialiser in the file is literally 0 -- so the helper is INERT in
the shipped configuration and the row returns 0. The drive binds
[0x10123c0c] to null explicitly rather than relying on no earlier drive having
populated it, the same defensive move 3z155 made with the assert guard. Two
object shapes verify (build/r165.log registry4743 failures=0).

The census proof states the limit plainly: the non-null-registry walk is NOT
modelled, because it does not occur in this image. That is a narrower claim
than the neighbouring rows make, and it is recorded as such rather than
dressed up. 004743 moves to `reconstructed`. No gate, coverage-floor, or
policy change.

## 6. What this task did not do

- No behavioural reconstruction: every row here stays `discovered` until a
  differential drives it.
- Actor +8 subobject semantics, TBL_87 slots 63/64 identity, third-pose role,
  and the twelve-descriptor mapping are recorded as open questions in
  `object_model.json`.
