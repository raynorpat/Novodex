# Joint families

Measurement note for the joint-families plan (`docs/superpowers/plans/2026-09-25-joint-families.md`),
which continues the revolute translation-unit pilot (`evidence/unit-pilot-revolute.md`): the
shared NpJoint base, the shared joint base rows, then the nine remaining joint families
(prismatic, cylindrical, spherical, point-on-line, point-in-plane, distance, pulley, fixed,
D6) reconstructed as real source and wired into `Scene::createJoint` behind the byte-exact
staged-pair joint test. The contract is `units/joint-families-contract.md`. Each task appends
one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T06:53:58 | 2026-09-25T07:04:52 | 13 | 591 | Shared NpJoint base: `NpJointShared<Iface, Internal>` (novtable) in `core/NpJointShared.h`; the 13 folded bodies (004539 004437 004441 004497 004499 004483 004573 004577 004491 004635 004443 004479 004743, owned by NpD6/NpPulley/NpDistance/NpPointInPlane/NpSpherical/NpPrismatic units) claimed in `core/NpJointShared.cpp`, 6 of them discovered -> reconstructed; revolute's 10 per-family NxJoint rows now call shared `forward*` helpers. Revolute vtable slot targets unchanged apart from the 13 moved members; joint gate stdout_delta=0; no transcript defects. |
| 2 | 2026-09-25T07:10:01 | 2026-09-25T07:39:10 | 12 | 10506 | Shared joint base: `## Shared rows` classifies all 142 rows of Joint.cpp and the three gaps (effector 19 rows, scene-dump writer 41, articulation/scene helpers 29, list helpers 5 -> defer; 004417-004433, 004115/004117, 004119 -> reuse). Written: 004064 004091 004093 004099 004101 004109 004111 004123 004133 004135 004143 (core/Joint.cpp), 004391 (core/JointSupport.cpp); new stub 000598. No family creation/getter path needed a new row. Deferred: 004085 004103 004105 004113 004068 004072, Scene rows 000022 000571 000598 000633 000758. Joint gate stdout_delta=0. An uncommitted setter experiment matched the oracle except where the rebuilt Foundation NxNormalToTangents differs (defect outside this task). |
| 3a | 2026-09-25T07:49:20 | 2026-09-25T08:23:00 | 19 | 8575 | Prismatic (type 0): core/PrismaticJoint.cpp 004376 004378 004380 004382 004384 004386 (7,608 B; 004386 is the 6,772 B solver slot, written from the Capstone listing with the new supplement decompile), core/NpPrismaticJoint.cpp 004731-004757 minus 004743 (967 B, 004755 generated). Supplement +0x000ad4e0/+0x000ad850. createJoint case 0 wired (0x17c, public object at +0x48); two nxPrismaticCase cases; 4 oracle lines per joint list, floors 6=19, 7=8. Transcript defects found: 0 (stdout_delta=0 on the first run). Deferred: 004318 (cylindrical-owned debug visualization in prismatic slot 4) to Task 3b. cdb trace: 004380 004753 004378 004751 004376 executed. |
| 3b | 2026-09-25T08:27:00 | 2026-09-25T08:55:08 | 19 | 7850 | Cylindrical (type 2): core/CylindricalJoint.cpp 004316 004318 004320 004322 004324 004326 (6,883 B; 004326 is the 5,377 B solver slot and 004318 the 1,115 B debug visualization shared with prismatic slot 4, written once as Joint::row004318), core/NpCylindricalJoint.cpp 004655-004679 (967 B, 004677 generated). Gap SphericalJoint..CylindricalJoint: all five rows (004306-004314) are spherical (004314 is 004312's tail), left to Task 3c. Supplement +0x000a7200/+0x000a7810. Prismatic: slot-4 stub replaced by the call, linear-record/solve/error helpers hoisted to core/JointLinearRecords.h. createJoint case 2 wired (0x16c, public object at +0x48); two nxCylindricalCase cases; 4 oracle lines per joint list, floors 6=27, 7=12. Transcript defects found: 0 (stdout_delta=0 on the first run). Rows deferred: 0. cdb trace: 004320 004675 004673 004316 executed. Ledger: 15 rows moved to reconstructed_not_falsified (counts 239/192). |
| 3c | 2026-09-25T09:04:23 | 2026-09-25T09:53:57 | 32 | 18944 | Spherical (type 3): core/SphericalJoint.cpp 004282-004314 (17 rows, 17,809 B, including the five gap rows 004306-004314; 004296 is the 5,907 B solver slot, 004312 the 4,461 B debug visualization whose 430 B tail 004314 is written inside it with its stable-ID line stacked above 004312's, 004310 the 2,942 B spring/limit slot, 004306 the fpatan twist helper), core/NpSphericalJoint.cpp 004623-004653 minus 004635 (15 rows, 1,135 B, 004651 generated); slots 34/36 carry the bodies of revolute 004703/004707 without stable-ID lines. Supplement +0x000a3090/+0x000a4a00/+0x000a5ee0. Shared: revolute's _CIacos reproduction moved to core/JointAcos.h; jointLinearRecord/jointSolveRecord reused for 004296's singular-arm records. createJoint case 3 wired (0x23c, public object at +0x48); two nxSphericalCase cases (also getFlags/getProjectionMode and every saveToDesc field); 4 oracle lines per joint list, floors 6=35, 7=16. Transcript defects found: 0 (stdout_delta=0 on the first run). Rows deferred: 0. cdb trace: 004300 004649 004284 004643 004286 004290 executed. Ledger: 22 rows moved to reconstructed_not_falsified (counts 217/214). |
| 3d | 2026-09-25T10:20:00 | 2026-09-25T10:40:00 | 20 | 4778 | Point-on-line (type 4): core/PointOnLineJoint.cpp 004268-004280 (7 rows, 3,811 B; 004272 is the 2,073 B solver slot -- two kind-1 records along the rotated normal and cross, through jointLinearError/jointLinearRecord/jointSolveRecord -- 004274 the 1,345 B debug visualization, 004270 the 5 B self-calling slot 11), core/NpPointOnLineJoint.cpp 004597-004621 (13 rows, 967 B, 004619 generated; the 3-row gap NpPointOnLineJoint..NpSphericalJoint is this unit's constructor/thunk/destructor tail). No row shared with point-in-plane. Supplement +0x000a1cc0. Also put core/SphericalJoint.cpp on the /arch:IA32 list (Task 3c left it off). createJoint case 4 wired (0x16c, public object at +0x48); two nxPointOnLineCase cases; 4 oracle lines per joint list, floors 6=43, 7=20. Transcript defects found: 0 (stdout_delta=0 on the first run). Rows deferred: 0. cdb trace: 004276 004617 004615 004268 executed. Ledger: 16 rows moved to reconstructed_not_falsified (counts 201/230). |
| 3e | 2026-09-25T10:46:16 | 2026-09-25T11:05:48 | 19 | 4019 | Point-in-plane (type 5): core/PointInPlaneJoint.cpp 004256-004266 (6 rows, 3,052 B; 004258 is the 1,391 B solver slot -- one kind-1 record along the rotated axis, bias by fdiv, through jointLinearError/jointLinearRecord/jointSolveRecord -- 004260 the 1,273 B debug visualization, point-on-line 004274's instructions except the white line from P and Q's sum grouping), core/NpPointInPlaneJoint.cpp 004567-004595 minus the shared 004573/004577 (13 rows, 967 B, 004593 generated; the unit holds its own constructor/thunk/destructor tail). The 3-row gap NpFixedJoint..NpPointInPlaneJoint is the fixed family's Np tail (004561 called by fixed 004250), left to Task 3h. Supplement +0x000a10a0/+0x000a1650. createJoint case 5 wired (0x16c, type bit 2, public object at +0x48); two nxPointInPlaneCase cases; 4 oracle lines per joint list, floors 6=51, 7=24. Transcript defects found: 0 (stdout_delta=0 on the first run). Rows deferred: 0. cdb trace: 004262 004591 004589 004256 executed. Ledger: 15 rows moved to reconstructed_not_falsified (counts 186/245). |
| 3f | 2026-09-25T11:14:24 | 2026-09-25T11:40:00 | 19 | 4793 | Distance (type 6): core/DistanceJoint.cpp 004230-004240 (6 rows, 3,826 B; 004240 is the 2,747 B solver slot -- a rigid arm (min == max, both limits) with one kind-1 record, else a max arm along -d and a min arm along d with kind-0 records, each through row004393 when the spring flag is set and the jointSolveRecord tail otherwise -- 004232 the 564 B debug visualization, one 0xf0f0f0 line between the world anchors gated only by SDK parameters 32/31), core/NpDistanceJoint.cpp 004511-004535 (13 rows, 967 B, 004533 generated; the unit holds its own constructor/thunk/destructor triple, and 004537 in its range is the generated ~NxJoint body). No neighbouring gap. Supplement +0x0009f1c0/+0x0009f230/+0x0009f560. createJoint case 6 wired (0x184, type bit 0x2000, public object at +0x48; fields maxDistance/minDistance/spring/flags at +0x16c..+0x180); two nxDistanceCase cases printing the saved family fields; 4 oracle lines per joint list, floors 6=59, 7=28. Transcript defects found: 0 (stdout_delta=0 on the first run). Rows deferred: 0. cdb trace: 004234 004531 004529 004230 executed. Ledger: 15 rows moved to reconstructed_not_falsified (counts 171/260). |
| sqrt-fix | 2026-09-25T11:53:56 | 2026-09-25T12:01:00 (estimated; the fix and this row were committed in 0095df0 at 11:59:58) | 0 | 0 | Cross-family fidelity fix, no row state change: all 35 `sqrt()` calls in core/Joint.cpp, RevoluteJoint.cpp, SphericalJoint.cpp, CylindricalJoint.cpp, PrismaticJoint.cpp and DistanceJoint.cpp (the 47 oracle `fsqrt` sites of 004097 004101 004121 004143 004240 004298 004306 004308 004310 004314 004326 004356 004374 004386; no joint row calls a CRT sqrt) now go through the naked x87 helpers in the new core/JointX87.h, which form the listing's sum and take `fsqrt` at the live control word instead of `__CIsqrt` (round to nearest under 0x0f7f). No core object references `__CIsqrt` any more. Joint gate stdout_delta=0; Phase 7 pass; Phase 5 red only on its vtables marker. |
| 3g | 2026-09-25T12:10:55 | 2026-09-25T12:40:00 | 22 | 4492 | Pulley (type 7): core/PulleyJoint.cpp 004214-004228 (9 rows, 3,525 B; 004228 is the 1,572 B solver slot -- per body the lever, world point, unit direction to the pulley and its length, then mBias = (distance - (len0 + len1 ratio)) (stiffness / arg), one kind-6 record with no direction, the crosses and the effective mass with 1/K and 0.7/K -- 004219 the 805 B impulse slot 0 that applies the impulse to the support records itself, 004221 the 592 B visualization drawing each anchor to its pulley, 004214 slot 1, 004218 the separate family-field copy), core/NpPulleyJoint.cpp 004475-004509 minus the five folded bodies (967 B, 004507 generated). Supplement +0x0009e3d0/+0x0009e4e0/+0x0009e810/+0x0009eb00. createJoint case 7 wired (0x1e0, type bit 0x1000). Defects found by the transcript: 0 (stdout_delta=0 first run; 67/67, 32/32). Rows deferred: 0. The 3-row gap NpD6..NpPulley is D6's constructor triple (004210 -> 004469) and is left to Task 3i. Listing over decompile: 004228 stores the lever into lever[1] and reads lever[i] (an original bug, reproduced); the supplement decompile of 004221 passes the first anchor to both addLine calls. No motor in this SDK's NxPulleyJointDesc. |
| 3h | 2026-09-25T12:43:00 | 2026-09-25T13:21:05 | 19 | 4932 | Fixed (type 8): core/FixedJoint.cpp 004242-004254 minus the folded 004248 (6 rows, 3,965 B; 004246 is the 2,922 B solver slot -- no stale-body refresh, three kind-1 linear records from r = R0 * the stored relative position against the .data zero triple 0x10123c1c (now gJointZeroVector) and three kind-3 angular records from E = conj(q0) q1 * the stored relative rotation, sign-flipped when E.w < 0, times -2 inv -- 004244 the 723 B relative-pose row the constructor and loadFromDesc call), core/NpFixedJoint.cpp 004541-004565 (13 rows, 967 B, including the Np constructor/thunk/destructor triple 004561-004565 from the gap NpFixed..NpPointInPlane; 004563 generated). Supplement +0x000a00e0/+0x000a1010. createJoint case 8 wired (0x188, type bit 0x200). Defects found by the transcript: 0 (stdout_delta=0 first run; 75/75, 36/36). Rows deferred: 0. 004248 (folded no-op at slots 0/4/8) left unclaimed. NxFixedJointDesc has no field of its own; the transcript cannot see the relative pose. |
| 3i | 2026-09-25T13:35:21 | 2026-09-25T14:40:00 | 34 | 13753 | D6 (type 9): core/D6Joint.cpp 004178-004212 minus the folded 004186 (17 rows, 12,450 B; the image's __FILE__ is src\D6Joint.cpp, kept in core/ per the pilot's Joint.cpp precedent and repointed from the allowlisted Physics/src/D6Joint.cpp; 004206 is the 3,200 B solver slot -- W0/W1/rel poses through the 004178/004180 pose helpers, locked/limited linear records, locked angular records, twist limit and elliptical swing cone, then a fprintf dump of every call to D6JointDump.txt via 004188-004192 -- 004207 the 2,394 B projection slot with its 7-way locked-angular jump table, 004200 the 2,098 B visualization, 004198 the 1,164 B JwQ matrix), core/NpD6Joint.cpp 004435-004473 minus the folded 004437/004441/004443 (17 rows, 1,303 B, including the Np constructor/thunk/destructor triple 004469-004473 from the gap NpD6..NpPulley; 004471 generated; the four drive setters 004461-004467 call the folded no-op 004248). Supplement +0x0009b550/+0x0009b590/+0x0009cba0/+0x0009e2f0. JointX87.h gains jointFsqrtDot2. createJoint case 9 wired (0x270, type bit 0x4000); all ten types now reconstructed. Defects found by the transcript: 0 (stdout_delta=0 first run; 83/83, 40/40). Rows deferred: 0 (000022 stays a deferred stub, as for revolute/spherical). The oracle's D6 saveToDesc saves only the base part (tail-jump 004066); the test prints sentinel family fields to show it. Solver/projection/visualization/dump rows are unexercised by any transcript (static proof only). |
| 4 | 2026-09-25T14:49:00 (approx.; after ddac64c at 14:48:42) | 2026-09-25T15:10:56 | 0 | 0 | Results and loose ends; no row written. Phase 6 ledger back-fix (0b74ab4): 32 rows that Tasks 1, 2 and 3a moved to `reconstructed` but left at `not_reconstructed_in_phase` now give `reconstructed_not_falsified` (counts 115/316 -> 83/348). Contract sqrt example and cylindrical slot wording corrected (4726728). Cylindrical's duplicate `cylindricalMul`/`cylindricalSdkParameter` replaced by the shared `jointLinearMul`/`jointLinearSdkParameter` (object disassembly identical apart from the two dropped unreferenced copies); `jointCIacos`/`jointAcos` back to plain `static`; Scene.cpp's unreachable size case 8 names 0x188 (52495b5). work_units.json and the 28 joint bundles regenerated (only Joint.cpp and two gap bundles changed). Fresh configure, clean build, gates 2-7, headers, tool tests, stable-ID and `__CIsqrt` checks (see ## Verification). |

The Task 1 row's byte figure (591) is a slip: the 13 folded bodies are 601 B in the inventory, as
`unit-pilot-revolute.md` also records. The tables below use the inventory figure.

## Result

All ten joint types are now built by `Scene::createJoint` through reconstructed translation
units: revolute (the pilot) and the nine families of this plan, cases 0 and 2-9. Each family has
its own `nx<Family>Case` in `tests/PhysicsJointTests.cpp`, and the Phase 6 staged pair
(`NxPhysicsJointStagedPairTests`) compares the whole transcript against the oracle's.

Per task. "Written" is the rows given native source in `Physics/src/core/` (inventory
`implementation`/`source`, stable-ID line in the file). "Moved" is the subset whose inventory
state went `discovered` -> `reconstructed`; the rest were already `reconstructed` through
parameterised ObjectModel models or differentials and only gained the native source and an
appended proof. "Executed" is the rows given a `dynamic_proof` from the family's cdb trace
(`evidence/joint-families-trace-<family>.txt`). Counts are from diffing the committed
`inventory.json` at each task's final commit.

| Task | Family (NxJointType) | createJoint | Written rows / B | Moved rows / B | Executed rows / B | Lines registered |
|---|---|---|---:|---:|---:|---:|
| 1 | shared NpJoint bodies | - | 13 / 601 | 6 / 259 | 0 / 0 | 0 |
| 2 | shared Joint base | - | 12 / 10,506 | 11 / 10,444 | 0 / 0 | 0 |
| 3a | prismatic (0) | case 0, 0x17c | 19 / 8,575 | 15 / 8,259 | 5 / 719 | 4 + 4 |
| 3b | cylindrical (2) | case 2, 0x16c | 19 / 7,850 | 15 / 7,534 | 4 / 282 | 4 + 4 |
| 3c | spherical (3) | case 3, 0x23c | 32 / 18,944 | 22 / 18,305 | 6 / 1,052 | 4 + 4 |
| 3d | point-on-line (4) | case 4, 0x16c | 20 / 4,778 | 16 / 4,462 | 4 / 279 | 4 + 4 |
| 3e | point-in-plane (5) | case 5, 0x16c | 19 / 4,019 | 15 / 3,703 | 4 / 279 | 4 + 4 |
| 3f | distance (6) | case 6, 0x184 | 19 / 4,793 | 15 / 4,477 | 4 / 412 | 4 + 4 |
| 3g | pulley (7) | case 7, 0x1e0 | 22 / 4,492 | 16 / 4,053 | 5 / 489 | 4 + 4 |
| 3h | fixed (8) | case 8, 0x188 | 19 / 4,932 | 15 / 4,616 | 5 / 987 | 4 + 4 |
| 3i | D6 (9) | case 9, 0x270 | 34 / 13,753 | 25 / 13,039 | 9 / 1,563 | 4 + 4 |
| **Total** | | | **228 / 83,243** | **171 / 79,151** | **46 / 6,062** | **36 + 36** |

"Lines registered" is four oracle-copied lines in each of the two joint lists in
`tools/gate_targets.ps1` (`NxPhysicsJointTests`, oracle differential; `NxPhysicsJointStagedPairTests`,
staged pair): created, anchor/axis/state, the type and `is<Family>Joint`, and one family-getter or
saveToDesc line. Floors went from `'6' = 11, '7' = 4` to `'6' = 83, '7' = 40`. No existing expected
line changed; the only removed text in the file is the last revolute line of each list regaining a
trailing comma, and the two floor lines.

Executed rows, by family: prismatic 004380 004753 004378 004751 004376; cylindrical 004320 004675
004673 004316; spherical 004300 004649 004284 004643 004286 004290; point-on-line 004276 004617
004615 004268; point-in-plane 004262 004591 004589 004256; distance 004234 004531 004529 004230;
pulley 004222 004505 004218 004503 004216; fixed 004250 004561 004244 004559 004242; D6 004210
004204 004469 004459 004182 004461 004463 004465 004467. That is each family's constructor chain,
its saveToDesc and a few getters. The other 182 rows (77,181 B) written by this plan are source only:
reviewed against the Capstone listing, built, and on no transcript path. This includes every solver,
projection and debug-visualization slot and every loadFromDesc, setter and destructor. Some of them
carried older proofs as ObjectModel models, and those proofs were kept.

Rows deferred, and why:

- **Task 2** left six Joint rows and five Scene/Actor rows unwritten. The contract's `## Shared
  rows` holds the full classification.
  - 004085 is a folded getName that no joint family calls.
  - 004103, 004105 and 004113 are Scene teardown and break-event dispatch. They need 000557/000633
    and the Scene event flush, which the candidate lacks.
  - 004068 and 004072 are called only by the scene-dump writer.
  - 000022, 000571, 000598, 000633 and 000758 stay asserting stubs in `core/JointSupport.cpp`.
    000598 is new, called by 004093. The solver/projection rows of revolute, spherical and D6 reach
    these stubs on some arms.
- **Task 3a** deferred 004318 (cylindrical's visualization in prismatic slot 4). Task 3b wrote it as
  `Joint::row004318`.
- **Tasks 3b-3i:** none deferred.
- **Left unclaimed:** the folded 004248 (`ret 4`, 19 table slots) and 004186 (the projection-mode
  getter). Both are already `reconstructed` inline in the headers.

Ledger. `gates/phase6-closure.json` now gives `reconstructed_not_falsified` with the ledger's
standard note for all 171 moved rows. Counts are 254/177 before the plan and 83/348 after it;
`differential_falsified` stays at 2.

- Tasks 3b-3i flipped 139 of them as they went.
- Task 4 flipped the other 32, which Tasks 1, 2 and 3a had left at `not_reconstructed_in_phase`:
  - Task 1's six: 004437 004441 004497 004499 004577 004635.
  - Task 2's eleven: 004064 004093 004099 004101 004109 004111 004123 004133 004135 004143 004391.
  - Prismatic's fifteen.

The validator does not require the flip, and it did not ask for anything else.

The other phase ledgers were checked too:

- Phase 3 has 78 rows that are `reconstructed` but still give `not_reconstructed_in_phase`,
  Phase 5 has 3 and Phase 7 has 1.
- All 82 were already `reconstructed` before this plan started (`64d3437`), and the validator
  accepts them.
- This plan moved no row outside Phase 6. The three Phase 5 rows its commits touched (000955,
  000961, 000963) already give `reconstructed_not_falsified`.

They are left as they were. No row is above `reconstructed`.

## Defects found by the transcript

None, in any family. Every family's staged pair gave `stdout_delta=0` on its first run, and so did
Tasks 1 and 2, which re-ran the revolute cases. This still holds after the clean rebuild in
`## Verification`: 83/83 and 40/40.

As in the pilot, the defects came from review against the listing and from the implementers' own
checks, not from the transcript:

- **Pilot: lock links passed by address.** The revolute Np rows passed `&mWord08`/`&mWord04` to the
  `nxNpSceneGuard*` helpers instead of the link values (fixed in `9b124d2`, before this plan). It
  became a Global Constraint, and every family passes the links by value.
- **sqrt, across families.** Every family since the pilot wrote `sqrt()`, which compiles to a
  `__CIsqrt` call. `__CIsqrt` rounds to nearest under the in-step control word 0x0f7f, where the
  oracle's inline `fsqrt` chops. Task 3f flagged it. `0095df0` moved all 35 sites (47 oracle
  `fsqrt`) onto the naked helpers in `core/JointX87.h`. `0637850`, from another session, did the
  same for `Geometry.cpp`.
- **D6 004206 sum order.** The linear-limit arm summed the squares x, y, z where the listing sums
  x, z, y (0x9d222-0x9d23a). Fixed in `ddac64c`, with 004178 now returning `out` in eax as the
  oracle does.
- **Prismatic ledger.** Task 3a did not flip its Phase 6 ledger reasons, and neither did Tasks 1
  and 2. Task 3b noticed; Task 4 fixed it (`0b74ab4`).
- **3c `/arch:IA32` omission.** Task 3c left `core/SphericalJoint.cpp` off the x87 list, although
  the CMake comment named it. Task 3d added it (`bb0af4a`). The transcript could not see this,
  because the spherical rows it drives are not x87-sensitive.
- **Shared-helper drift, fixed in Task 4 (`52495b5`):**
  - Cylindrical kept file-static copies of `jointLinearMul`/`jointLinearSdkParameter`.
  - The moved `_CIacos` helpers had become `NX_INLINE`, a `__forceinline` on an inline-asm
    function.
- **Contract example.** The sqrt rule's example, x = 1.5625 - 2^-63, is not a double; replaced in
  `4726728`.
- **Outside the joint units: Foundation `NxNormalToTangents`.** Task 2's setter experiment found
  that its |n.z| > 1/sqrt(2) arm computes t2.y/t2.z from `n.x * k`, where the oracle's t2 is
  n x t1, and that the other arm is one ulp off in 2 of 8 probes. Not fixed; see open items.

A clean transcript is weak evidence here. The joint test drives only creation, the getters,
saveToDesc and (D6) the drive setters. It uses identity-oriented bodies and never steps the scene,
so 182 of the 228 rows written by this plan never run.

## Oracle quirks reproduced

These behaviours are in the oracle and the reconstruction keeps them.

- **Pulley 004228's lever store.** The solver stores the rotated lever into `lever[1]` and reads
  `lever[i]`. For a dynamic body 0, the world point and first cross therefore come from a stack
  slot the row never writes. The listing's access pattern is reproduced, so the value is the
  candidate's own stack garbage and cannot match the oracle's. A future stepping test with a
  dynamic body 0 is not comparable on that path.
- **D6 saveToDesc is base-only.** 004182 tail-jumps to 004066 and saves only the `NxJointDesc`
  part. The test fills the output with sentinels, and the oracle prints them back.
- **D6 drive setters store nothing.** The four public setters (004461-004467) take the write lock,
  call the folded `ret 4` (004248) and unlock.
- **D6JointDump.txt.** The D6 solver slot opens `D6JointDump.txt` on the first solver step, without
  any flag or gate. It then writes a dump on every step through 004188-004192 into a 100-record
  array with no bound check, and never closes the file. A simulating test with a D6 joint will
  create that file on both sides.
- **Fixed's zero triple.** 004246 uses the .data triple 0x10123c1c, now `gJointZeroVector`, as body
  1's lever. The image initialises it to zero and nothing found writes it, but it was not proved
  that the oracle never does.
- **Smaller shapes:**
  - point-on-line slot 11 (004270) is a virtual call of itself;
  - distance and pulley report through the static `FoundationSDK::error` and have no broken-joint
    test on load;
  - D6's `__FILE__` is `src\D6Joint.cpp`, not under `core\`.

## Rate

**Method.** This is the pilot note's method.

- Rows and bytes moved are rows whose inventory `state` changed, and bytes are those rows'
  `size`. "Written" is rows given native source.
- **Window** hours run from the previous task's final commit to this task's final commit
  (`git log --format='%ci'`). They include the controller's review of the previous task and any
  idle gap before the next brief. D6's window runs to its review fix `ddac64c`.
- **Impl.** hours are the timing table's own start and end. Several of those ends are round numbers
  that fall after the task's last commit, so they are estimates:
  - 3a: 08:23:00, against a commit at 08:22:00;
  - 3d: 10:40:00, against 10:38:53;
  - 3f: 11:40:00, against 11:38:00;
  - sqrt-fix: 12:01:00, against 11:59:58;
  - 3g: 12:40:00, against 12:35:25;
  - 3i: 14:40:00, against 14:32:05.

| Task | Window h | Moved rows/h | Moved B/h | Written rows/h | Written B/h | Impl. h | Impl. written rows/h | Impl. written B/h |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 shared Np | 0.27 | 22.6 | 980 | 49.0 | 2,270 | 0.18 | 71.6 | 3,310 |
| 2 shared base | 0.58 | 19.1 | 18,160 | 20.9 | 18,260 | 0.49 | 24.7 | 21,630 |
| 3a prismatic | 0.71 | 21.2 | 11,660 | 26.8 | 12,100 | 0.56 | 33.9 | 15,280 |
| 3b cylindrical | 0.56 | 26.7 | 13,390 | 33.8 | 13,950 | 0.47 | 40.5 | 16,740 |
| 3c spherical | 0.97 | 22.7 | 18,860 | 33.0 | 19,510 | 0.83 | 38.7 | 22,930 |
| 3d point-on-line | 0.75 | 21.4 | 5,970 | 26.7 | 6,390 | 0.33 | 60.0 | 14,330 |
| 3e point-in-plane | 0.47 | 31.8 | 7,860 | 40.3 | 8,530 | 0.33 | 58.4 | 12,350 |
| 3f distance | 0.51 | 29.2 | 8,710 | 37.0 | 9,330 | 0.43 | 44.5 | 11,230 |
| sqrt-fix | 0.37 | - | - | - | - | 0.12 | - | - |
| 3g pulley | 0.59 | 27.1 | 6,860 | 37.2 | 7,600 | 0.48 | 45.4 | 9,270 |
| 3h fixed | 0.78 | 19.2 | 5,920 | 24.4 | 6,320 | 0.63 | 29.9 | 7,770 |
| 3i D6 | 1.44 | 17.3 | 9,050 | 23.6 | 9,540 | 1.08 | 31.6 | 12,760 |
| **Plan (1-3i)** | **7.99** | **21.4** | **9,900** | **28.5** | **10,410** | **5.92** | **38.5** | **14,050** |

- **The whole plan**, 06:49:03 (`64d3437`, the plan commit) to 14:48:42 (`ddac64c`), is 7.99 h.
  - Moved: 171 rows / 79,151 B, which is **21.4 rows/h, 9,900 B/h**.
  - Written: 228 rows / 83,243 B, which is 28.5 rows/h and 10,410 B/h.
  - The implementer intervals alone add up to 5.92 h, giving 38.5 rows/h and 14,050 B/h. That is
    an upper bound, not the rate to quote.
  - Task 4 adds about 0.4 h and moves no rows.
- **Against the pilot's writing + wiring window** (Tasks 6-10: 17.8 rows/h and 9,750 B/h moved;
  27.0 rows/h and 10,220 B/h written; implementer intervals 27.3 rows/h and 14,980 B/h), the
  families ran at about the same byte rate: 9,900 against 9,750 B/h moved. Their row rate was about
  1.2x the pilot's: 21.4 against 17.8 rows/h.
  - The plan's window also includes its contracts, scaffolds, wiring, tests, inventory and a
    cross-family fix. The pilot's contract and scaffold (its Tasks 4-5) sat outside its quoted
    window.
  - The shared tooling (work units, bundles, supplement script) was already built, so no tooling
    phase is charged here. The pilot's all-in figure, tooling included, was 11.4 rows/h.
- **Per family.** The rates track row mix more than family size:
  - Spherical (18.9 kB/h moved) and the Task 2 base (18.2 kB/h) were dominated by a few multi-kB
    x87 rows.
  - Point-in-plane, distance and pulley had the highest row rates, with the smallest rows and
    Np files generated from the previous family's.
  - D6 was slowest in rows/h. It wrote 34 rows including the 3.2 kB solver slot and the 2.4 kB
    projection slot, and its window includes a review-fix round.

**Caveats.** These are the pilot's caveats, and they still apply.

- **The bar is source plus listing review, not behaviour.** Only 46 of the 228 rows are shown
  executing, and none has a mutation aimed at it: every one is `reconstructed_not_falsified`.
- **Row mix.** Bytes are dominated by large x87 solver, projection and visualization rows that
  nothing runs: 004386, 004326, 004296, 004310, 004312, 004206, 004207, 004200, 004246, 004240,
  004228 and others. 57 of the 228 rows were already `reconstructed` as models.
- **Wall-clock is not effort.** Each family was an implementer subagent followed by reviewer
  subagents and fix rounds. The hours are elapsed time, not agent-hours or tokens. The windows
  include review and idle gaps: for example, 3d's window opens 26 minutes before its implementer
  started.
- **The families reuse the pilot.** They reuse the pilot's shapes: `NpJointShared`, the record
  helpers, and Np files generated by row mapping from the previous family. Later families are
  cheaper for that reason, not only because they are smaller.
- **One sample per family,** with one ready-made transcript harness.

## Open items carried forward

1. (Closed by joint-open-items Task 2: release wired, see `evidence/joint-open-items.md`.) **Release is unwired.**
   - `NpScene::releaseJoint` is empty and `nxSceneAddJoint` (000661) is a no-op, so joints of every
     type live until scene teardown.
   - Every family's release chain (internal deleting destructor -> 004095 -> deferred 000633) is
     written but has never run.
2. (Closed by joint-open-items Task 2: all five written; 000754 and 004167 remain deferred.) **Deferred Scene rows 000022, 000571, 000598, 000633 and 000758 are asserting stubs.** Revolute
   004356, spherical 004298 and D6 004207 reach 000022 on some projection arms. The solver slots
   need 004093's Scene record array (000598), and 004111's break path needs 000571.
3. (Closed by joint-open-items Task 4: every family matches over rotated bodies; the defects were in the candidate's body-record writers and the joint-descriptor exports, see `evidence/joint-open-items.md`.) **Rotated-body conventions are untested.** Every family's test uses the pilot's
   identity-oriented, translated bodies, as the pilot's contract open issue 3 describes. Only the
   identity case of the +0x5c quaternion and +0xdc 3x3 conventions is confirmed. That covers
   004101's frame quaternions, 004378/004244's relative rotations, and the D6 pose helpers.
4. **No simulation-path execution.**
   - Unexecuted rows: the solver slots, projection slots, debug visualization, the impulse slot
     (pulley 004219), the D6 dump rows, and the shared 004064, 004093, 004111, 004123, 004133,
     004135 and 004391.
   - No transcript steps a scene or draws debug geometry, so these rows are checked against the
     listing and by the build only.
   - The first real check for them is a simulation differential. It must expect D6JointDump.txt
     and pulley's uninitialised lever (see `## Oracle quirks reproduced`).
5. **PC64 narrowing at the `core/JointX87.h` helpers.**
   - The mechanism: the helpers take qword arguments. Under the in-step word 0x0f7f, an operand the
     reconstruction holds as an unrounded `double` is narrowed from the 64-bit register value to 53
     bits before the helper uses it, where the oracle keeps all 64 bits.
   - What is exact: float operands, float x float products and constants. Joint.cpp's
     quaternion-from-matrix sites take float matrix elements and a float pair sum, so they are
     unaffected.
   - Under the public API's 0x027f nothing differs. This is the same narrowing that the `double`
     convention accepts at every MSVC spill.
   - The operands concerned, from a pass over every `jointFsqrt*` call in `Physics/src/core/`:
     - distance 004240: `dzUnrounded`;
     - spherical 004306: `sz` (`sx`/`sy` are stored floats) and `cx`/`cy`/`cz`;
     - spherical 004308: `az`;
     - spherical 004310: `qz` and `pz`;
     - spherical 004314: `x` and `y` (fcos/fsin x radius);
     - cylindrical 004326 and prismatic 004386: `sx` and `sz`;
     - revolute 004356: `x` in the quaternion norm (`jointFsqrtDot4`);
     - revolute 004374: `az`;
     - D6 004206: `sumX` in the linear-limit length, and `iq` = 1/q in the swing-cone root;
     - D6 004207: `a`, `b` and `d` in the swing1-locked arm and `a`, `b` and `c` in the
       swing2-locked arm (sums of two floats held as `double`).
   - Not affected:
     - fixed has no square root;
     - pulley's operands are stored floats;
     - D6 004207's `lockedX` is a `double` that only ever holds a float.
   - Nothing on the transcript runs under 0x0f7f, so none of this is observable today.
6. (Closed by joint-open-items Task 4: near-z axes match for every family after main's Foundation fix.) **Foundation `NxNormalToTangents` defect** (Task 2). It changes the local normal that 004101 and
   `NxJointDesc::setGlobalAxis` store for axes near z, and the tangents prismatic's 004386 builds.
   It belongs to a separate Foundation task. The joint tests avoid such axes.
7. **Phase 2/3 test targets.** (Build closed by joint-open-items Task 1; Phase 3 red on three
   registered counts 0637850 moved, see `evidence/joint-open-items.md`.)
   - `NxPhysicsInternalTests` and `NxPhysicsCollisionTests` still fail to compile (`IcePrunable.h`),
     a failure that predates the pilot.
   - Both compile `Scene.cpp`, which now constructs every joint type through `Physics/src/core`. Once the include path is
     fixed, they will also need `Physics/src/core/*.cpp` at link time.
8. (Closed by joint-open-items Task 3: 000797 stores 0, only the step's 000611 writes non-zero; see joint-open-items-contract.md `## Body record +0x204`.) **Body +0x204 is unbuilt** (revolute contract open issue 7). The candidate's body record never
   writes the `JointSupportBody*` that the solver-slot rows read; every family's internal file
   reads it through the body record.
9. (Closed by joint-open-items Task 2: 000661 writes it.) **`Joint::mScene` (+0x30) is never written.** 000661 is a no-op, so `~Joint`, 004107's false
   branch and the break paths read uninitialised memory there. Whoever implements 000661/000633
   must write it.
10. (Closed by joint-open-items Task 5: the joint files build with `/EHs-c-`.) **SEH/GS frames on the candidate's deleting destructors.**
    - The pilot recorded this for 004729. A map/disassembly check after this plan's clean build shows
      the same `push -1; push <handler>; mov eax,fs:[0]` and cookie prologue on every candidate
      `??_G`/`??_E` of the nine family classes, their Np classes and `Joint`.
    - The oracle's copies are frameless (`push esi; mov esi,ecx; ...`). Examples: 004382, 004757,
      004322, 004679, 004202, 004473, 004252, 004565, 004224, 004509, 004236, 004535, 004264,
      004595, 004278, 004621, 004653, 004368, 004729.
    - The generated `sub ecx,0Ch; jmp` thunks match. The cause is still uninvestigated, probably the
      EH/GS settings of these translation units, and none of these destructors has run.
11. (Closed by joint-open-items Task 1: path removed.) **The generic `createJoint` path**
    (`nxJointConstruct`, `nxJointSizeForType`) is now reachable
    for no type. Its size table keeps the stand-in's literals, with comments naming the real sizes
    for types 8 and 9 only.
12. (Closed by joint-open-items Task 1: folded into X87Sqrt.h.) **Two sets of naked sqrt helpers.** `Physics/src/include/core/JointX87.h` (joint files) and
    `Physics/src/include/X87Sqrt.h` (Geometry.cpp, commit 0637850) hold equivalent helpers;
    `jointFsqrtDot2` has the same body as `x87FsqrtDot2`. X87Sqrt.h notes that folding JointX87.h
    into it is a rename only. Not done here, to keep the joint transcript's reviewed code stable.

## Verification

Fresh configure and clean build, run 2026-09-25T15:00:49 to 15:01:41 on the tree at `52495b5` (all
product changes of this plan and Task 4's loose ends):

```
cmake -S . -B build -A Win32 --fresh                                   exit 0
cmake --build build --config Release --target NxPhysics --clean-first  exit 0
  warnings: 23 x C4005 'ARRAYSIZE' (winnt.h vs Ice/IceUtils.h), 8 x C4291 (FoundationSDK.cpp,
  PhysicsInternal.cpp, PhysicsSDK.cpp, Scene.cpp), 2 x D9025 (NxOpcode /EHc /EHs); none from
  Physics/src/core
```

Gates, run on that build (`run_phase_gate.ps1 -Phase N`):

```
public_headers=pass (both roots, every gate); git diff 64d3437 -- Physics/include Foundation/include: empty
Ran 643 tests ... OK
validate_inventory.py exit 0: inventory=pass, closure phase=6 closed=2 deferred=431, unexplained=0
phase 2 exit 1   GATE FAILED: build_physics exited 1
                 ObjectModel.h(11,10): error C1083: Cannot open include file: 'IcePrunable.h' [NxPhysicsInternalTests.vcxproj]
phase 3 exit 1   GATE FAILED: build_physics exited 1
                 ObjectModel.h(11,10): error C1083: Cannot open include file: 'IcePrunable.h' [NxPhysicsCollisionTests.vcxproj]
phase 4 exit 0   coverage_assertions_evaluated=101 floor=101 / phase_gate=4 status=pass
phase 5 exit 1   candidate CANDIDATE-MISSING family=vtables reason=shape finals/actor classes are Tasks 3-4
                 gate_failure=oracle_differential:NxPhysicsObjectLayoutTests exited 1 (coverage_assertions_evaluated=829 floor=829;
                 all 12 staged-pair differentials stdout_delta=0)
phase 6 exit 0   differential target=NxPhysicsJointStagedPairTests oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
                 coverage_assertions_evaluated=83 floor=83 / phase_gate=6 status=pass
phase 7 exit 0   differential target=NxPhysicsJointStagedPairTests oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
                 coverage_assertions_evaluated=40 floor=40 / phase_gate=7 status=pass
```

- **Phases 2 and 3** fail at `build_physics`, as they already did at the pilot's starting commit
  (open item 7). This plan does not fix that.
- **Phase 5** fails only on its existing RED-on-purpose `CANDIDATE-MISSING family=vtables` marker.
- **Stable-ID form.** Every line in `Physics/src/core/*.cpp` matching `^\s*// phys_fn_` fullmatches
  `\s*// phys_fn_\d{6} \(0x[0-9a-f]{8}, \d+ B\)`: 306 lines, 0 malformed, 0 duplicates. Each line's
  RVA and size equal the inventory row's.
- **No CRT sqrt or acos in core/.**
  - `dumpbin -symbols` over the 23 `Physics/src/core` objects finds no `__CIsqrt` or `__CIacos`
    reference. The only acos symbol is the file-static `jointCIacos` in D6Joint.obj,
    RevoluteJoint.obj and SphericalJoint.obj.
  - A scan of `build/Release/NxPhysics.dll` for `call rel32` to `__CIsqrt` (0x100d17be) and
    `__CIacos` (0x100d1848), attributed through `NxPhysics.map`, finds callers only in
    NxOpcode (Ice*, OPC_*) and NxQhull objects. No `Physics/src/core` object and no `Geometry.obj`
    calls either.
- **Task 4's code changes.**
  - `CylindricalJoint.obj` disassembles identically before and after the helper swap, apart from
    the two dropped unreferenced COMDAT copies.
  - After the acos change, `RevoluteJoint.obj` and `SphericalJoint.obj` call `jointAcos` out of
    line again (three call sites each), as the pilot's `revoluteAcos` was.
  - Joint gate `stdout_delta=0` before and after.
