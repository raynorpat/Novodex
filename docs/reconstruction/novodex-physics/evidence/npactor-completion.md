# NpActor.cpp completion

Measurement note for the NpActor.cpp completion plan
(`docs/superpowers/plans/2026-09-28-npactor-completion.md`), which closes the public actor API
unit: 87 code rows (evidenced span 0x2610-0xb100), 53 of them (30,673 B) still `discovered`
after the Phase 5 packets implemented and tested most of them against the oracle without
promoting them. The contract is `units/npactor-contract.md`; the execution evidence is
`evidence/npactor-trace.txt`. Each task appends one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T08:00:00 (approx.; the configure log is stamped 08:01:22) | 2026-09-28T08:23:50 | 0 | 0 | Audit, contract, trace; no source change, no inventory change. Actor table phys_data_000679 (0x10104530) read from the image and mapped to `NxActor.h` order (setStatic is commented out; slots 63/64 are the inline getPointVelocity pair in the gap; word 87 is 000116, the member table's thunk). All 87 rows reviewed against their Capstone listings in seven passes. The 53 discovered rows: 37 implemented, 14 partial, 2 missing (000122 setDynamic, 000164 updateMassFromShapes, which also needs the unreconstructed 000008); 1 faithful (000128), 52 defect (4 H1-only, 13 E1-only, 4 S1, 29 substantive, 2 missing). Cross-cutting: G1 lock-failure report absent everywhere; E1 precondition reports absent; H1 the shared dirty-mark helper drops marks on an unallocated list or body id >= 256; NpActor.cpp is compiled SSE (not on the /arch:IA32 list). The inventory's "partial" 000196-000202 reduce to G1/S1/H1. 000216 is 000214's tail. cdb trace of the 12 Phase 5 actor targets (85 breakpoints, candidate sha256 fb64a931...): 50 of the 53 hit; 000122, 000164 (no case calls them) and 000216 (no function of its own) not. Gates 2, 3, 4, 6, 7 pass; Phase 5 red only on its CANDIDATE-MISSING family=vtables marker, 12/12 actor staged pairs stdout_delta=0. |
| 2 | 2026-09-28T08:36:00 (approx.; the baseline Phase 5 gate finished 08:46:07) | 2026-09-28T09:11:32 | 66 | 30196 | Cross-cutting pass (commit 22134a8). G1: 46 write-guarded rows report the lock failure (kind 2, row line, 0x10104760) through `nxNpActorWriteTry` and return without unlocking; 000088 setName now takes the write lock. E1: 51 reports (kind 1; first written as 52, recounted in Task 3) in each listing's order (readers inside the read lock, writers before the unlock, 000170/000172 value first, 000204-000208 before the lock, 000142 lockless, 000070 `desc.isValid()`, 000126 its own lock and check); every message and the file string found byte for byte in the PE; `FoundationSDK::getInstance().error` gives the oracle's inline `cmp [instance],0; int3` before the import call (disassembled). H1: `nxNpActorMarkRecordDirty` rewritten from the inline pushBack (no null/id tests, 2n+2, `nxFoundationSDKAllocator`, no `if(!grown)`); 000785/000787 marks before its 0x20-byte malloc/free through the Foundation allocator, unconditional 1/m (root refresh still open). SSE: NpActor.cpp on the NxPhysics /arch:IA32 list; 000150/000152 rewritten from the listing. Rows written counts the 66 unit rows whose code or verdict changed (65 functions and 000216, bytes = their oracle sizes) and excludes the 000785/000787 helper (1,753 B). Verdicts now: 20 of the 53 discovered rows faithful, 22 of the 34 reconstructed. Tests: 231 setter lines (static/kinematic/invalid E1s, G1 on every guarded row with the lock flag held by owner 0), 12 body-flag lines (null-list growth 2/6/14, 000785 allocation order), 8 force lines (000150); floor 5 = 1122. The Task 1 candidate differs in 379 setter lines, crashes on the null list and misses x87_rotate_5 by one bit. Existing Phase 5 transcripts byte-identical; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0; tools suite 753 passed. No inventory change. |
| 3 | 2026-09-28T09:20:00 (approx.; the first listing dump is stamped 09:23:56) | 2026-09-28T10:12:14 | 39 | 17718 | Row-level defects (commits d6a84ba, cf5b3c0, 123b88c, ebfd183, 725cfd2, 7666f0c). (a) 000204-000208: 000756 = the 000801 conversion, 000789 from the listing (000746 first, displacement row 0 unrounded, setter conversion), the 000004 shape update as a virtual slot-6 dispatch with the group arm modelling 001018 (group-level 001315 open); quaternion roots through X87Sqrt.h. (b) 000784 (no null test, ORs 1/2, wake); 000124/000126 compose the mass-frame target from the listing; 000090 adds +0x100. (c) the 000174/000176/000180/000182 wakes against +0xd0/+0xd4; 000182 row orders. (d) 000168 `_fpclass` & 0x207 on the float inverses. (e) 000060/000742 order; W = R F orders 134/138/140/144 and the 000746 model in 000134/000140/000142/000144. (f) 000785/000787 000712 root refresh; read locks on the NG readers; 000094 static-arm setter conversion; 000782 and 000791 from the listing (mode > 4 wakes). Task 2 leftovers: stable-ID lines, setName null test, E1 count 51. Rows written counts the 39 unit rows whose code changed (bytes = their oracle sizes); the helper rows outside the unit (000756, 000789, 000784, 000782, 000791: 5,915 B) are not counted. Verdicts: 42 of the 53 discovered rows faithful (7 S1-residual, 2 X in Task 4, 2 M in Task 5), 30 of the 34 reconstructed. Tests: 446 oracle lines (CMass 286, Dynamics 21, Momentum 93, DynamicSetter 23, BodyFlag 5, Force 18), each group falsified against the previous commit; floor 5 = 1568. Existing Phase 5 transcripts byte-identical; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0; tools suite 753 passed. No inventory change. |
| 3 review | 2026-09-28T10:20:00 (approx.) | 2026-09-28T10:34:02 (5baa5c1) | 2 | 1394 | 001315's pose composition rewritten from the listing (owner rotation from +0x24 with the five spills, permuted operand orders, translation row 0 unrounded); CMass shape poses printed and registered as exact words (48 rounded lines replaced by 48 exact lines, all matching); 000128's fld/fstp x/y copy (SNaN case); a Force case falsifying 000782's modes 0/1 unrounded terms; the 500 -> 446 line count. Rows written: 001315 (1,061 B, Shape.cpp) and 000128 (333 B). Floor 5 = 1571; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0, no existing registered line changed; gates 2, 3, 4, 6, 7 pass; tools suite 753 passed. |
| 4 | 2026-09-28T10:37:00 (approx.; the first listing dump is stamped 10:39:46) | 2026-09-28T11:20:06 (78b7f7c; this timing row is committed after it) | 9 | 4755 | Shape add/remove (commits 370aca7, 2b4bcb1, 78b7f7c). 000070 calls Actor.cpp 000036 on the body and returns [shape+0x9c]; 000072 calls 000024 with [NxShape+8]. Scene.cpp (the Actor.cpp equivalent) gains the chain from the listings: 000036 (reentry 0x150; empty-root install through 000531; group append 001041 + slot 6 + 001941 + 000503 + 003628; promotion: 000535/000533, the 0x110 group through [0x101041bc], 001033 with a 000012 id, +8, 001041 x2, 000531), 000024 (reentry 0x186, E1 0x18f/0x19c/0x19d/0x1a4, 001028 swap-remove, 000006 and the deleting destructors), the factory 000032 (family sizes 0x10c/0xe4/0x228/0xec, id first and returned on failure, handle lock links, +8), the runtime base init/destroy (001273, 001323), the group as its own object with a table (001033, 001041, 001028, 001018, 001032/001037/001039), Scene 000531/000533/000535/000006 and 000503 (rewritten), and the pruning rows 001941/001943/001945/001955/001957/001960 plus 003628 over the candidate pruner model (per-prunable insert/erase, growth at the fifth prunable, kinds, root counts). The group table closes S1 for 000196-000208 (000004 now reaches the group-level 001315). A shapeless dynamic actor registers its record. Rows written: the 9 unit rows whose code changed (000070, 000072 and the seven S1 rows; bytes = their oracle sizes); the 25 chain rows outside the unit (3,994 B) are not counted. Not reproduced: the mesh arm of 000032, OPCODE pruner internals (entry order), 001943/001955/000517/004861 pieces the candidate keeps no data for, the reentry flag (no callback), the 001315 +0xa0 append and slot-3 arms (unreachable from the add paths). Verdicts: 51 of the 53 discovered rows faithful (2 M in Task 5), 30 of the 34 reconstructed. Tests: 136 oracle lines in ShapeMutation (125 absent from the Task 3 candidate, which crashes mid-block); floor 5 = 1707. Every other Phase 5 target byte-identical to the oracle; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0; tools suite 753 passed. No inventory change. |
| 5 | 2026-09-28T11:30:00 (approx.; the first listing dump is stamped 11:34:10) | 2026-09-28T12:24:35 (0e316b7; this timing row is committed after it) | 2 | 2607 | Mass from shapes and setDynamic (commits 65e3add, 5df7040, 0e316b7). 000164 from the listing (G1 0x98; E1 0x9a/0x9d/0x9e/0x9f/0xa0/0xa8/0xa9; the record writes, marks and ++0x198 in order; _fpclass-zeroed inverses; 000768) over Actor.cpp 000008 (Scene.cpp: 000847, the root's slot 4 at unit density, 0x1c720, the three scaling arms with the register ratio, NxDiagonalizeInertiaTensor). Slot-4 audit: box/sphere/capsule/plane tables reach 000947/001371/001008/001249; the group table gets slot 4 = 001024, whose model now dispatches thiscall; 000845 selector 1 writes +0x00 (fixed); 001397 unreachable (no runtime mesh family). 000122 from the listing (G1 0x5b; E1 0x63/0x66/0x7c/0x7d; 000533, 000026, 000632 + notifyObservers + 000776 + free, 000531). Chain rows written in Scene.cpp/Joint.cpp/JointSupport.cpp: 000026 (replaces the creation path's one-box density approximation), 000030 (actor destructor; releaseActor 000628 rewritten on it, and the creation failure path), 000630, 000632, 000557, 004103, 000722, 000776, 000797, 000799 and the mass block of the 000793 model; the creation group arm is 000034's (001033 + 001041 pushes) and a static group runs its slot 6; creation failures report through the Foundation (Actor.cpp 0xe5/0xe6, Scene.cpp 0x228); the reentry flag 0x10123c10 is one variable. Rows written: the 2 unit rows (bytes = their oracle sizes); the 12 chain rows outside the unit (3,114 B, excluding the 000793 model and the 001024/000845 fixes) are not counted. Verdicts: 53 of the 53 discovered rows faithful; 30 of the 34 reconstructed. Tests: 110 oracle lines in ShapeMutation (99 absent from the Task 4 candidate's transcript); floor 5 = 1817. Every existing Phase 5 transcript byte-identical; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables (candidate_fold 4492c8c1), 12/12 staged pairs stdout_delta=0; tools suite 753 passed. No inventory change, no cdb trace. |
| 5 review | 2026-09-28T12:30:00 (approx.; after the Task 5 timing row at 12:25:00) | 2026-09-28T12:55:05 (fb71a41) | 4 | 1702 | Task 5 review (commit fb71a41; this row was added in Task 6). 000833 transcribed from its listing (0x1c040-0x1c598) and checked word for word against the oracle rows in-process; 000849 calls 000833 instead of its inlined centred specialization; 000829 rounds mass/3 and the three pairwise sums to float; SNaN moves through the x87 in 000793's mass, 000030's position and 000841's negations. Rows written: 000833, 000849, 000829 and 000841 (chain rows outside the unit, already `reconstructed`). Tests: 32 oracle lines (inexact translations through updateMassFromShapes and creation; setDynamic with a quiet and a signalling NaN mass); floor 5 = 1849. Gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0; tools suite 753 passed. |
| 6 | 2026-09-28T12:58:00 (approx.; the first build log is stamped 13:01:15) | 2026-09-28T13:19:13 (a2803e0) | 0 | 0 | Final trace (evidence/npactor-trace-final.txt): 138 breakpoints over the 12 Phase 5 actor targets, candidate sha256 5c2e247f.... All 53 discovered unit rows execute (000122 14 hits and 000164 48, unreached in Task 1; 000216 through 000214's epilogue); 000102 now runs (7); every chain row the plan wrote as faithful executes, four of them (000756, 000799, 001037, 000829) only inlined in a named caller. Four Scene.cpp helpers made `__declspec(noinline)` so a breakpoint sees them (001028, 001032, 001941, 000535; their out-of-line copies had no caller), stable-ID lines for 000128 and 000216, the 000947 comment. No row unexecuted, so no case added: floor 5 stays 1849. Phase 5 12/12 staged pairs stdout_delta=0 after the change, red only on CANDIDATE-MISSING family=vtables (candidate_fold 4492c8c1); gates 2, 3, 4, 6, 7 pass. No inventory change. |
| 7 | 2026-09-28T13:19:13 (after a2803e0) | 2026-09-28T13:37:28 (648946f; this timing row and the results below are committed after it) | 0 | 0 | Promotion (commit 648946f). 85 rows to `reconstructed`: the 53 discovered NpActor.cpp rows (30,673 B; the unit is 87/87) and 32 chain rows the plan wrote as faithful (12,219 B), each with implementation, static_proof (contract verdict and covering targets) and dynamic_proof (the trace); 8 already-reconstructed rows the plan changed get appended evidence. Ledgers: reconstructed_not_falsified for the promoted rows (Phase 5 63, Phase 2 5, Phase 3 9, Phase 6 1, Phase 7 7). Fresh configure and clean build: the DLL differs from the Task 6 incremental build only in its link timestamp, so the trace was re-recorded on it (sha256 ccae6021..., identical offsets and hit counts). work_units.json and the NpActor.cpp bundle regenerated. 18 in-scope rows left discovered, each with its reason in the contract. Validator pass; tools suite 753 passed; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0. No product code change. |
| final review | 2026-09-28T13:55:00 (approx.; the first NpActor.cpp edit is stamped 14:03:46) | 2026-09-28T14:45:00 (approx.; commit 6112710 and the records commit after it) | 24 | 17521 | Final-review fixes. I1 (6112710): NpActor.cpp's x87 float copies (fld/fstp, which quiet an SNaN) made bit copies where the listings move words with rep movsd/mov (000204, 000208, 000210, 000214/000216, 000192, the 000124/000126 inputs, the 000218/000222 input, the getters 000096, 000100, 000130, 000134); nxNpActorX87Dot3 takes references (the listing's in-place fld/fmul; 000124, 000126, 000134-000144, 000789); 000124 adds pose.t from memory; the x arm of both matrix-to-quaternion conversions subtracts (x87FsqrtDiffSum) instead of adding a negation (000094, 000124, 000126, 000196, 000200, 000756, 000789). 13 ActorCMass SNaN lines (12 differ on the previous candidate); floor 5 = 1862. I2: 000628 demoted to discovered (its 003635 fluid arm needs the unwritten 003485/003593/003622 and the stubbed createFluid); Phase 7 ledger 356/201. Trace re-recorded (sha256 9dadfcea...; only ActorCMass changed); 91 dynamic_proofs re-pinned with the new counts, 24 static_proofs note the fix. Rows written: the 25 rows whose code changed, bytes = their oracle sizes. Validator pass; tools suite 753 passed; gates 2, 3, 4, 6, 7 pass; Phase 5 red only on CANDIDATE-MISSING family=vtables, 12/12 staged pairs stdout_delta=0. |

