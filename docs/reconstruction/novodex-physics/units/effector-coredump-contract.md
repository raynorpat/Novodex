# Effector and core dump: contract

Recovered by Task 1 of `docs/superpowers/plans/2026-09-28-effector-and-coredump.md` from the
Capstone listing (authoritative), with the Ghidra manifest decompiles and the pinned
supplement as a cross-check, the .rdata tables and strings read from the image
(`D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, base 0x10000000), and the candidate
sources at 25acaf2. Bundles: `units/gap__fluids__NpImplicitMesh.cpp__to__NpSpringAndDamperEffector.cpp.md`
(effector rows 003922-003938 only), `units/NpSpringAndDamperEffector.cpp.md`,
`units/gap__NpSpringAndDamperEffector.cpp__to__Joint.cpp.md`.

Supplement: `DecompileSupplement.java` was run once (`-readOnly -noanalysis`, Ghidra 12.1.2,
toolchain pin verified) with the union of the 34 existing requests and 9 new rows that had only
a Capstone body: 0x8ed50 (003924), 0x8edb0 (003928), 0x8f100 (003954), 0x8fc00 (003983),
0x8fc50 (003985), 0x8fcb0 (003987), 0x8fd00 (003989), 0x91940 (004025), 0x91de0 (004031).
All 43 are `ok`; the 34 existing entries are byte-identical. The continuation rows
004039/004041/004043 (of 004037) and 004053-004061 (of 004051) were not requested: a function
created at a mid-body address decompiles the tail out of context, and the parents' manifest
decompiles already contain them.

"Model" below means a row that is `reconstructed` in the inventory only through a
parameterised shape model or a direct-drive fixture (the `static_proof`/`dynamic_proof` name
the drive: lockacc8, quadbatch, tmplinit2, ...), with no product code on any public path.
Every model row in these ranges is of that kind; none of them has a product implementation.
Per the Global Constraints the models stay; the product rows are new code.

## Effector

### Row assignment (30 rows, 3,325 B in the three bundles)

| Rows | B | Class / role | State today |
|---|---:|---|---|
| 003934 ctor, 003936 dtor body, 003938 deleting dtor | 88 | `Effector` base (derives `NxFoundation::Observable`); table 0x10117920 | models |
| 003922 ctor, 003926 setBodyRecords, 003928 event, 003924 slot 2 apply-thunk, 003930 dtor body, 003932 deleting dtor | 299 | `ActorPairEffector` (two observed body records); table 0x101178f8 | 003924/003928 models, rest discovered |
| 003960 ctor, 003962 setBodies, 003966 setLinearSpring, 003968 setLinearDamper, 003970 spring force, 003972 damper force, 003974 getLinearSpring, 003975 getLinearDamper, 003977 deleting dtor, 003979 slot 3 solver apply, 003964 dump reader | 2,358 | `SpringAndDamperEffector` (internal, 0x68 B); table 0x101179e4 | 003966/003968/003974/003975 models, rest discovered |
| 003940 setBodies, 003942 setLinearSpring, 003944 setLinearDamper (NpSpringAndDamperEffector.cpp) | 324 | Np wrapper setters | 003942/003944 models |
| 003946 getLinearSpring, 003948 getLinearDamper, 003950 isSpringAndDamperEffector (x3 slots), 003952 getInternal, 003954 member thunk, 003956 deleting dtor, 003958 ctor | 256 | Np wrapper, rest | 003946-003952 models |

003981 (254 B) sits in this gap but is not effector code: it is the `isValid` slot of the
descriptor table at 0x10117a44 (`[004025, 003985, 003981]`), a public-header inline the dump
TU emits (see `## Core dump`, generated rows). It goes with the dump.

Owner files: `Physics/src/core/SpringAndDamperEffector.cpp` (internal rows 003922-003938 and
003960-003979 with 003964) and `Physics/src/core/NpSpringAndDamperEffector.cpp` (003940-003958).
The image's `__FILE__` for the Np setters is `\Epic\Novodex\SDKs\Physics\src\NpSpringAndDamperEffector.cpp`
(lines 0x1b, 0x22, 0x29); the internal rows carry no string. Neither file does x87-sensitive
work except 003962/003964/003970/003972/003979, so the internal file goes on the
`/arch:IA32` list; the Np file does not.

### Layouts

Np wrapper `NpSpringAndDamperEffector`, 0x18 B (003960 allocates it through
`nxFoundationSDKAllocator` slot +8 with `(0x18, 0)`; 003958 constructs it):

| Off | Field | Evidence |
|---|---|---|
| +0x00 | primary vptr 0x1011794c | 003958 last store; 003956 first store |
| +0x04 | `NxEffector::userData` | public header |
| +0x08 | hook member vptr 0x10117948 (the 3-word `EmbeddedHookBase` of the joints: 002404 stores its vptr and zeroes +0xc/+0x10, 002406 restores 0x101088b8) | 003958, 003956 |
| +0x0c | hook word 1 = scene write-lock link (NpScene+0xc) | 002404 (0), then 000301 at 0xc68f; setters tryLock it (002364) |
| +0x10 | hook word 2 = scene read-lock link (NpScene+0x10) | 002404 (0), then 000301 at 0xc689; getters lock it (002362) |
| +0x14 | internal `SpringAndDamperEffector*` | 003958; 003952 returns it |

Note the lock links are one word lower than the joints' (+0x10/+0x14): the hook is at +8 here
because `NxEffector` has no vptr-adjacent `appData`. Pass the link by value to the
`nxNpSceneGuard*` helpers as the joints do.

Primary table 0x1011794c, 8 slots: [0] 003950 isSpringAndDamperEffector (read lock/unlock,
return `this`), [1] 003940 setBodies, [2] 003942 setLinearSpring, [3] 003946 getLinearSpring,
[4] 003944 setLinearDamper, [5] 003948 getLinearDamper, [6] 003950, [7] 003950. The public
headers declare six virtuals; slots 6 and 7 are two more Np-level virtuals folded onto the same
body (the header cannot change, so the Np class declares two extra virtuals returning `this`
under the read lock, after the interface's). Member table 0x10117948 has one slot, 003954
(`sub ecx,8; jmp 003956`). 0x101179ac is the abstract intermediate (six `_purecall`) that
003958 stores before the final table; not observable after construction.

Internal `SpringAndDamperEffector`, 0x68 B (000587 allocates `(0x68, 0)` through
`nxFoundationSDKAllocator` slot +8):

| Off | Field | Written by |
|---|---|---|
| +0x00 | vptr: 0x10117920 (Effector) -> 0x101178f8 (ActorPairEffector) -> 0x101179e4 | 003934, 003922, 003960 |
| +0x00..+0x13 | `NxFoundation::Observable` base, sizeof 0x14: vptr at +0, observer array {first, last, memEnd} at +0x4..+0xc (zeroed by the Observable ctor, import 0x10104190), the array's allocator word at +0x10; destroyed through 0x10104194 | 003934, 003936 |
| +0x14 | pad word: no effector row writes it (name it explicitly, e.g. `mPad14`) | -- |
| +0x18 | next effector in the Scene list (+0x5a4) | 003934 (0), 000587, 000573, 000575 |
| +0x1c | owning Scene | 003934 (ctor argument) |
| +0x20 | Np wrapper | 003960 (null if its allocation failed) |
| +0x24 / +0x28 | observed body records (`actor(0x50)+8`), null = world | 003922 (0), 003926 |
| +0x2c..+0x34 | anchor 1 in body-1 frame (world when body 1 is null) | 003962 |
| +0x38..+0x40 | anchor 2 in body-2 frame | 003962 |
| +0x44..+0x54 | spring: distCompressSaturate, distRelaxed, distStretchSaturate, maxCompressForce, maxStretchForce | 003966; read 003974 |
| +0x58..+0x64 | damper: velCompressSaturate, velStretchSaturate, maxCompressForce, maxStretchForce | 003968; read 003975 |

003960 zeroes +0x2c..+0x64 in the listing's order (+0x44..+0x64 first, then +0x34, +0x30,
+0x2c, +0x40, +0x3c, +0x38), then allocates and constructs the Np wrapper.

Internal tables: 0x10117920 `[004387 Observable::event trampoline, 003938, _purecall]`;
0x101178f8 `[003928, 003932, 003924, _purecall]`; 0x101179e4 `[003928, 003977, 003924, 003979]`.
Slot 0 is the `Observable::event(NxU32, Observable&)` override: 003928 nulls +0x24 (if it is the
sender) else +0x28 when the event is 0x100 (a body record going away). Slot 1 is the deleting
destructor. Slot 2 (003924) is the per-tick entry: it calls slot 3 with (+0x24, +0x28).

### Scene and NpScene rows

| Row | B | What | State |
|---|---:|---|---|
| 000301 (0xc630) | 140 | NpScene slot 7 createSpringAndDamperEffector: tryLock +0xc (report line 0x87), 000587, then Np+0x10 = NpScene+0x10, Np+0xc = NpScene+0xc, return Np; if the Np is null, 000594 on the internal and return 0 | discovered |
| 000303 (0xc6c0) | 92 | slot 8 releaseEffector: tryLock (line 0x9a), 003952 on the argument, 000594 | discovered |
| 000327 (0xc9a0) | 36 | slot 22 getNbEffectors: read lock, 000561 | model |
| 000329 (0xc9d0) | 31 | slot 23 resetEffectorIterator: read lock, 000565, tail-jump unlock | model |
| 000331 (0xc9f0) | 55 | slot 24 getNextEffector: read lock, 000569, return `[internal+0x20]` or 0 | model |
| 000561 | 7 | `[+0x6c4]` effector count | model |
| 000565 | 13 | `[+0x6c0] = [+0x5a4]` | model |
| 000569 | 23 | pop `[+0x6c0]`, advance through +0x18 | model |
| 000573 (0x10900) | 109 | removeEffector: unlink from +0x5a4 via +0x18; not found -> error 2, Scene.cpp line 0x84c, "Scene::removeEffector: effector is not in the scene." | discovered |
| 000575 (0x10970) | 68 | release all effectors (slot 1 with 1 on each), called by the Scene destructor 000663 at 0x13f90, after 000596 (actors) and before 002320 / 000604 (joints) | discovered |
| 000587 (0x10c90) | 187 | createSpringAndDamperEffector: allocate 0x68, 003960(scene), push at the head of +0x5a4, `++[+0x6c4]`, `[+0x6c0] = [+0x5a4]`, then 003962(desc.body1 ? [body1+0x14] : 0, &desc.pos1, desc.body2 ? [body2+0x14] : 0, &desc.pos2), 003966(desc +0x20..+0x30), 003968(desc +0x34..+0x40); returns the internal | discovered |
| 000594 (0x10e80) | 126 | releaseEffector: the global user-callback re-entry guard .data 0x10123c10 (shared by about 50 rows, createJoint among them; report line 0x4ec with the message pointer at 0x10122050), 000573, slot 1 with 1, `--[+0x6c4]`, `[+0x6c0] = [+0x5a4]` | discovered |

Scene fields: +0x5a4 list head, +0x6c0 enumeration cursor, +0x6c4 count. The candidate
constructor already zeroes all three (Scene.cpp, from 000647 0x12e59/0x12f03/0x12f09). The
count is incremented even when the allocation fails (0x10ced jumps back to 0x10cc5), after
which 003962 runs on a null `this`; a faithful 000587 keeps that.

Desc layout used (public `NxSpringAndDamperEffectorDesc`): body1 +0, body2 +4, pos1 +8,
pos2 +0x14, the five spring words +0x20..+0x30, the four damper words +0x34..+0x40.
`isValid()` is never called (the desc has no vtable and 000587 reads it raw).

### Body records must be Observables (prerequisite, not in the candidate)

003926 calls `Observable::addObserver`/`removeObserver` (imports 0x1010415c/0x10104158) on the
two body records. In the oracle the dynamic body record is an Observable: its constructor
000797 calls the Observable constructor (0x1001b60e) and stores the record table 0x10106890,
whose only slot is the Observable::event trampoline 004387; its destructor 000776 calls
`~Observable` (0x100185d6). The candidate record (`nxActorComputeMass`, Scene.cpp) is
`memset` to zero, so +0x00 is a null vptr. `removeObserver` calls `event(OE_LAST_OBSERVER_REMOVED)`
virtually on the record when its observer count reaches zero -- on the first effector release
the candidate would call through a null vptr. Task 2 must construct the record's Observable
part (placement of `NxFoundation::Observable` at +0 plus the 0x10106890-equivalent table, and
`~Observable` in the record teardown) before any effector can be released. The Observable
occupies +0x00..+0x13 (sizeof 0x14); +0x14 is a pad word no row writes (name it explicitly);
the record's pose sub-object starts at +0x18 (000801 on `record+0x18`).

Who sends event 0x100 to the effector: `Observable::notifyObservers` (import 0x101041ac;
0x10104160 is `NxGetBoxTriangles`, not an Observable import) is called with 0x100 from two
sites only:
- 000030 at 0x1d82 (the internal actor teardown, on `[actor+8]`, the record, just before the
  record's destructor 000776 runs); 000030 is called by 000596 (the Scene destructor's actor
  loop) and by 000626/000628 (releaseActor);
- 000122 at 0x3af6 (NpActor.cpp).
So in the oracle, releasing an actor, or the scene, calls 003928 on every effector observing
that record, which nulls its +0x24/+0x28 before 000575 runs (000663 calls 000596 before 000575).

Controller decision (binding for Task 2): reproduce this. Add `notifyObservers(0x100)` on the
record in the candidate's record teardown, in 000030's order (before the record's destructor
work), on both reachable paths: `NxSceneInternal::releaseActor` (Scene.cpp, near line 1331)
and the actor loop of `nxSceneDelete`; also 000122's site if it is on a path the candidate
reaches. Wire 000575 into `nxSceneDelete` at the oracle's position: after the actor loop
(000596) and before 002320 / 000604 (the joint lists). What is reproduced is the behaviour of
those sites of 000030 and 000122 (Phase 5 rows); the rows themselves stay `discovered` unless
written whole.

### Solver slots

003979 (1,019 B, slot 3) is reached only from the Scene's pre-tick loop 000655 (0x13968:
for each effector in +0x5a4, slot 2 = 003924, which calls slot 3 with the two records);
000655 is called only by the simulate row 000659. Step-only: no public call reaches it in the
candidate. It reads the records' +0x134..+0x154 (orientation), +0x158..+0x160 (world centre of
mass), +0x34..+0x48 (velocities), `[this+0x1c]+0x548` (the Scene step size), calls 003972
(damper) and 003970 (spring) -- both return on the x87 stack -- then, per non-null body,
000713 (the root of the record chain through +0x1e8, a model today, 32 B) and, when the root's
+0x1f8 is non-zero, 000791 (addForceAtPos, 133 B, Phase 2, discovered) which calls 000782
(3,428 B, Phase 2, discovered). With both bodies null it has no side effect. So the only
observable arm needs 000791/000782: write 003979, 003970, 003972 and 000713 faithfully and call
000791 through a deferred asserting stub (as `Row000758Fixture`), or drive slot 3 through the
internal vtable with a root whose +0x1f8 is zero and report static proof only.

