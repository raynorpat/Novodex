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

## 6. What this task did not do

- No behavioural reconstruction: every row here stays `discovered` until a
  differential drives it.
- Actor +8 subobject semantics, TBL_87 slots 63/64 identity, third-pose role,
  and the twelve-descriptor mapping are recorded as open questions in
  `object_model.json`.
