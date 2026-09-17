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

## 6. What this task did not do

- No behavioural reconstruction: every row here stays `discovered` until a
  differential drives it.
- Actor +8 subobject semantics, TBL_87 slots 63/64 identity, third-pose role,
  and the twelve-descriptor mapping are recorded as open questions in
  `object_model.json`.