## Result

Task 7 (commit 648946f) moved 85 rows to `reconstructed`; the final review sent one of them,
000628, back to `discovered` (I2, below), so 84 stand. Each carries a `static_proof`: the
contract verdict, its summary, and the staged-pair targets that cover the row. Each also carries a
`dynamic_proof`: a cdb breakpoint hit in `evidence/npactor-trace-final.txt` (candidate sha256
9dadfcea..., re-recorded for the final-review fixes; first ccae6021...), with all twelve Phase 5
actor staged-pair transcripts byte-identical to the oracle.

| Set | Rows | Bytes |
|---|---:|---:|
| NpActor.cpp unit: every `discovered` row, so the unit is now 87 of 87 `reconstructed` | 53 | 30,673 |
| Chain rows the plan wrote in other units: Actor.cpp and the gap before it, Scene.cpp, the SceneRaycast..CapsuleShape gap, the group and pruning gaps, Joint.cpp | 31 | 11,978 |
| Total | 84 | 42,651 |

- **The chain rows:** 000006, 000008, 000024, 000026, 000036, 000531, 000533, 000535,
  000630, 000632, 000722, 000746, 000756, 000782, 000784, 000785, 000787, 000789, 000791, 000799,
  001018, 001028, 001032, 001033, 001037, 001039, 001041, 001279, 001941, 001945 and 004103.
