# Actor mass from shapes (actor-mass Task 1)

The core-dump differential (evidence/effector-and-coredump.md, Task 4) found that the candidate
had no mass from shapes. phys_fn_000008 was unwritten, so a dynamic actor created with a density
and a sphere, capsule or compound got mass 0, a box with a local pose got no centre-of-mass
frame, and a dynamic actor whose only shape is a trigger was accepted where the oracle refuses
it. This task writes 000008 and what it reaches, and tests it against the oracle. Contract:
units/actor-mass-contract.md. Trace: evidence/actor-mass-trace.txt.

## Rows

* **000008** (0x10a0, 751 B), Actor::computeMass, new in `Physics/src/core/ActorMass.cpp`
  (/arch:IA32): the zeroed frame, the root shape's slot 4 at unit density, return 1 or 2 on
  failure, the pose position from the frame's offset, 000841, one of three scaling arms, and
  `NxDiagonalizeInertiaTensor`. `discovered` -> `reconstructed` (Phase 5 ledger: 85 -> 84 not
  reconstructed, 120 -> 121 `reconstructed_not_falsified`).
* **000841** (0x1c720, 43 B) and **001024** (0x229b0, 87 B, the compound's slot 4) get product
  rows in the same file. The candidate's 0x110-byte group had no table, so 000008 could not reach
  a compound's children. It now gets the compound table (0x10106c2c) with the slots the candidate
  has rows for.
* Wired into the candidate model of 000026 (`nxActorComputeMass`): the body-descriptor copy, the
  integer-zero massSpaceInertia test, and 000008 before the record is allocated. The one-box
  density formula that stood in for all of this is gone.

## Defects found

The per-shape rows were `reconstructed`, and until now nothing drove them through actor creation.
With a density-based actor of each shape in front of the oracle, four existing rows and one
creation path were found wrong, each confirmed against the listing:

1. **000829** (box builder): the ⅓·mass factor and the three pairwise sums of squared extents
   are stored as 32-bit floats before the products. The earlier transcription kept them on the
   stack. This is the "small cube an inertia one ulp off" the core dump had worked around.
2. **000845** (capsule builder): on selector 1, which is the capsule's slot 4, +0x00 does get
   the transverse moment. The store at 0x1c836 precedes the branch. The earlier transcription
   left the word unwritten.
3. **000833** (translate): retranscribed op for op. The earlier version did both paths with one
   formula and rounded in different places. **000849** now calls it as the listing does,
   instead of a centred-box substitute. The Phase 2 mftranslate (54/54) and negtrans (5/5)
   differentials still pass.
4. **createActor** validated through the non-virtual `NxActorDescBase::isValid`, so it never
   ran isValidInternal. A density with an explicit mass was accepted where the oracle refuses
   it. It now applies the whole NxActorDesc predicate to the two shape-list descriptor types.
5. **Reports.** The mass failures and createActor's refusals were `printf`s. They now go
   through `FoundationSDK::error` with the oracle's `__FILE__` and line (Actor.cpp 0xe5/0xe6,
   Scene.cpp 0x203/0x209/0x21b/0x228). A failed load now releases the shapes and name it built
   (the failed-creation half of 0x1c40), so the next actor gets the same ids as on the oracle.

The merge row 000839 was checked against the listing and is unchanged. 000164
(updateMassFromShapes, the other caller of 000008) is not reconstructed. Through createActor,
000008's arm for a positive density with a positive mass cannot be reached.

## Tests

* **`NxPhysicsActorMassTests`** is new and registered in Phase 5. It covers a sphere, a box, a
  small cube and a capsule, each centred and with a rotated and shifted local pose; a posed
  three-part compound, with and without a trigger child; the mass-only arm on a compound and on
  a capsule; and three refusals: density with mass, a trigger-only sphere and a trigger-only
  compound. Each created actor prints getMass, getMassSpaceInertiaTensor and getCMassLocalPose
  as words. Result: `stdout_delta=0`. All 36 oracle lines are registered.
* **`NxPhysicsCoreDumpTests`** gains **scene D**, dumped after scene C in its own pointer epoch.
  It has scene A's dynamic bodies and scene B's pair with densities, both refusals with their
  reports, and a revolute limit plane over two computed frames, dumped as text and binary.
  Result: `stdout_delta=0`, with 23 new oracle lines registered (the other 5 were already
  registered verbatim).
  * The dump file is byte-identical with density bodies. Changing scenes A-C to densities was
    also tried: it matched byte for byte, but 20 of their registered lines changed, and the
    Global Constraints forbid editing a registered line. So scenes A-C keep their explicit
    masses and scene D carries the density bodies.
* **Floors:** Phase 5 871 -> 907, Phase 6 855 -> 878, Phase 7 728 -> 751
  (`test_gate_targets.py` and `test_gate_commands.py` updated).

## Verification

Fresh Release build. Gates:

* Phases 2, 3, 4, 6 and 7: `status=pass`.
* Phase 5: fails only on `candidate CANDIDATE-MISSING family=vtables`, as before. All 13 staged
  pairs are at `stdout_delta=0` with 907/907 coverage assertions.
* `validate_inventory.py`: exit 0. Tool tests: 753 OK. Stable-ID form check: 0 violations.
  Public headers: unchanged (the gates verify them).

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T11:46:00 | 2026-09-28T12:45:00 (approx.) | 3 new product rows (000008, 000841, 001024) + 3 corrected (000829, 000833, 000845) + 000849's call | 751 + 43 + 87 = 881 new; 187 + 1371 + 181 + 101 corrected | Fast-forwarded to d628181. Defects found by the transcript: 000829 rounding, 000845 selector-1 store, 000833 paths, createActor isValidInternal, printf reports, no shape release on a failed load. New target NxPhysicsActorMassTests (36 lines); core-dump scene D (23 lines). cdb trace: 12 rows gain or extend `dynamic_proof`. |