### Dependency closure

Written in the candidate and reused: 002362/002364/002366 (the recursive scene lock), 002404/002406
(hook member), 000448/000450, the Foundation Observable exports, `nxFoundationSDKAllocator`.
New with the effector: the 30 rows above, the 12 Scene/NpScene rows, the body-record Observable
(000797/000776 parts), the `notifyObservers(0x100)` sites of 000030/000122 in the candidate's
record teardown (behaviour only), 000575 in the candidate's `nxSceneDelete` (after the actor
loop 000596, before 002320/000604, as 000663 orders them), 000713, and a stub for 000791. 004387 is a model; the
product uses `NxFoundation::Observable::event` itself (the trampoline is the import thunk).

### Task 2 record

Written (Task 2): the 30 effector rows in `Physics/src/core/SpringAndDamperEffector.cpp`
(003922-003938, 003960-003979, 003964 included) and `Physics/src/core/NpSpringAndDamperEffector.cpp`
(003940-003958); the Scene rows 000561 000565 000569 000573 000575 000587 000594 as
`NxSceneInternal` members in `Physics/src/Scene.cpp`; the NpScene slots 000301 000303 000327
000329 000331 in `Physics/src/NpScene.cpp`; 000713 as `Row000713Fixture` in
`Physics/src/core/JointSupport.cpp`. 003932, 003938 and 003954 are the compiler's (the scalar
deleting destructors of the two abstract classes and the hook base's adjustor thunk), carried as
stable-ID lines only. Deferred: 000791, an `NX_ASSERT(0)` stub (`Row000791Fixture`), because it
calls 000782 (3,428 B, Phase 2).