- **Inlined rows.** These run only inlined, so each proof names the caller whose hit shows the row
  ran:

  | Row | Caller named in the proof |
  |---|---|
  | 000756 | 000204 and 000208, before their 000789 call |
  | 000799 | 000776's candidate |
  | 001037 | 001039's candidate |
  | 000829 (an appended row) | 000849's candidate |

  In Task 6, four Scene.cpp helpers were made `__declspec(noinline)` so that a breakpoint sees them
  run, as the oracle calls them: 001028, 001032, 001941 and 000535.
- **Fluid-manager arms.** 000036, 000531, 000533 and 000535 carry their 003628 call. 000628's
  003635 call was never written; Task 7 promoted the row with the arm documented as unreachable, and
  the final review demoted it (see `## Final review fixes`).
- **Appended evidence.** Eight rows were already `reconstructed` and had their candidate changed by
  the plan: 000557, 000829, 000833, 000845, 000849, 000947, 001024 and 001273. Each got appended
  evidence. 000845's old proof said selector 1 leaves +0x00 unwritten; that claim is now marked
  superseded.

**Ledgers.** Each promoted row is `reconstructed_not_falsified` in the closure ledger of the phase
that owns it:

| Phase | Rows | Reason before |
|---|---:|---|
| 5 | 63 | not_reconstructed_in_phase |
| 2 | 5 | homeless_shared_code |
| 3 | 9 | not_reconstructed_in_phase |
| 6 | 1 | not_reconstructed_in_phase |
| 7 | 6 (7 at Task 7; 000628 is back to not_reconstructed_in_phase) | not_reconstructed_in_phase |

