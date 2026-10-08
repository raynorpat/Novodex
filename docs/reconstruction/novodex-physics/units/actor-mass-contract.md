# Actor mass from shapes: contract

Recovered by actor-mass Task 1 from the Capstone listing (authoritative), the .rdata tables and
strings read from the image (`D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, base
0x10000000), and the candidate sources at d628181 (`claude/nostalgic-hamilton-a71fa5`). No
bundle was generated: `unit_bundle.py` names its output after the unit, and the unit that owns
000008, `gap:<start>..Actor.cpp`, contains `<`, which Windows refuses in a file name. The rows
below were read straight from `oracle/capstone/manifest.json`.

## The call chain

    Scene::createActor 000626 (0x11730)
      inlined NxActorDesc::isValid per descriptor type, isValidInternal at 0x11d23
      malloc(0x50) + Actor ctor 000013, Actor::loadFromDescInternal 000034 (0x2010)
        shapes: one -> shape factory 000032 (0x1de0); several -> malloc(0x110) + compound
                ctor 001033 (0x22d60, table 0x10106c2c) + 001041 per child
        body -> 000026 (0x19b0, __thiscall ret 4, the body creation)
          000010: Row000010Fixture copies exactly 0x78 bytes (including +0x74)
          if massSpaceInertia's three words are all integer zero (0x19d6..0x19e9):
            000008(density = [actor+0x18], &copy.mass, &copy.massLocalPose,
                   &copy.massSpaceInertia); non-zero -> return it (0x1a16)
          malloc(0x260), body ctor 000797, 000630
        result 1 -> report line 0xe5, 2 -> report line 0xe6 (Actor.cpp), return 0
      load failed -> Actor dtor 0x1c40, free, report line 0x228 (Scene.cpp), return 0

## phys_fn_000010 (0x00001390, 150 B): NxBodyDesc copy

`Row000010Fixture::row000010` in `Physics/src/Scene.cpp` copies exactly `0x78` bytes into the destination. The Phase 5 `NxPhysicsBodyCreationTests` mutation shortens the copy to `0x74`, dropping `solverIterationCount`, and is caught with `stdout_delta=2`; the restored staged-pair output is exact. See `evidence/phase5-actor-body-desc-copy-000010.md`.

## phys_fn_000008 (0x000010a0, 751 B): Actor::computeMass

`__thiscall` on the 0x50-byte internal actor (`ecx`, the candidate's actor body at
NxActor+0x14), four stack arguments, `ret 0x10`:
`(NxReal density, NxReal* totalMass, NxMat34* massLocalPose, NxVec3* massSpaceInertia)`.

| Listing | What it does |
|---|---|
| 0x10a9..0x10af | 000847 with 1: the 0x34-byte MassFrame at [esp+0x5c] is zeroed |
| 0x10b4..0x10e0 | a zeroed NxVec3 at [esp+0x50]; `[actor+0x10]`'s slot 4 (`call [eax+0x10]`) with (&frame, 1.0f, &vec) |
| 0x10e3..0x10fc | false: 001583 (one-byte ret) on the frame, return 1 |
| 0x10ff..0x1128 | `fcomp [0x101041f0]` (0.0f), `test ah,0x41; jp`: frame mass <= 0 or unordered -> return 2 |
| 0x112b..0x1150 | massLocalPose->t = frame offset (+0x24..+0x2c), integer moves |
| 0x1153 | 000841: the frame translated to its centre |
| 0x1158..0x1177 | density <= 0 or unordered -> 0x12e6: `fld [totalMass]; fdiv [frame+0x30]`, the quotient kept on the stack; each tensor word times it, stored m32 |
| 0x117d..0x1188 | *totalMass <= 0 or unordered -> 0x1242: *totalMass = frameMass * density (m32); each word times the density |
| 0x118e..0x123d | otherwise each word times the density; *totalMass untouched |
| 0x1354..0x1375 | `NxDiagonalizeInertiaTensor(tensor, *massSpaceInertia, massLocalPose->M)` through the import at 0x101041b8, cdecl; result ignored |
| 0x1378..0x138c | 001583 on the frame, return 0 |

Callers: 000026 (above) and 000164 (NxActor::updateMassFromShapes, 1846 B, not reconstructed;
the candidate's slot is `(unimplemented)`). Through createActor the both-positive arm cannot be
reached: isValidInternal refuses a density with an explicit mass.

## phys_fn_000841 (0x0001c720, 43 B)

`__thiscall` on a MassFrame, no arguments, plain `ret`: `{-ox, -oy, -oz}` (fchs, fstp m32) on
the stack, then 000833 with it. Since `-o + o` is +0.0f in every component, 000833 takes its
centred path.

## phys_fn_001024 (0x000229b0, 87 B): the compound's slot 4

`__thiscall`, three stack arguments, `ret 0xc`. The children array is [this+0xe0]..[this+0xe4];
a child with any of the low three bits of its byte +0xde (the trigger flags) is skipped; the
others get their own slot 4 with the three arguments unchanged; the first false returns false
(`xor al,al`), otherwise true (also for an empty array).

The compound's table 0x10106c2c: 0 0x22e50, 1 0x27740 (001347), 2 0x256f0 (001277),
3 0x22970 (001022), 4 0x229b0 (001024), 5 0x22a10, 6 0x227d0, 7 0x22dd0 (001035), 8 0xa0f60,
9 0x22bf0 (001030), 10 0xa0f60, 11 0x22810, 12-14 0x27f00 (001391). The candidate's group had
no table at all (memset only), so 000008 could not reach its children; it now installs a table
with slots 1, 2, 4, 7 and 12-14 filled from rows it has and the others null.

## The slot-4 rows and the frame they build

| Row | RVA | Role | State before | This task |
|---|---|---|---|---|
| 000947 | 0x20850 | box slot 4: unless flags&7, 000849(density, &dims, &pose6c) | reconstructed | executed |
| 001371 | 0x27be0 | sphere slot 4: 000851(density, radius, &pose6c) | reconstructed | executed |
| 001008 | 0x22440 | capsule slot 4: 000853(density, 1, r, r+h, &pose6c) | reconstructed | executed |
| 001249 | 0x24f70 | base slot 4 (plane): false | reconstructed | -- |
| 000849 | 0x1c8c0 | box: 000829, then 000831 + 000833 on the pose, scale unless 1.0f, 000839 | reconstructed | 000833 call restored |
| 000851 | 0x1c930 | sphere: 000843 (pose folded inside), scale, merge | reconstructed | -- |
| 000853 | 0x1c980 | capsule: 000845, 000831 + 000833, scale, merge | reconstructed | -- |
| 000829 | 0x1bd00 | unit box over half-extents | reconstructed | **corrected** |
| 000845 | 0x1c7c0 | unit cylinder (the capsule's mass model: height 2(r+h)) | reconstructed | **corrected** |
| 000843 | 0x1c750 | unit sphere | reconstructed | -- |
| 000831 | 0x1bdc0 | rotation fold | reconstructed | executed |
| 000833 | 0x1c040 | translation (parallel axis) | reconstructed (provisional) | **retranscribed** |
| 000837 | 0x1c5c0 | scale | reconstructed | -- |
| 000839 | 0x1c630 | merge (checked against the listing, unchanged) | reconstructed | -- |
| 000847 | 0x1c880 | conditional zero | reconstructed | executed |

Corrections, each read from the listing:

* **000829.** `fld 1/3; fmul st(1); fstp [esp+0x1c]` (0x1bd31..0x1bd39): the diagonal factor
  is stored m32; the three pairwise sums of squares are stored m32 at 0x1bd67/0x1bd6c/0x1bd72;
  each diagonal is a stored sum times the stored factor. The earlier transcription kept both on
  the stack.
* **000845.** On the capsule's selector 1, `mov [ecx], edx` at 0x1c836 (the transverse value)
  runs before the `je` at 0x1c838 that selects the arm. The earlier transcription read it as
  the default arm's and left +0x00 unwritten.
* **000833.** Both paths form K(v) = [v]x[v]x (v v^T - |v|^2 I) as nine sums whose second
  terms multiply the 0.0f at 0x101041f0 and are kept (they decide the sign of a zero result).
  Centred path (c all integer-zero words, 0x1c0d7): I += m K(o). Displaced path (0x1c26f):
  I += m (K(o) - K(c)), with that arm's own stores (-cy stored by `fst`, -cy^2 and -cx^2 stored,
  the (1,1) sum from the register copy of -cx^2, -cz^2 kept). Every difference, product with m
  and sum with the old inertia is stored m32 (the rep movsd copies). The earlier version
  computed one Q(c) - Q(o) for both paths.

## Scene.cpp (the candidate's models of 000026, 000034 and 000626)

* `nxActorComputeMass` (000026's model): the copy, the integer-zero test and the 000008 call
  before the record is allocated, as 0x19b0..0x1a18 do; the rest reads the copy. The one-box
  density formula it had is gone.
* `nxActorLoadFromDescInternal` (000034's model): the two reports through
  `FoundationSDK::error(NXE_INVALID_PARAMETER, "\Epic\Novodex\SDKs\Physics\src\Actor.cpp",
  0xe5/0xe6, 0, message)` (0x21d7..0x2214) instead of a printf.
* `createActor` (000626's model): for descriptor types 1 and 2 the whole NxActorDesc::isValid
  (the header's predicate, isValidInternal included -- NxActorDescBase::isValid is not
  virtual, so the base call skipped it), reported with Scene.cpp's `__FILE__` and line 0x203
  (type 1), 0x209 (type 2) or 0x21b (other); "Actor Initialisation failed" at 0x228.
* `nxSceneActorDestroy` (the failed-creation half of 0x1c40): the actor's name and shapes are
  released through the same helper releaseActor uses, so the ids go back to the scene.

## Test coverage

`NxPhysicsActorMassTests` (new, Phase 5): sphere, box, small cube and capsule, centred and with
a local pose; a three-part posed compound with and without a trigger child; the mass-only arm
on a compound and a capsule; a density with an explicit mass (refused by createActor); a
trigger-only sphere and a trigger-only compound (refused, 000008 returns 2). Each created
actor prints getMass, getMassSpaceInertiaTensor and getCMassLocalPose as words.

`NxPhysicsCoreDumpTests` scene D (Phases 6/7): scene A's dynamic bodies and scene B's pair with
densities, both refusals and a revolute limit plane over two computed frames, dumped in text
and binary. Scenes A-C keep their explicit masses: their 373 dump lines are registered and the
Global Constraints forbid editing a registered line.