Wiring and the body record:
- The dynamic body record gets its Observable part: `nxActorComputeMass` placement-constructs
  `NxBodyRecordObservable` (an `NxFoundation::Observable` with no overrides, so its one slot is
  `Observable::event`, as 0x10106890's is) at +0 right after the record's `memset`. Nothing moves
  (+0x14 stays a pad word, the pose at +0x18) and nothing is allocated.
- `NxSceneInternal::releaseActor` (the path of 000626/000628 and of `nxSceneDelete`'s actor loop)
  calls `notifyObservers(0x100)` on the record before the record id is recycled, then
  `~Observable` before the record is freed (000030's and 000776's order). 000122's site is on
  NpActor slot 18, `setDynamic`, which the candidate does not implement; it is not reachable.
- `nxSceneDelete` calls `releaseEffectors` (000575) after the actor loop, before the joint lists.
- 000573, 000575 and the rows the listing calls as functions (003922 003926 003930 003934 003936
  003970 003972) are `noinline`, so the cdb trace sees them run.

Found by the transcript and fixed: `releaseActor` recycled a shape's id after freeing the shape;
the oracle recycles it first (the scene release's free order `1c,8,228`, the 8 being the id
vector's old block when the recycle grows it). Not visible to any earlier registered line.

Differences measured and left (outside this task):
- The oracle leaves the record's +0x14 pad as allocated (0xcdcdcdcd under the fill allocator);
  the candidate's record `memset` zeroes it.

Review fix (the +0x1f8 difference): the candidate's record now builds 000797's island.
`nxActorComputeMass` calls 000760 (written) and then 000722 (written by this task in
`core/JointSupport.cpp`: the island's largest +0x4c on the root's +0x1cc, the copy of
+0x1bc..+0x1d4 to +0x1e8..+0x200, +0x208 and +0x25c zeroed) where it used to store only
+0x1bc/+0x1e8, before the descriptor's wake counter is stored at +0x4c, as 000797 calls both
before 000793. The root's +0x1f8 is now 0x3ecccccc on both sides; the transcript prints the
island words and registers them. Every Phase 5 and joint staged-pair target stays identical.

Recorded, not changed: `releaseActor`'s compound-shape branch (shape type 5) still frees each
sub-shape before recycling its id, the order the non-compound branch had before the fix above.
The oracle's order there is not measured: no registered target creates a multi-shape actor (the
candidate's group builder is a hole) and the compound shape's deleting destructor is not
reconstructed.

Test: `NxPhysicsEffectorTests` (`tests/PhysicsEffectorTests.cpp`), a staged-pair target on the
Phase 6 and 7 lists, with the page-guarded fill allocator. 79 lines registered from the oracle
side; floors 6/7 = 482/355. dynamic_proof from `evidence/effector-and-coredump-trace-effector.txt`.

## Core dump

### Call chain

`NxPhysicsSDK::coreDump(fname, binary, addendum)` is NpPhysicsSDK slot 000267 (0xbf20,
221 B, NpPhysicsSDK.cpp): for each scene i < 000448(), tryLock `000450(i)->[+0x6cc]->[+0xc]`
(002364). If one fails, unlock the ones taken in reverse, report code 2 at NpPhysicsSDK.cpp
line 0xe1 with the deadlock string and return false. Otherwise call
`PhysicsSDK::coreDump` = 004062 (`this` = the PhysicsSDK, `ret 0xc`), keep its `al`, unlock
every scene in order, return it.

004062 (0x950f0, 1,699 B) always returns false (`xor al, al` at 0x95787 on both the success
and the fopen-failure path). The current candidate stub already returns false; it writes
nothing. The whole chain:

```
000267 NpPhysicsSDK::coreDump
  004062 PhysicsSDK core dump (file, header, materials, per-scene constants, trailer, addendum)
    003992 date/time (tzset, time, localtime, strftime x2)
    003995 float token (ring of 16 x 64-byte buffers)
    004051 per-scene asset writer (+ continuations 004053 004055 004057 004059 004061)
      000509 gravity; 000563/000567 joint iterator
      004037 joint block (+ 004039 004041 004043) -> 004068 004070 004072 004004 004007
             004009 004011 004013 004081 004083 004145, joint internal slot 10 (saveToDesc)
      NxActor slot +0x148 saveToDesc, +0x144 saveBodyToDesc; 004006 actor name (000086);
      000015/000017 shapes; 003997/003999 settings records
      004048 shape writer -> shape internal slot 13 (saveToDesc), 001283 type,
             004017 trigger flags, 004046 mesh writer (003991 004035 001472, mesh vtables)
      004015 PsJoint line -> 004068 004006 004004
      000523/000525 pair flags -> 004006, 001281
      003974/003975/003964 effector readers -> 004006
```

### File handling

- Name: `sprintf(buf512, "%s.psc", fname)` (0x95126); no length check.
- Open: 005671 = the static CRT `fopen` (`_fsopen(name, mode, _SH_DENYNO 0x40)`) with mode
  `"wb"` (.rdata 0x101061bc). Binary mode: every line ending in the file is written literally,
  mostly `\r\n`, but the effector lines end in `\n` only.
- Write: 005716 `fprintf` for every line, 005739 `sprintf` for name and number tokens,
  005732 `strstr(buf, ".")` in the float token. No fputs, no fwrite.
- Close: 005673 `fclose` at 0x9576f, after the addendum, on the success path only (a failed
  open skips everything).
- Scratch: 004062 calls `operator new(0x20000)` (005701) first and `free` (005668) last, on
  both paths. It is CRT heap, not the SDK allocator. It is the mesh-name table 003991 fills
  (0x8000 pointers), not an unused block; see "### Task 3a record".
- 004051 allocates its pair arrays with `operator new` and frees them with `free` (005700/005668).

### Text vs binary

There is no binary record format. `binary` selects the float token 003995 uses and nothing else:
- both modes: `3.4028235e38` (FLT_MAX exactly) -> `fltmax`, `0.0` -> `0`, `1.0` -> `1`, `-1.0` -> `-1`;
- text (`binary == false`): `sprintf("%.9f", (double)v)`, then, if the text contains `.`,
  strip trailing `0`s and a trailing `.`;
- binary: `sprintf("%.4f$%x", (double)v, bits(v))`, the float's raw 32-bit pattern in hex.
The token buffer is a ring of 16 x 64 bytes at .data 0x10126978 with the index at 0x10126d78
(so one fprintf may hold at most 16 tokens). Some lines bypass 003995: the asset header's
elapsed-time lines always pass `binary = false`, and the effector lines use `%f` with a
promoted `double` in both modes. `%d`/`%08X` integers are printed directly; three setting kinds
(material index, solver count, group) round the float with `fistp` at the live control word.

### Record formats (strings read from the image; `T` = one 003995 token)

File header (004062):
```
################################################################\r\n
### Core Dump from Novodex Physics SDK\r\n
### Core Dump Generated on <strftime "%A, %B %d, %Y"> at <strftime "%I:%M %p">\r\n
### Contains one Asset.\r\n            (scene count 1) | ### Contains %d assets.\r\n
PsReset\r\n
PsVersion 1.4\r\n
PsNameSpace PhysicsSDK__<%I64x of the PhysicsSDK pointer>\r\n\r\n
```
Materials: for i in 0 .. (mMaterials count of `this`) - 1, read `PhysicsSDK::instance`'s material
`(i & 0xffff) < count ? i : 0` (the inlined getMaterial): `\r\n### Begin Material Definition ####\r\n`,
`PsMatBegin mat<i+1>`, `PsMatDynamicFriction T`, `PsMatStaticFriction T`, `PsMatSpinFriction T`,
`PsMatRollFriction T`, `PsMatRestitution T`, `PsMatDynamicFrictionV T`, `PsMatStaticFrictionV T`,
`PsMatDirOfAnisotropy T T T`, `PsMatDirOfMotion T T T`, `PsMatSpeedOfMotion T`,
`PsMatAnisotropic true|false` (flags bit 0), `PsMatMovingSurface true|false` (bit 1), `PsMatEnd`,
`### End Material Definition ####\r\n\r\n` -- NxMaterial offsets +0..+0x38 in header order.

Per scene (the PhysicsSDK's scene array, `this`+8/+0xc): `PsAssetBegin Asset__<scene ptr>`,
the 3-line constants banner, 13 lines `PsSetConstant <NAME> T` for NxParameter 0..12 read from
the live parameter array (.data 0x10123b18), a closing banner line plus a blank line; then,
only if some group's mask (.data 0x10123a98, 32 words) is not 0xffffffff, a 3-line banner,
`PsGroupCollisionFlag %d %08X` per such group, and a closing banner; then 004051.

004051 asset block:
```
###########################################################\r\n
#### Asset Information\r\n
###########################################################\r\n
## Scene in initial configuration.\r\n        (Scene+0x544 == 0.0)
   | ## Total Elapsed Time: T / ## Elapsed Time Last Frame: T / ## MaxTimeStep: T /
     ## MaxIter: %d / ## TimeStep = FIXED|VARIABLE      (+0x53c, +0x544, +0x52c, +0x530, +0x534)
###########################################################\r\n
PsGravity T T T                                (000509: Scene+0x520..+0x528)
<004037 block per joint, scene joint iterator order>
<per actor, Scene+0x55c..+0x560 order: settings lines, shape lines, then the actor line>
<004015 "PsJoint <joint> <actor0> <actor1>" per joint>
<"PsActorPair <a> <b> false" per disabled pair, deduplicated for shape pairs>
<per effector in the +0x5a4 list: 5 fixed PsDefaultSettings spring_* lines plus spring_pos1/spring_pos2 when present (5-7 lines), "PsSpring <a> <b>">
PsAssetEnd\r\n
##################################################################################\r\n\r\n
```
Trailer (004062): `\r\n`, then per scene `PsSetScene %d`, `PsAsset Asset__<ptr> position(0,0,0)
orientation(0,0,0,1)`, a blank line; `PsSetScene 0`; with an addendum: `### Begin : User supplied
addendum script.\r\n`, `fprintf("%s\r\n", addendum)`, `### End   : User supplied addendum script.\r\n`.

Settings records (the 0x540-byte object 004062 passes as 004051's `this`): 24 records of 0x1c
bytes {+0 kind 0..23, +4 first, +5 emit, +6 binary, +7 pending, +8..+0x14 value[4], +0x18
FILE*} at +0x000, and a saved copy at +0x2a0. 004051 initialises kind/first/pending/binary/FILE
for all 24 at its start (values are not initialised; the first store overwrites them). 003997
stores a value: if not first and equal to the previous one it clears `emit` and, when pending,
prints a `PsDefaultSettings <kind>(...)` line; otherwise sets `pending`. 003999 prints
`<kind>(...) ` inline when `emit` is set. Kinds (003999 table): 0 position, 1 orientation,
2 density (only when > 0), 3 sides, 4 localposition, 5 localorientation, 6 plane, 7 height,
8 radius, 9 material(mat<fistp(v)+1>), 10 com, 11 comrot, 12 inertia, 13 mass, 14 velocity,
15 angularvelocity, 16 force, 17 torque, 18 wakeupcounter, 19 lineardamping, 20 angulardamping,
21 maxangularvelocity, 22 solvercount (fistp), 23 group (fistp). Record k sits at +k*0x1c
(10 at +0x118, 22 at +0x268). Kinds 16 force and 17 torque are never stored or printed by the
dump. An actor with exactly one shape writes that shape through 004048 and the actor fields
follow on the same line (0x94a71 -> 0x94b29). Any other shape count, 0 included, increments the
shape counter, saves the block, writes `PsShapeBegin Shape%d\r\n` + each shape (each followed
by `\r\n`) + `PsShapeEnd\r\n`, restores the block and writes `PsShape Shape%d ` before the
actor fields; the shape counter is 004062's local, shared across scenes.

Actor line (004051/004055/004057): quaternion from saveToDesc's globalPose matrix (the listing's
trace-branch conversion, `fsqrt`), records 0 position, 1 orientation, 2 density; if
saveBodyToDesc returns true, records 10 com (massLocalPose.t, +0x118), 11 comrot (+0x134),
12 inertia (+0x150), 13 mass (+0x16c), 22 solvercount (`(float)(unsigned)solverIterationCount`,
+0x268), 14 velocity, 15 angularvelocity, 18 wakeupcounter, 19 lineardamping, 20
angulardamping, 21 maxangularvelocity -- stored in that order (0x94903-0x94a41) and printed in
the same order (0x94b8b-0x94bff). Line layout: the single shape's part (or `PsShape Shape%d `),
then `name(<name>) `, `awake(false) ` when the actor has no record or record+0x4c (wakeUpCounter) is
zero, the inline settings, `static(true) ` when not dynamic, else `kinematic(true) ` (body flags
bit 7), `locked(true) ` (bits 1-6 all set) or `locked(%s,%s,%s,%s,%s,%s) ` (bits 1-6, in the
listing's argument order) when some are set; `collision(false) ` when the saved actor desc's
`flags` (+0x38, `test byte ptr [esp+0x90], 1` at 0x94cae) has bit 0; `\r\n`.

Shape line (004048, switch on `[shape+0xd0]` = 001283): builds the family's `Nx*ShapeDesc` on
the stack (setToDefault inlined), calls the internal shape's slot 13 (`[vt+0x34]`, saveToDesc)
on the shape cast by type, stores localposition/localorientation (quaternion from localPose),
material (record 9, materialIndex, desc +0x3e) and group (record 23, desc +0x3c), and prints
these inline records in this order:
- plane: `PsPlane  ` plane, localposition, localorientation, group, material (group before
  material only here);
- sphere: `PsSphere ` radius, localposition, localorientation, material, group (0x93689/0x93694);
- box: `PsBox ` sides (dimensions), localposition, localorientation, material, group;
- capsule: `PsCapsule ` height, radius, localposition, localorientation, material, group;
- type 4: `PsConvex <mesh> ` or `PsTriangleMesh <mesh> ` (after 004046 wrote the mesh),
  localposition, localorientation, material, group;
- type 5 and others: nothing. Each ends with 004017: `triggerevent(` + `enter,`/`leave,` ... for the trigger flag bits.

Mesh (004046, type 4 only): `PsConvexBegin tmesh%d`/`PsTriangleMeshBegin tmesh%d`,
`@pmap(%d)`, `gouraud(%s)`, `winding(%s)`, `@heightfield(%s, %f)`, `PsVert T T T` (004035),
`PsFace %d`, `PsTri %d %d %d`, `PsConvexEnd`/`PsTriangleMeshEnd`; names from 003991 (`tmesh%d`
with a per-mesh index); reads the mesh object through vtable +0x28/+0x0c/+0x34 and 001472.

Joint block (004037, `ret 0xc`, `this` = the settings object): 004068 bodies, 004070 type;
types 0-5 only (jump table 0x10092b30 has six entries; `ja` for type > 5):
`PsJointBegin <joint name>`, `### <Prismatic|Revolute|Cylindrical|Spherical|Point On Line|Point In Plane> Joint`,
then the family desc on the stack (setToDefault inlined; tables 0x101182b4, 0x101182a4,
0x10118280, 0x101182c0, 0x10118298, 0x1011828c), `004072(type)` returns the internal joint when
its +0x168 matches, internal slot 10 (`[vt+0x28]`, saveToDesc) fills the desc, 004007 prints
`PsJointOffset frame(primary) ... offset(T,T,T)` (localAnchor[0], desc +0x40), the secondary
offset (localAnchor[1], +0x4c), two `PsJointAxes` lines with xaxis = localAxis[i] (+0x28 / +0x34)
and yaxis = localNormal[i] (+0x10 / +0x1c), `PsDefaultSettings bodycollide(true|false) `
(jointFlags +0x68 bit 0), `breakable(false) ` when maxForce and maxTorque (+0x58/+0x5c) are both
FLT_MAX else `breakable(T,T) `, `name("<name>") ` when 004085 finds one, `\r\n`; then `PsJointLimit`
and the family's limit text (revolute: `twist(T,T,T,  T,T,T)` via 004009 / `motor(...)` via
004013 / `twistspring(T,T,T)` via 004011 from flags bits 0-2; spherical: swing/twist/twist-
spring/swing-spring/joint-spring/projection; the others fixed strings). Every type, 0-9, then
gets `PsJointLimitPlane T T T T` per limit plane (004081, 004083, 004145) and `PsJointEnd`.
Types 6-9 (distance, pulley, fixed, D6) produce only the limit-plane lines and `PsJointEnd`.

Names (004004 joints, 004006 actors; every `%s__%I64x` in 004062 too): the pointer is
sign-extended with `cdq` before the 64-bit push, so faithful code passes `(__int64)(int)ptr`.
`"$__%I64x"` of the internal object (the joint, or the
actor's 0x50-byte body); when it has a name: `<name>___%I64x`, quoted with `"` when the name
contains a delimiter (004002: space, quote, tab, comma, parens, `=`, brackets, braces, `#`);
a null actor is `@world`. Name sources: joints 004085 (the registry 000454 keyed on the internal
joint, i.e. `nxGetSdkPointerBinding`), actors 000086 (NpActor slot 84, getName).

Effector (004061): 003974, 003975, 003964 (owners `[record+0x19c]` of both bodies and the world
anchors); if either owner has a record: `PsDefaultSettings spring_dist_relaxed(%f)\n`,
`spring_dist_compress_saturate(%f) spring_dist_stretch_saturate(%f)\n`,
`spring_max_compress_force(%f) spring_max_stretch_force(%f)\n`,
`spring_vel_compress_saturate(%f) spring_vel_stretch_saturate(%f)\n`,
`spring_damper_max_compress_force(%f) spring_damper_max_stretch_force(%f)\n`,
`spring_pos1(%f,%f,%f)\n` (owner 1 non-null), `spring_pos2(%f,%f,%f)\n` (owner 2 non-null),
`PsSpring <a> <b>\n`. 003964 dereferences `[+0x24]+0x19c` and `[+0x28]+0x19c` before any null
test (0x8f390-0x8f3ac): an effector with a world end crashes the oracle's dump.

### Readers and whether the candidate has them

| Reader | Used for | Candidate |
|---|---|---|
| 004068 (56) joint bodies' owners | 004037, 004015 | model only -> write (Joint.cpp) |
| 004070 (7) joint type | 004037 | written, `core/Joint.cpp` |
| 004072 (25) `is(type)` over +0x168 | 004037 | not written -> write (Joint.cpp) |
| 004081 / 004083 / 004145 limit-plane iterator | 004037 | written, `core/Joint.cpp` |
| 004085 (10) joint name lookup | 004004, 004007 | model only -> write (Joint.cpp; wraps `nxGetSdkPointerBinding`, product 000454 in PhysicsInternal.cpp) |
| joint internal slot 10 saveToDesc, types 0-5 | 004037 | written for every family (`core/*Joint.cpp` 004376, 004330..., see Np headers) |
| 000563 / 000567 scene joint iterator | 004051 | written, `Scene.cpp` (`resetJointIterator`/`getNextJoint`) |
| NxActor +0x148 saveToDesc, +0x144 saveBodyToDesc | 004051 | written, `NpActor.cpp` `NpActorVtable::saveToDesc`/`saveBodyToDesc` (slots 82/81) |
| 000086 actor getName | 004006 | product `NpActorVtable::getName` (slot 84); it reads `gNxShapeNames` (NpActor.cpp), not the 000454 registry -- equivalent for the dump only if called through the product getName |
| 000015 (41) / 000017 (28) shape count / list of the 0x50 body | 004051 | model only (`nxBodyShapeRecordCount`); the same logic is inlined in `NpActorVtable::getNbShapes`/`getShapes`, but 000017 returns the internal list (+0xe0) where the product getShapes returns +0xf0 -> write both |
| 001283 (7) `[shape+0xd0]` | 004048 | model only -> write or read the word directly as the listing's call |
| shape internal slot 13 saveToDesc, types 0-3 | 004048 | written (`ObjectModel.cpp` `nxPlaneSaveState`/`nxSphereSaveState`/`nxBoxSaveState`/`nxCapsuleSaveState`, installed by `nxShapeFactory`) |
| mesh shapes (type 4), mesh vtables, 001472 (664) | 004046 | not constructible: `nxShapeFactoryInstallVtable` has no type-4 table -> write 004046 faithfully, 001472 as a deferred asserting stub, and keep meshes out of the test |
| 000509 (33) gravity copy | 004051 | model only -> write (Scene.cpp); the candidate stores gravity at +0x520 from the scene desc (Scene.cpp:1725); `NpScene::setGravity` is still empty |
| Scene +0x52c/+0x530/+0x534/+0x53c/+0x544 | 004051 | written by the candidate initialiser |
| 000523 (4) pair count `[+0x3c]` | 004051 | model only -> write; the candidate keeps +0x3c at 0 |
| 000525 (61) + 000527 pair array | 004051 | not written (walks the hash at +0x624 through 001957) -> leave; the test sets no pair flags, so 000523 returns 0 and it is not called |
| 001281 (4) shape owner | 004059 | written (`ContactGeneration.cpp`) |
| record +0x4c (wakeUpCounter), +0x19c owner | 004055, 003964 | written by `nxActorComputeMass` |
| 003974 / 003975 / 003964 effector readers | 004061 | 003974/003975 models, 003964 discovered -> Task 2 |
| PhysicsSDK scenes, materials, `instance` | 004062 | written (`PhysicsSDK.cpp`) |
| live parameters 0x10123b18, group masks 0x10123a98 | 004062 | written, but file-static in `PhysicsSDK.cpp` (`gParameter`, `gGroupCollisionMask`): the dump file needs them exported (an internal accessor or `extern`) |

Globals the dump TU owns: `%s__%I64x` name buffer .data 0x10126878 (0x100), token ring
0x10126978 (16 x 0x40) with index 0x10126d78, joint-name buffer 0x10126d80.

### Scene contents covered

Superseded by "### Task 4 record" (what the test scenes actually hold and which branches stay
unreached); kept as the pre-test survey.

Covered: SDK materials, the 13 SDK constants, group collision masks, per scene the timing
header, gravity, joints (full blocks for prismatic, revolute, cylindrical, spherical,
point-on-line, point-in-plane; `PsJointEnd` only for distance, pulley, fixed, D6), limit
planes, joint names/breakability/collision flag, actors (pose, density, body state, flags,
name, sleep state), shapes (plane, sphere, box, capsule, convex, triangle mesh with its
vertices and faces; trigger flags, group, material, local pose), multi-shape actors, joint
actor pairs, disabled actor/shape pairs, spring-and-damper effectors, the addendum.
Not covered: fluids, implicit meshes, controllers, shape names, pair flags other than
"disabled", anything of a simulated scene beyond the timing header.

### Proposed test scene for Task 4

Superseded by "### Task 4 record": the scenes built differ (two scenes, then a third;
multi-shape and zero-shape actors; explicit masses; triggers only where the oracle accepts them).
Kept as the plan the record amends.

Write every reader the dump needs and exercise only what the candidate can construct:
- one SDK, one scene (header `Contains one Asset.`), created with a non-default gravity in the
  scene desc; never simulated (header `Scene in initial configuration.`);
- the default material plus one added material with non-trivial friction/restitution and a
  flags value with bits 0 and 1;
- one SDK parameter changed (visible in `PsSetConstant`), and one group pair disabled through
  `setGroupCollisionFlag` (one `PsGroupCollisionFlag` line per group touched);
- actors, one shape each (no multi-shape actors -- the candidate's group builder is a hole):
  a static actor with a plane shape, a static actor with a box, dynamic actors with a sphere, a
  box (non-default local pose, group and material) and a capsule; one actor named with a plain
  name and one with a space in the name; one dynamic actor with `NX_BF_FROZEN_POS_X` (the
  `locked(...)` arm) if `raiseBodyFlag` is faithful; no mesh shapes; no triggers unless the
  candidate's shapeFlags round-trip is proved;
- joints between the dynamic actors: revolute with limit, motor and spring flags set, spherical,
  prismatic, cylindrical, point-on-line, point-in-plane, one of distance/fixed (bare
  `PsJointEnd`), one joint with a limit plane, one named and one breakable joint, one joint to
  the world (`@world`);
- one effector between two dynamic actors (both ends non-null: 003964 crashes on a world end),
  with all nine spring/damper values distinct;
- no actor/shape pair flags;
- coreDump four times: text and binary, each with and without an addendum; print the return
  value (false) and the file read back.

The transcript must normalise two things the oracle writes that differ between runs and between
the DLLs: line 3 (the date and time), and every hex pointer after `__` (the PhysicsSDK, scenes,
actor bodies and joints). Replace each distinct pointer by its first-appearance ordinal before
printing. Text lines are otherwise printed verbatim; the binary-mode `$%x` suffixes are
deterministic. The dump consumes the scene's joint iterator (+0x6bc) and the actor's settings
are read under the recursive scene lock that 000267 already holds.

### Task 3a record

File placement: `Physics/src/core/SceneDump.cpp` with `Physics/src/include/core/SceneDump.h`.
No row in the gap reports an error or carries another `__FILE__`-style string, so nothing names
the oracle's unit; the file is named by what it does. It is on the `/arch:IA32` list (the rows
compare and widen x87 floats and round three setting kinds with `fistp` at the live control
word; Task 3b's asset writer adds the `fsqrt` pose-to-quaternion conversion) and on the `/EHs-c-`
list (the joint block keeps descriptors with virtual destructors on its stack; the oracle's frame
has no unwind state). The readers 004068, 004072 and 004085 are `Joint` members in
`Physics/src/core/Joint.cpp` (`getBodyOwners`, `is`, `getName`). `PhysicsSDK.cpp` keeps
`gParameter` and `gGroupCollisionMask` file-static and exposes them through
`nxPhysicsSDKParameters()` and `nxPhysicsSDKGroupCollisionMasks()` (declared in
`PhysicsSDK.h`); `PhysicsSDK::coreDump` (004062) is declared in `PhysicsSDK.h` and defined in
`SceneDump.cpp`.

Written (27 rows, 11,793 B): 003992 (`sceneDumpDateTime`), 003994 (`sceneDumpPointerName`), 003995
(`sceneDumpToken`), 003997/003999 (`SceneDumpSetting::store`/`print`), 004002
(`SceneDump::hasDelimiter`), 004004 (`jointName`), 004006 (`actorName`), 004007
(`writeJointFrames`), 004009 (`limitPairText`), 004011 (`tripleText`), 004013 (`motorText`),
004015 (`writeJointLine`), 004037 with its continuations 004039/004041/004043 (`writeJoint`),
004062 (`PhysicsSDK::coreDump`); readers 004068, 004072, 004085. The descriptor inlines 003981
003985 004021 004023 004025 004027 carry stable-ID lines only (the compiler emits them from the
public headers where `writeJoint` constructs the descriptors). Format strings: all 163 distinct
literals the file passes to the CRT occur NUL-delimited in the image; the 164th, `"\r\n"`
(0x101135bc), is the tail of a longer string there, as the oracle's pointer is.

Corrections and additions to the sections above, from the listing:
- The 0x20000-byte block is not unused. 004062's frame words 0x1c, 0x20 and 0x24 are
  `SceneDumpNames` {shape-group counter, mesh count, mesh table}: 004062 zeroes the first two and
  stores the block's address in the third, and passes the three by address as 004051's fifth
  argument, which hands it to 004048 and on to 004046/003991. 003991 is `thiscall` on it: it looks
  the mesh up in the table (+8, count +4), appends it when new, formats `tmesh%d` and caps the
  count at 0x7fff; the block holds 0x8000 pointers. 004062 frees it through `free` on both paths.
- 004051 is `thiscall` on the settings object with five arguments (`ret 0x14`): scene, FILE*,
  binary, the scene index (not read), `SceneDumpNames*`; it returns `al = 1`, which 004062
  ignores. Task 3b replaces the placeholder `SceneDump::writeAsset` (an `NX_ASSERT(0)` that writes
  nothing) with it and adds its stable-ID line.
- The material lookup in 004062 is not 000456: it takes the index as an `NxMaterialIndex` and
  falls back to element 0 past the end, with no bit-31 test (0x95240-0x95277). The loop count
  is `this`'s material count, the lookup is on `PhysicsSDK::instance`; both counts are signed
  quotients (`jl`), the scene count unsigned (`jb`).
- 004011 is one row for three floats of either an `NxJointLimitDesc` (spherical swing limit,
  0x9272b) or an `NxSpringDesc` (every spring); it is written once, taking the first float's
  address.
- 004015's third argument is not read; its owners come from 004068 into two zeroed locals, as in
  004037, whose owners are read and never used.
- 003994 takes its pointer in `eax` (`cdq` on entry; its one caller, 004006, loads `eax` first):
  a register convention the candidate's compiler does not produce without whole-program
  optimisation; the candidate passes it as a parameter. Output is the same.
- 004007 compares maxForce and maxTorque with 0x7f7fffff as words (0x911a4-0x911b3); written as a
  bit comparison.
- The descriptor rows: the oracle folded identical bodies (004023 for four `isValid`s, 003985 for
  the base and the four trivial `setToDefault`s, 004025 for every descriptor's scalar deleting
  destructor). The candidate links `/OPT:NOICF` and keeps one copy per class
  (`?isValid@NxCylindricalJointDesc@@UBE_NXZ` and the prismatic, point-on-line and point-in-plane
  copies; `??_G<Desc>@@UAEPAXI@Z` per descriptor); the revolute and spherical tables' slot 1 is
  `NxJointDesc::setToDefault` in both DLLs.

Model rows now product rows: 003985, 004002, 004068 (no model code in the repository; the recorded
proofs stay) and 004085 (`nxRegistryLookupNull` in `ObjectModel.cpp` stays and gains a
`// Product row:` pointer). Not reachable yet: `NpPhysicsSDK::coreDump` (000267) is still the
candidate stub; dynamic proofs come with Task 4's transcript.

### Task 3b record

Written (23 rows, 11,630 B, matching the split): in `Physics/src/core/SceneDump.cpp`, 003991
(`SceneDumpNames::meshName`), 004017 (`SceneDump::writeTriggerFlags`), 004035
(`SceneDumpNames::writeVertices`), 004046 (`SceneDump::writeMesh`), 004048 (`SceneDump::writeShape`)
and 004051 with its continuations 004053 004055 004057 004059 004061 (`SceneDump::writeAsset`,
replacing 3a's placeholder); the shape-descriptor inlines 003983 003987 003989 004019 004029 004031
004033 carry stable-ID lines only (the capsule, sphere and triangle-mesh descriptor tables; the
compiler emits them where `writeShape`/`writeMesh` construct the descriptors). The readers are
written with their own units: 000015/000017 as `JointActorBody::getNbShapes`/`getShapes` in
`core/JointSupport.cpp`, beside 000022 of the same gap unit (`JointActorBody` now names its +0x10
shape word, `mShape`); 000509/000523 as `NxSceneInternal::getGravity`/`getNbPairs` in `Scene.cpp`;
001283 as `NxShapeGetType` beside 001281 in `ContactGeneration.cpp` (so `NxPhysicsInternalTests`,
which links `core/*.cpp`, now also links `NarrowPhase.cpp` and `ContactGeneration.cpp`). Former
model rows now product rows: 003983 003987 003989 000015 000017 000509 000523 001283 (000015's
`nxBodyShapeRecordCount` model in `ObjectModel.cpp` stays with a `// Product row:` pointer).

Deferred, as `NX_ASSERT(0)` stubs that write nothing (rows stay `discovered`): 001472 (the convex
mesh's polygon builder, `SceneDumpConvexMesh::buildPolygons` in `SceneDump.cpp`) and 000525 with
its continuation 000527 (`NxSceneInternal::getPairFlagArray` in `Scene.cpp`). Neither is reachable
in the candidate: no shape of type 4 can be built (`nxShapeFactoryInstallVtable` has tables for 0-3
only) and the candidate never raises a pair flag, so 000523 returns 0 and the pair block is
skipped. 004046 and the mesh arm of 004048 are written in full but likewise not reachable; the
dump test keeps meshes and pair flags out of its scene. The internal shape's slot 13 and the
internal mesh's slots 3/10/13 are called through two call-view classes (`SceneDumpShape`,
`SceneDumpTriangleMesh`); the candidate's `TriangleMesh` has no C++ table, so the mesh view is a
contract for a later mesh task, not something that runs.

Listing details recorded while writing:
- The actor record reads both descriptors through the body's `NxActor` (+0), not through the
  Scene array's pointer; the actor descriptor is an `NxActorDescBase` (its empty constructor
  leaves only `globalPose`'s identity, matching 0x94324-0x94382), the body descriptor an
  `NxBodyDesc` (setToDefault inlined, 0x9457c-0x946bb).
- `NxMat33::toQuat` is inlined in three spellings, reproduced by `sceneDumpQuat`: the actor and
  centre-of-mass poses form m8 + m4, spill it to a float and add m0 on the stack (the m0 arm uses
  the float); the plane, sphere, box and capsule arms form m0 + m8 and add m4 (the m4 arm uses the
  float); the mesh arm stores m0 + m8 first (`fstp`) and forms the trace from the stored float
  (0x93e6e-0x93e85). The root, `0.5 * s` and `0.5 / s` stay on the stack.
- `awake(false)` tests the record's +0x4c as a word (`test edx, edx`), so -0.0 counts as awake.
- The solver count is stored as `(float)(unsigned)` (`fild` + 2^32 when negative, 0x9496b).
- The box stores its sides (the dimensions doubled, `fadd st0, st0`) before the pose records; the
  capsule prints height before radius.
- The capsule arm passes the capsule's own `flags` (desc +0x54, `mov ecx, [esp+0x7c]` at 0x93d02)
  to the trigger writer 004017 where every other arm passes `shapeFlags` (+0x38). Written as the
  listing does.
- With `NX_SHAPE_DESC_LIST` (Nxp.h) `NxShapeDesc::next` sits at +0x48, so each family's fields
  start at +0x4c (plane normal/d +0x4c/+0x58, capsule radius/height/flags +0x4c/+0x50/+0x54, mesh
  meshData/meshFlags +0x4c/+0x50); pinned by `static_assert`s.
- 004035's weld compares each vertex with every earlier one, written or not (`fabs` against
  1e-5f, .rdata 0x10107a08) and maps a match to its index; 004046's triangle lines break every 16
  triangles; a convex mesh with a null hull frees a null remap; the triangle loop is signed.
- The pair block's arrays are `operator new` (005701) and are freed through `free` (005668); the
  0xc-byte pair records are released through 005700, a `jmp` to `free`, written as
  `operator delete`, behind the listing's null test.
- The effector block's five fixed lines and `PsSpring` use `%f` of the promoted floats with `\n`
  endings; 003964 reads both records' +0x19c before the null tests (a world end still crashes the
  oracle), and the anchors print only for a non-null end.

Format strings: all 225 distinct literals `SceneDump.cpp` now passes to the CRT occur in the image;
222 NUL-delimited, and three start exactly at the oracle's pointer behind a non-string word: `"\r\n"`
(0x101135bc, the tail of a longer string, as in 3a), `"PsVert %s %s %s\r\n"` (0x101182e4) and
`"tmesh%d"` (0x10117a50), each preceded by the last pointer of a descriptor table. Every one of the
64 literals the 3b listing references appears verbatim.

Not reachable yet: `NpPhysicsSDK::coreDump` (000267) is still the candidate stub; Task 4 wires it
and records the dynamic proofs.

### Task 4 record

Wiring: 000267 is written in `Physics/src/NpPhysicsSDK.cpp` as the listing has it (stable-ID
line; `NpScene::writeLink()` exposes the +0x0c link). For i < `getNbScenes()` it tries the write
link of `getScene(i)`'s NxScene wrapper (+0x6cc) with `nxNpSceneGuardWriteTry` (002364); on a
failure it releases the links already taken in reverse (002366), reports code 2 at
`NpPhysicsSDK.cpp` line 225 (the immediate 0xe1) with the deadlock message and returns false
without calling the dump. Otherwise it calls `PhysicsSDK::coreDump` (004062), keeps the result,
releases every link in order and returns it (always false). The file name, binary flag and
addendum pass through unchanged; 004062 appends `.psc`.

Test: `NxPhysicsCoreDumpTests` (`tests/PhysicsCoreDumpTests.cpp`), a staged-pair target on Phases 6
and 7 under the 0xcd page fill. Scene contents follow "### Proposed test scene" with these
changes, each forced by the oracle or by an unwritten row:
- two scenes (header `Contains 2 assets.`, the shape counter shared across them), then one, then
  none;
- multi-shape actors (three shapes; two in scene B) and a zero-shape actor are in: the oracle and
  the candidate build them alike;
- every dynamic actor but one gives `mass` and `massSpaceInertia` (three give a shifted or rotated
  mass frame). Mass from shapes is phys_fn_000008 (Phase 5, discovered); the candidate's creation
  model (`nxActorComputeMass`, Scene.cpp) covers one unrotated box from a density only. The first
  run showed the gap: sphere, capsule and compound bodies with mass 0, the local-pose box without
  its com/comrot, a 0.3 cube's inertia one ulp off, and a joint limit plane whose round trip went
  through the missing mass frame. Only `mover` (an unrotated box) takes its mass from a density;
- trigger flags sit on the static box and on one part of the compound: the oracle refuses a
  dynamic actor whose only shape is a trigger (`Actor::loadFromDescInternal: Can't compute mass
  from shapes: must have at least one non-trigger shape!`), which the candidate accepts (the
  same unwritten mass path; recorded, not fixed here);
- the deadlock arm: scene B's lock block is made to look held by another thread (flag +0x18 = 1,
  owner +0x1c = another id); the report, `returned=0`, no file, and scene A's flag back at 0 are
  identical in both DLLs.
Normalised, and nothing else: the date line (everything after `Generated on `), and the hex of
every `__<hex>` pointer token, replaced by its first-appearance ordinal over the run. The CRT
digit rule of `NxPhysicsJointSlotTests` is in the harness as a guard, but no token reaches it:
FLT_MAX prints as the literal `fltmax` (003995), `NX_COLL_INFINITY` included.

Result: one source defect in what the dump reads. The body loader (000795) takes each threshold a
body descriptor leaves at or below zero from the SDK's live parameter array: +0xd8 =
`NX_MAX_ANGULAR_VELOCITY` squared (`fld`/`fmul [0x10123b34]` at 0x1b04a), +0xd0/+0xd4 =
`NX_DEFAULT_SLEEP_LIN_VEL_SQUARED`/`NX_DEFAULT_SLEEP_ANG_VEL_SQUARED` copied (0x1b1fc, 0x1b3ac).
The candidate's model pinned 7.0 and the two defaults, so `maxangularvelocity(7)` printed where the
oracle printed the test's `NX_MAX_ANGULAR_VELOCITY` (9). Fixed in `Scene.cpp`. After it the
transcript is identical (`stdout_delta=0`); no writer row needed a change. 278 oracle lines are
registered; floors 6/7 = 760/633.

Dumps (lines / normalised bytes): text 298 / 16,940; text + addendum 302 / 17,079; binary 298 /
19,479; binary + addendum 301 / 19,608; one scene 239 / 13,916; no scene (binary, empty addendum)
67 / 1,799.

Trace (`evidence/effector-and-coredump-trace-coredump.txt`): every row the scene can reach runs:
000267 (7 calls), 004062 (6), all written 3a/3b rows but the mesh arm, with the continuations and
the three rows the candidate's compiler inlined (003992, 003994, 004013) anchored inside their
enclosing functions. Not hit: the mesh arm (003991 004035 004046 001472), the pair loop (004059)
and 000525, and the thirteen descriptor inlines, which nothing calls.

Not hit, after Task 5's scene C as well:
- the elapsed-time header of 004051 (`## Total Elapsed Time`, `## Elapsed Time Last Frame`,
  `## MaxTimeStep`, `## MaxIter`, `## TimeStep = FIXED|VARIABLE`): it prints only when Scene+0x544
  is non-zero, i.e. after a simulate, which the candidate does not have;
- the mesh arm (003991 004035 004046 001472, and 004048's type-4 arm): no mesh shape can be built;
- the pair block (004059's `PsActorPair` loop, 000525/000527): no pair flag is ever raised;
- settings kinds 16 force and 17 torque: never stored or printed by the dump (not a gap);
- the thirteen descriptor inlines, which nothing calls;
- an effector with a world end: the oracle's 003964 faults on one, so no test can compare it.
Every other settings kind prints its `PsDefaultSettings` line in scene C, and `awake(false)` is
reached on a dynamic body (an unjointed actor created asleep; the jointed actor created asleep in scene A does not print
it, in either DLL).

### Task 5 addendum (review coverage)

Scene C (dumped after scenes A and B are released, text and binary, with the pointer ordinals
restarted: an address a freed object had can be handed out again, and which one depends on the
process's address-space history, so aliasing with freed objects is not compared) holds an
unjointed actor created asleep, two static capsules, pairs of equal actors and shapes, and a
static three-shape actor. Its first run found one defect: the candidate's shape factory
(`nxShapeFactory`, Scene.cpp) did not store a capsule's own `flags` (desc +0x54) at +0xe8, where
the capsule loader 000989 stores them (0x00021af0) and slot 13 reads them back; 004048's capsule
arm hands that word to 004017, so the oracle printed `triggerevent(enter,)` for a capsule with
`NX_SWEPT_SHAPE` and the candidate nothing. Fixed (d628181). A capsule's `shapeFlags` triggers do
not print, in either DLL: the quirk recorded in "### Task 3b record". 95 lines added, inserted before the target list's last pre-Task-5 entry;
floors 6/7 = 855/728. The final review added the outstanding SDK block count at the epoch reset
(`allocator after_release outstanding=14`; floors 856/729); the raw allocation and free counts
differ (actor-creation models; evidence, "## Open items") and are not printed.

## Task split

Out of scope: 004064, 004066 and 004070, in the same gap, are already written in
`core/Joint.cpp`.

| Task | Rows | B | Content |
|---|---:|---:|---|
| 2 | 30 + 12 | 3,325 + 887 = 4,212 | the effector rows above (003922-003979 without 003981), Scene/NpScene 000301 000303 000327 000329 000331 000561 000565 000569 000573 000575 000587 000594; plus the body-record Observable (parts of 000797/000776), the `notifyObservers(0x100)` behaviour of 000030 (0x1d82) and 000122 (0x3af6) in the candidate's record teardown (those Phase 5 rows stay `discovered`), 000575 wired into `nxSceneDelete` after the actor loop, 000713 (32) and a 000791 stub |
| 3a | 24 + 3 | 11,702 + 91 = 11,793 (23 rows / 11,448 B without 003981) | dump infrastructure, joints and entry: 003981 003985 004021 004023 004025 004027 (joint-desc inlines), 003992 003994 003995 003997 003999 004002 004004 004006 004007 004009 004011 004013 004015 004037 004039 004041 004043 004062; readers 004068 004072 004085; export the parameter and group-mask arrays |
| 3b | 18 + 5 | 11,517 + 113 = 11,630 | asset writer, actors, shapes, meshes, effectors: 003983 003987 003989 004019 004029 004031 004033 (shape-desc inlines), 003991 004017 004035 004046 004048 004051 004053 004055 004057 004059 004061; readers 000015 000017 000509 000523 001283; stub 001472 |
| 4 | 1 | 221 | 000267, the wiring and the staged-pair dump test |

The 13 desc-inline rows (003981 003983 003985 003987 003989 004019 004021 004023 004025 004027
004029 004031 004033, 3,350 B) are the `setToDefault`/`isValid`/scalar-deleting-destructor
virtuals of the public `Nx*JointDesc`/`Nx*ShapeDesc` classes the dump builds on its stack, in
tables 0x10117a44 and 0x10118274-0x101182d8: capsule `[004031 003983 004019]`, cylindrical
`[004025 003985 004023]` (point-in-plane, point-on-line and prismatic share it), revolute
`[004025 003985 004021]`, spherical `[004025 003985 004027]`, sphere `[004031 003987 004029]`,
convex/triangle mesh `[004031 003989 004033]`, 0x10117a44 `[004025 003985 003981]`. Nothing
calls them directly; MSVC emits them from the public headers wherever the dump constructs the
descs, so they get stable-ID lines on generated rows (as 004755 was) rather than hand-written
bodies. The box and plane desc tables (0x10108818, 0x1010656c) belong to other units.