No mutation was aimed at any of these rows, so none of them is closed.

## Rows left

These rows were in scope and are still `discovered`. `units/npactor-contract.md` (`## Task 7`) has
the details.

**Unreproduced parts that the product can reach:**

| Row | Size | What is not reproduced |
|---|---:|---|
| 000030 | 403 B | the actor is freed directly rather than through 000118; no 000521 -> 000517 pair pass |
| 000503 | 232 B | 004861's per-pruner slot-4 call |
| 000776 | 117 B | the record vptr store and the Observable destructor |
| 000797 | 402 B | the vptr, the Observable constructor and 000801's sub-object |
| 001315 | 1,061 B | the pruner slot-3 call is modelled; the +0xa0 append is not reproduced; the candidate adds a scene-null guard |
| 001323 | 182 B | 000517's pair pass and 002406 |
| 001943 | 270 B | the cached +0x2c object |
| 001955 | 153 B | the pair-record loop |

**A dependency not written:** 000628 (241 B), Scene::releaseActor. Its fluid-manager arm
(0x1248c-0x12497) calls 003635, which needs 003485, 003593 and 003622; none is written, and Scene
+0x61c is set only by the stubbed createFluid (000645/000400).

**Reachability not established:** 000032 (539 B). Its triangle-mesh arm is not reproduced.
- A valid mesh descriptor cannot exist in the product.
- createActor validates with `NxActorDescBase::isValid()`, which runs no shape loop, so an invalid
  mesh descriptor might still reach the factory.

**Models:**
- 000034 (565 B), the creation path;
- 000793 (1,613 B);
- 000795 (3,090 B);
- 000801 (741 B);
- 001397 (104 B), the mesh table's slot 4;
- the OPCODE pruner rows 004852, 004857 and 004859 (415 B).

**Not reviewed as a row:** 000768 (1,164 B). Only its calls from 000196-000202 were checked.

Outside the promotion's scope, four of the unit's already-`reconstructed` rows keep their defect
verdicts:
- 000086 and 000088: the name table is not 000454/000480's;
- 000116 and 000118: there is no member table and no destructor.

## Defects found and fixed

Every change was checked against the Capstone listing and driven by staged-pair cases registered
from the oracle. Each case was falsified against the previous commit's candidate.

| Area | Commits | What was wrong |
|---|---|---|
| Cross-cutting | 22134a8 | See the list after this table. |
| Row level | d6a84ba, cf5b3c0, 123b88c, ebfd183, 725cfd2, 7666f0c; review bd53f6e, 5baa5c1 | See the list after this table. |
| 001315 pose composition | bd53f6e | The owner rotation came from +0x5c without the five spills, and the operand orders were not the listing's. The shape poses were one bit off; they are now printed and registered as exact words. |
| The shape chain | 370aca7 (cases in 2b4bcb1) | createShape returned 0 for every append. releaseShape shifted entries where the oracle swap-removes, and keyed on the public pointer. See the list after this table for what was written. |
| Mass from shapes and setDynamic | 65e3add, 5df7040 | 000164 and 000122 were empty. See the list after this table for what was written and fixed. |
| 000833/000829/000849 | fb71a41 | 000833 was a provisional model and is now transcribed. 000849 inlined a centred specialization that rounded differently. 000829 kept mass/3 and the sums in the register. SNaNs were not quieted in 000793, 000030 and 000841. |
| Trace visibility | a2803e0 | Four helpers were inlined into conditional arms; they are now noinline. 000128 and 000216 had no stable-ID line. 000947 had a "provisional" comment. |
| SNaN copies and NaN selection (final review I1) | 6112710 | Under `/arch:IA32` the candidate copied floats through fld/fstp (quieting SNaNs) where the listings move words; by-value dot-product operands were quieted before the multiply; 000124 loaded pose.t first; the conversions' x arm negated a NaN. See `## Final review fixes`. |

**Cross-cutting (22134a8):**
- G1: 46 write-guarded rows skipped silently on a failed write-lock try. The oracle reports kind 2
  with the row's line.
- E1: 51 precondition reports were missing.
- H1: the dirty-mark helper dropped marks on an unallocated list or an id >= 256, and used the wrong
  allocator. 000785/000787 allocated before marking.
- x87: NpActor.cpp was compiled SSE. It is now on the `/arch:IA32` list, and 000150/000152 were
  rewritten from the listing.

**Row level:**
- 000204-000208: 000756, 000789 and 000746, and the missing 000004 shape update.
- Kinematic moves: 000784, and the mass-frame composition of 000124/000126/000090.
- The wake blocks of 000174-000182, and 000182's row order.
- 000168's `_fpclass` gate.
- The summation orders of 000060/000742 and of the four W = R F frames.
- 000712's island-root refresh.
- The read locks on the NG readers.
- 000094's static arm.
- 000782 and 000791.
- 000128's x87 copy.

**The shape chain, what was written:**
- Actor.cpp 000036, 000024 and 000032.
- The group as its own object with a table: 001033, 001041, 001028, 001018 and 001032/001037/001039.
- 000531, 000533, 000535 and 000006.
- 000503 and the pruning rows.
- This also closed S1: the group-level 001315 call.

**Mass from shapes and setDynamic:**
- Written with 000164 and 000122: 000008, 000026, 000030, 000628, 000630, 000632, 000557, 004103,
  000722, 000776, 000797, 000799, and 000793's mass block.
- Also fixed:
  - 001024's model called slot 4 without `this`;
  - 000845's selector 1 left +0x00 unwritten;
  - the creation path used a one-box density approximation;
  - two reentry flags that should be one.

## Transcripts and tests

The Phase 5 registered coverage floor went from 871 to 1862, an increase of 991 oracle lines. Each
line was copied verbatim from the oracle side, and no existing expected line was edited.

| Task | Lines | Floor |
|---|---:|---:|
| 2 | 251 | 1122 |
| 3 | 446 | 1568 |
| 3 review | 3 | 1571 |
| 4 | 136 | 1707 |
| 5 | 110 | 1817 |
| 5 review | 32 | 1849 |
| 6 | 0 (no row unexecuted) | 1849 |
| final review | 13 | 1862 |

- The staged-pair targets that grew are ActorDynamicSetter, ActorBodyFlag, ActorForce, ActorCMass,
  ActorDynamics, ActorMomentum and ActorShapeMutation.
- All twelve Phase 5 actor staged pairs stay at stdout_delta=0.
- The Phase 6 and 7 floors (403 and 276) are unchanged.

## Rate

The method is the one in `joint-families.md` `## Rate`:
- rows moved are rows whose inventory `state` changed;
- rows written are counted per task, as the timing table counts them.

The window runs from the plan commit 63fd8cd (08:00:45) to the promotion commit 648946f (13:37:28):
5.61 h. The implementer intervals in the timing table add up to 4.80 h.

| Measure | Plan total | Per window hour | Per implementer hour |
|---|---:|---:|---:|
| Rows moved to `reconstructed` | 84 (unit 53, chain 31; 85 at Task 7) | 15.0 | 17.5 |
| Bytes moved | 42,651 (unit 30,673, chain 11,978) | 7,603 | 8,886 |
| Row writes (timing table) | 122 (2: 66; 3: 39; 3 review: 2; 4: 9; 5: 2; 5 review: 4) | 21.7 | 25.4 |
| Bytes written (timing table) | 58,372 | 10,401 | 12,166 |
| Rows gaining `dynamic_proof` | 86 (the 84, plus 000829 and 000849) | 15.3 | 17.9 |
| Registered coverage floor 5 | 871 -> 1862 (+991) | - | - |

**Caveats.**
- Row writes count a row once for each task that changed it. They exclude the chain rows the
  earlier tasks wrote outside the unit: 5,915 B in Task 3, 3,994 B in Task 4 and 3,114 B in Task 5.
- The joint-families caveats apply: wall-clock time is not effort, and the windows include review
  and idle gaps.

## Verification

Fresh configure and clean build of every target, 2026-09-28T13:22:56 to 13:27:17, on the tree at
a2803e0. That commit holds every product change of the plan; Task 7 changes records only.

```
cmake -S . -B build -A Win32 --fresh                  exit 0
cmake --build build --config Release --clean-first    exit 0
  NxPhysics.vcxproj warnings: 23 x C4005 'ARRAYSIZE' (winnt.h vs Ice/IceUtils.h), 5 x C4291,
  4 x D9025; the test targets add C4273, C4806 and LNK4217/LNK4286 (the joint-open-items profile)
NxPhysics.dll sha256 ccae60213e30a5ca98698734f36d402875a687b93486627b0b903d11f7243855
```

Checks and gates on that build (`run_phase_gate.ps1 -Phase N`), 13:27:17 to 13:29:10. The trace was
re-recorded on the same build afterwards.

```
git diff 63fd8cd -- Physics/include Foundation/include: empty
public_headers=pass (both roots, every gate)
pytest docs/reconstruction/novodex-physics/tools/tests: 753 passed, 690 subtests passed
validate_inventory.py: inventory=pass, closure phase=2 closed=56 deferred=87, phase=3 closed=61
  deferred=331, phase=4 closed=28 deferred=1025, phase=5 closed=0 deferred=205, phase=6 closed=2
  deferred=431, phase=7 closed=4 deferred=557, unexplained=0
phase 2 exit 0   phase_gate=2 status=pass
phase 3 exit 0   coverage_assertions_evaluated=103 floor=103 / phase_gate=3 status=pass
phase 4 exit 0   coverage_assertions_evaluated=159 floor=159 / phase_gate=4 status=pass
phase 5 exit 1   candidate CANDIDATE-MISSING family=vtables reason=shape finals/actor classes are Tasks 3-4
                 gate_failure=oracle_differential:NxPhysicsObjectLayoutTests exited 1 (candidate_fold
                 4492c8c1, unchanged); coverage_assertions_evaluated=1849 floor=1849; all 12 actor
                 staged pairs stdout_delta=0 stderr_exact=True; batch3268's documented provisional
                 failures=3 unchanged
phase 6 exit 0   coverage_assertions_evaluated=403 floor=403 / phase_gate=6 status=pass
phase 7 exit 0   coverage_assertions_evaluated=276 floor=276 / phase_gate=7 status=pass
```

- **Stable-ID form.** Every exact-form `// phys_fn_NNNNNN (0x%08x, N B)` line matches its inventory
  row's RVA and size. The files and line counts checked:

  | File | Lines |
  |---|---:|
  | NpActor.cpp | 72 |
  | Scene.cpp | 36 |
  | ObjectModel.cpp | 2 |
  | core/Joint.cpp | 34 |
  | core/JointSupport.cpp | 11 |
  | NpActorDynamicMath.h | 2 |

  Every promoted row's implementation file contains its ID. The validator exempts headers from this
  check.
- **Determinism.** The clean build differs from the Task 6 incremental build only in its link
  timestamp (0x6abaa2ad against 0x6aba9e29). The two builds give the same map offsets for all 138
  breakpoints and the same per-target HIT sequences.
- **work_units.json and the bundle.** `work_units.py` regenerated work_units.json (units=103,
  named=57, gaps=46). Only the NpActor.cpp unit and the units that own the chain rows changed.
  `unit_bundle.py` generated `units/NpActor.cpp.md`, with the Ghidra supplement.

## Final review fixes

The whole-branch review found two Important defects; `units/npactor-contract.md`
`## Final review fixes` has the detail.

- **I1 (6112710).** Since Task 2's `/arch:IA32` switch, NpActor.cpp's float copies compiled to
  `fld dword`/`fstp dword`, which quiet a signalling NaN, where the listings copy words with
  `rep movsd` or `mov`. Found by scanning every NpActor.obj function for an x87 load stored back
  without arithmetic and comparing with the same scan of the oracle rows. Fixed with memcpy in
  000204, 000208, 000210, 000214/000216, 000192, 000124/000126 (their inputs), 000218/000222 (their
  input) and the getters 000096, 000100, 000130 and 000134. Three NaN-selection defects surfaced in
  the new cases and were fixed as well: by-value dot-product operands (`nxNpActorX87Dot3` now takes
  references), 000124's `t + row` order, and the x arm's negated spill in both matrix-to-quaternion
  conversions (`x87FsqrtDiffSum`). 13 ActorCMass lines (`cmass snan_*`) register the oracle's words;
  the previous candidate differs in 12 of them.
- **I2.** 000628 is `discovered` again: its 003635 arm cannot be written without the fluid rows
  003485, 003593 and 003622, and Scene +0x61c is set only by the stubbed createFluid.
- **Minor M1** is corrected above (only 000628 lacked its fluid call).

Verification of the fix, 2026-09-28, on 6112710's product source:

```
cmake --build build --config Release                 exit 0 (incremental)
cmake --build build --config Release --clean-first   exit 0; NxPhysics.vcxproj warnings unchanged:
  23 x C4005, 5 x C4291, 4 x D9025
NxPhysics.dll sha256 9dadfcea6c5bba46368c7f204d5456dd0a63131f1f49162cd82c269ff89b4142
git diff 259dc52 -- Physics/include Foundation/include: empty; public_headers=pass
pytest docs/reconstruction/novodex-physics/tools/tests: 753 passed, 690 subtests passed
validate_inventory.py: inventory=pass, closure phase=7 closed=4 deferred=557, unexplained=0
phase 2 exit 0   phase_gate=2 status=pass
phase 3 exit 0   coverage_assertions_evaluated=103 floor=103 / phase_gate=3 status=pass
phase 4 exit 0   coverage_assertions_evaluated=159 floor=159 / phase_gate=4 status=pass
phase 5 exit 1   candidate CANDIDATE-MISSING family=vtables (NxPhysicsObjectLayoutTests exit 1,
                 candidate_fold 4492c8c1); coverage_assertions_evaluated=1862 floor=1862; all 12
                 actor staged pairs stdout_delta=0 stderr_exact=True
phase 6 exit 0   coverage_assertions_evaluated=403 floor=403 / phase_gate=6 status=pass
phase 7 exit 0   coverage_assertions_evaluated=276 floor=276 / phase_gate=7 status=pass
```
