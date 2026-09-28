# Effector and core dump

Measurement note for the effector-and-coredump plan
(`docs/superpowers/plans/2026-09-28-effector-and-coredump.md`), which continues the joint
families (`evidence/joint-families.md`): the spring-and-damper effector
(`NxScene::createSpringAndDamperEffector`, `releaseEffector`, the effector enumeration and the
`NxSpringAndDamperEffector` API) and the scene core-dump writer behind
`NxPhysicsSDK::coreDump`, reconstructed as real source and checked by staged-pair
differentials. The contract is `units/effector-coredump-contract.md`. Each task appends one
row to the timing table below.

## Task 2: the effector

The spring-and-damper effector is product code and wired to the public API: the 30 effector
rows (`core/SpringAndDamperEffector.cpp`, `core/NpSpringAndDamperEffector.cpp`), the 12
Scene/NpScene rows (`Scene.cpp`, `NpScene.cpp`) and 000713 (`core/JointSupport.cpp`); 000791 is a
deferred stub. The body record gained its Observable part and the record teardown its 0x100
notify, and the Scene release calls 000575 after the actor loop (details in
`units/effector-coredump-contract.md`, "### Task 2 record").

`NxPhysicsEffectorTests` drives the public API and the internal slots 2 and 3 over two dynamic
actors and prints, per step, the SDK allocations and frees, the internal effector's words, the
records' observer lists, the getters and the effector count and iterator. First run: 9 lines
differed per side. Five were the test's own (the lock-link heap pointers, now printed as names);
one was the record's +0x14 pad (0xcdcdcdcd in the oracle, 0 in the candidate, whose record is
`memset`); two were the chain root's +0x1f8 (0x3ecccccc in the oracle, the island wake counter
000722 copies there; 0 in the candidate); both are record-construction gaps outside the effector
and are no longer printed. The last was a real defect in the Scene release's free order: the
candidate recycled a shape's id after freeing the shape, the oracle before (fixed in
`Scene.cpp`). After that the transcript is identical (`stdout_delta=0`), and removing the record
notify makes the candidate fault in the next effector release (a mutation check, not recorded as
a ledger closure). 77 oracle lines registered; floors 6/7 = 480/353.

The cdb trace (`evidence/effector-and-coredump-trace-effector.txt`) shows 40 of the 43 written
rows executing; 003932 and 003938 (abstract classes' deleting destructors) and 003964 (the core
dump's reader) do not run. 003979 runs with both roots' +0x1f8 set to 0, so its force arm
(000791) is not reached and its arithmetic is checked against the listing only.

Review fix: the +0x1f8 difference was a record-construction defect, not a line to drop. The
candidate's `nxActorComputeMass` now calls 000760 and then 000722 (written in
`core/JointSupport.cpp`), 000797's order, so the island words +0x1bc..+0x200 and the root's
+0x1f8 (0x3ecccccc) match the oracle; two island lines are registered (79 in all, floors 6/7 =
482/355) and every Phase 5 and joint staged-pair target is still identical. The re-taken trace
shows 000722 and 000760 running once per dynamic body. The record's +0x14 pad stays as recorded;
`releaseActor`'s compound-shape branch order is recorded as unmeasured (contract, "### Task 2
record").

## Task 3a: the core dump's first half

The core dump's infrastructure, joint blocks and entry are product code in
`Physics/src/core/SceneDump.cpp` (new; `/arch:IA32`, `/EHs-c-`): the date, name and float-token
rows, the settings records, the joint frame and limit text rows, the joint line, the joint block
(004037 with its three continuations) and `PhysicsSDK::coreDump` (004062); the readers 004068,
004072 and 004085 in `core/Joint.cpp`; the SDK parameter and group-mask arrays reach the dump
through two accessors in `PhysicsSDK.cpp`. 27 rows, 11,793 B, 23 of them moved from `discovered`
(the other four were model rows). The descriptor inlines carry stable-ID lines only. The asset
writer 004051 is a placeholder for Task 3b.

Nothing calls 004062 yet (000267 is the candidate stub), so this task records static proofs only:
every row transcribed from the Capstone listing, and every format string the file passes to the
CRT checked against the image. The listing corrected one contract claim: the 0x20000-byte block
004062 allocates is the mesh-name table 003991 fills, reached through the three frame words
passed to 004051 (contract, "### Task 3a record"). Gates 2, 3, 4, 6 (482/482) and 7 (355/355)
pass; Phase 5 fails only on the vtables marker, as before.

## Task 3b: the core dump's second half

The asset writer and everything below it are product code in `Physics/src/core/SceneDump.cpp`: the
mesh names (003991), trigger flags (004017), welded vertex lines (004035), the mesh block (004046),
the shape records (004048) and the per-scene asset writer (004051 with its five continuations:
timing header, gravity, joint blocks, actors with their body records and shapes, joint lines,
disabled pairs, spring-and-damper effectors). The shape-descriptor inlines carry stable-ID lines
only. The readers are written with their units: 000015/000017 in `core/JointSupport.cpp`,
000509/000523 in `Scene.cpp`, 001283 in `ContactGeneration.cpp`. 23 rows, 11,630 B, 15 of them
moved from `discovered` (the other eight were model rows). Deferred stubs: 001472 (convex polygon
builder) and 000525/000527 (pair-flag array); with 004046 and the mesh arm they are unreachable in
the candidate, which builds no mesh shape and raises no pair flag.

Static proofs only (Task 4 wires 000267): every row transcribed from the Capstone listing, the
three inlined quaternion spellings kept with their spilled pairs, and every format literal checked
against the image (225 distinct, all present; 222 NUL-delimited, three starting at the oracle's
pointer behind a table word). One oracle quirk recorded: the capsule arm passes the capsule's own
`flags` to the trigger writer (contract, "### Task 3b record"). Gates 2, 3, 4, 6 (482/482) and 7
(355/355) pass; Phase 5 fails only on the vtables marker, as before, with all 12 staged-pair
targets at `stdout_delta=0`.

## Task 4: coreDump wired and tested

`NxPhysicsSDK::coreDump` is 000267 as the listing has it (`NpPhysicsSDK.cpp`): the scene write-lock
walk, the reverse unlock and report at line 225 on a held lock, and the call to 004062 with its
result (always false) returned after the unlock walk.

`NxPhysicsCoreDumpTests` (Phases 6 and 7) builds two populated scenes through the public API --
static plane and box, dynamic sphere, box, capsule, kinematic, three-shape, zero-shape, frozen with
collision off, asleep and unnamed actors; all ten joint families with names, breakability,
collision and limit planes; a spring-and-damper effector; two added materials, three changed
parameters, two disabled group pairs -- and dumps them seven times: text and binary, each with and
without an addendum, the deadlock arm, one scene, none. Each `.psc` file is printed back line by
line, with only the date line and the pointer tokens normalised (pointers to first-appearance
ordinals, so aliasing is still compared).

First runs: the oracle refused a dynamic actor whose only shape is a trigger (the candidate
accepts it), and the candidate's creation model has no mass-from-shapes (phys_fn_000008, not
written): spheres, capsules and compounds got mass 0, a local-pose box no com/comrot, a small cube
an inertia one ulp off, and one limit plane's round trip followed. The scene now puts triggers on
a static actor and a compound part, and gives every dynamic actor but one (an unrotated density
box) its mass and inertia. What remained was one real defect in what the dump reads: a body with
no maxAngularVelocity of its own took a pinned 7.0 where the body loader 000795 squares the SDK's
live `NX_MAX_ANGULAR_VELOCITY` (the sleep thresholds likewise take the live parameters); fixed in
`Scene.cpp`. After that the transcript is identical (`stdout_delta=0`) and no writer row needed a
change. 278 oracle lines registered; floors 6/7 = 760/633.

The cdb trace (`evidence/effector-and-coredump-trace-coredump.txt`) shows 000267, 004062 and every
3a/3b row the scene can reach executing, with the continuations of 004037/004051 and the three rows
the candidate inlined (003992, 003994, 004013) anchored on instructions only they execute. Not
hit: the mesh arm (003991 004035 004046 001472), the pair loop (004059) and 000525, and the
descriptor inlines nothing calls. 29 rows gain `dynamic_proof` (000267 among them); 000267 moves to
`reconstructed` (Phase 2 ledger: `reconstructed_not_falsified`, 22 -> 21 blocked, 6 -> 7).

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T07:58:00 (approx.; the first build started just before 08:00) | 2026-09-28T08:30:00 | 0 | 0 | Contract only. Fresh configure and Release build of the worktree (`build/`); Phase 6 gate baseline `status=pass` (403/403 coverage assertions). Bundles for `gap:fluids\NpImplicitMesh.cpp..NpSpringAndDamperEffector.cpp`, `NpSpringAndDamperEffector.cpp`, `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` generated. Supplement rerun with the 34 existing plus 9 new requests (0x8ed50 0x8edb0 0x8f100 0x8fc00 0x8fc50 0x8fcb0 0x8fd00 0x91940 0x91de0), 43/43 `ok`, existing entries unchanged. Effector: 30 rows 3,325 B plus 12 Scene/NpScene rows 887 B; the candidate body record has no Observable part, which the effector's observer calls need (Task 2 prerequisite). Core dump: 41 rows 22,965 B plus 003981; always returns false; text and binary differ only in the float token; the file embeds the date and heap pointers, which the test must normalise. Split: 2 = 4,212 B, 3a = 11,793 B (24 rows incl. 003981), 3b = 11,630 B, 4 = 221 B. |
| 2 | 2026-09-28T08:32:00 (approx.) | 2026-09-28T09:20:00 | 43 (40 hand-written, 3 compiler-generated: 003932 003938 003954) | 4,244 | 30 effector rows 3,325 B, 12 Scene/NpScene rows 887 B, 000713 32 B; deferred 000791 (133 B, needs 000782). Body record Observable (placement at +0, no field moved, no allocation), the 0x100 notify and `~Observable` in `releaseActor`, 000575 in `nxSceneDelete`. New staged-pair target `NxPhysicsEffectorTests`, 77 registered oracle lines, `stdout_delta=0`. Defect found by the transcript: shape id recycled after the shape free in `releaseActor` (fixed). Left: the record +0x14 pad (zeroed by the candidate) and the missing 000722 island snapshot (+0x1f8). Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 2 (review) | 2026-09-28T09:22:00 (approx.) | 2026-09-28T09:45:00 | 1 (000722) | 127 | Body construction now runs 000760 then 000722 (000797 0x1b6fb/0x1b702); root +0x1f8 and the island words identical to the oracle and registered (2 lines; floors 6/7 = 482/355). Trace re-taken (000722, 000760 hit 4 times each). 003970/003972 caveat added; compound-shape recycle order recorded. Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 3a | 2026-09-28T09:45:00 | 2026-09-28T10:10:38 | 27 (18 hand-written incl. 3 continuations of 004037; 6 compiler-generated desc inlines: 003981 003985 004021 004023 004025 004027; readers 004068 004072 004085) | 11,793 | `core/SceneDump.cpp` (new, /arch:IA32 and /EHs-c-) + `include/core/SceneDump.h`; readers in `core/Joint.cpp`; parameter/group-mask accessors in `PhysicsSDK.cpp`. Not wired (Task 4); static proofs only; 163/164 format literals NUL-delimited in the image (the 164th, `\r\n`, a string tail as in the oracle). Contract correction: the 0x20000 block is 003991's mesh-name table. 004051 placeholder for 3b. 23 rows discovered -> reconstructed, Phase 6 ledger 45/386. Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 3b | 2026-09-28T10:12:00 (approx.) | 2026-09-28T10:58:00 | 23 (11 hand-written incl. 5 continuations of 004051; 7 compiler-generated desc inlines: 003983 003987 003989 004019 004029 004031 004033; readers 000015 000017 000509 000523 001283) | 11,630 | Asset, shape, mesh and effector rows in `core/SceneDump.cpp`; readers in `core/JointSupport.cpp`, `Scene.cpp`, `ContactGeneration.cpp`; deferred stubs 001472 and 000525/000527 (unreachable: no mesh shapes, no pair flags). Not wired (Task 4); static proofs only; 225/225 format literals in the image (222 NUL-delimited, 3 at the oracle pointer behind a table word). Found: capsule arm passes its own flags to 004017. 15 rows discovered -> reconstructed, Phase 6 ledger 30/401. `NxPhysicsInternalTests` links NarrowPhase/ContactGeneration. Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 4 | 2026-09-28T10:55:00 (approx.) | 2026-09-28T11:30:00 | 1 (000267) | 221 | 000267 wired in `NpPhysicsSDK.cpp` (lock walk, reverse unlock and report at line 225, call to 004062); `NpScene::writeLink()`. New staged-pair target `NxPhysicsCoreDumpTests` (Phases 6/7): two populated scenes dumped seven times (text/binary, with/without addendum, deadlock arm, one scene, none), each `.psc` printed back; date line and pointer tokens normalised. `stdout_delta=0`; 278 oracle lines registered, floors 6/7 = 760/633. Defect found and fixed: body thresholds (maxAngularVelocity, sleep velocities) from the live SDK parameters as 000795 does, not pinned defaults (`Scene.cpp`). Found, not fixed (unwritten Phase 5 rows): no mass from shapes (000008) and a trigger-only dynamic actor accepted; the scene gives explicit masses. cdb trace: 29 rows gain `dynamic_proof`; not hit: mesh arm, pair loop, 000525, descriptor inlines. 000267 Phase 2 ledger `reconstructed_not_falsified`. Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 5 | 2026-09-28T11:30:00 (approx.) | 2026-09-28T12:05:00 | 0 | 0 | Review coverage: scene C in `NxPhysicsCoreDumpTests` (awake(false) on a dynamic body, capsule flags, every printable PsDefaultSettings kind, a static three-shape actor), 95 oracle lines, floors 6/7 = 855/728; defect found and fixed: capsule flags not stored at +0xe8 (`Scene.cpp`). Records: unreached branches, 000008/000795 open-item notes, Phase 6 ledger text, superseded contract sections. `work_units.json` and the three bundles regenerated. Result, defects, open items, rate, verification written. Fresh configure and clean build; gates 2/3/4/6/7 pass, 5 only the vtables marker. |

## Result

Every row of `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` is reconstructed (65 rows, 26,570 B, per
the regenerated `work_units.json`); `NpSpringAndDamperEffector.cpp` is complete (3 rows, 324 B,
plus its shared rows in the gap). Rows written, by part:

| Part | Rows | Bytes | Where | Dynamic proof |
|---|---:|---:|---|---|
| Spring-and-damper effector (internal and public, incl. 003964) | 30 | 3,325 | `core/SpringAndDamperEffector.cpp`, `core/NpSpringAndDamperEffector.cpp` | 29 of 30 |
| Scene/NpScene effector rows (000301 000303 000327 000329 000331 000561 000565 000569 000573 000575 000587 000594) | 12 | 887 | `Scene.cpp`, `NpScene.cpp` | 12 |
| Dump writer, Task 3a (24 rows, 6 of them compiler-generated descriptor inlines) | 24 | 11,702 | `core/SceneDump.cpp` | 19 (the descriptor inlines are never called) |
| Dump writer, Task 3b (18 rows, 7 of them compiler-generated descriptor inlines) | 18 | 11,517 | `core/SceneDump.cpp` | 10 (mesh arm and pair loop unreachable; descriptor inlines never called) |
| Readers (004068 004072 004085 000015 000017 000509 000523 001283) | 8 | 204 | `core/Joint.cpp`, `core/JointSupport.cpp`, `Scene.cpp`, `ContactGeneration.cpp` | 8 |
| 000267 `NpPhysicsSDK::coreDump` | 1 | 221 | `NpPhysicsSDK.cpp` | yes |
| 000713 record chain root, 000722 island wake counter | 2 | 159 | `core/JointSupport.cpp` | 2 |
| **Total** | **95** | **28,015** | | |

61 rows (26,599 B) moved from `discovered` to `reconstructed`; the rest were model rows that
became product rows. 50 rows gained a `dynamic_proof` from the two cdb traces
(`evidence/effector-and-coredump-trace-effector.txt`, `evidence/effector-and-coredump-trace-coredump.txt`).
No row is above `reconstructed`; every moved row takes `reconstructed_not_falsified` in its phase
ledger (Phase 6, 7 and 2).

Wired: `NxScene::createSpringAndDamperEffector`, `releaseEffector`, `getNbEffectors`, the effector
iterator and every `NxSpringAndDamperEffector` method; `NxPhysicsSDK::coreDump` (text and binary,
with and without an addendum, and its deadlock arm). Two staged-pair targets on Phases 6 and 7,
both `stdout_delta=0`: `NxPhysicsEffectorTests` (79 registered oracle lines) and
`NxPhysicsCoreDumpTests` (373). Floors 6/7 went from 403/276 to 855/728.

Deferred, as asserting stubs with their stable-ID lines, rows left `discovered`:
- 000791 (133 B, `addForceAtPos` on a body record): needs 000782, not written; the effector's
  solver slot 003979 reaches it only when a root's +0x1f8 island wake counter is non-zero.
- 001472 (664 B, the convex mesh's polygon builder): the dump's mesh arm, unreachable because the
  candidate builds no type-4 shape.
- 000525/000527 (345 B, the pair-flag array): the dump's pair block, unreachable because the
  candidate never raises a pair flag.

## Defects found

Found by the transcripts, fixed in source (no expected line was ever edited):

| Defect | Found by | Fix | Commit |
|---|---|---|---|
| The scene release recycled a shape's id after freeing the shape; the oracle recycles it first | effector transcript (free order) | `Scene.cpp` `releaseActor` | 7c9e589 |
| The body record had no Observable part and its teardown no 0x100 notify, so an effector's pointer to a released body stayed live; the scene release did not release live effectors (000575) | effector contract prerequisite, then the transcript | record Observable at +0, notify and `~Observable` in `releaseActor`, 000575 in `nxSceneDelete` | 7c9e589 |
| Body construction did not build 000797's island (000760, then 000722's copy), so the chain root's +0x1f8 island wake counter was 0 instead of 0x3ecccccc | effector transcript (Task 2 review) | 000722 written; both called in 000797's order | ab526d7 |
| Bodies with no threshold of their own took a pinned 7.0 max angular velocity and pinned sleep velocities; 000795 reads the live SDK parameters (`maxangularvelocity(7)` against 9) | core-dump transcript | `Scene.cpp` creation model | af43f33 |
| A capsule's own `flags` (desc +0x54) were not stored at +0xe8, where 000989 stores them and the dump's capsule arm reads them for 004017 (`triggerevent(enter,)` missing) | core-dump transcript, scene C | `Scene.cpp` shape factory | d628181 |

No dump-writer row (3a/3b) needed a change after its static proof: every record format, token,
name, quote and line ending matched on the first comparison.

## Open items

- **Mass from shapes and the trigger-only dynamic refusal** (separate follow-up task): phys_fn_000008
  and the shape mass slots are not written; the candidate's creation model covers one unrotated
  box from a density only, and accepts a dynamic actor whose only shape is a trigger, which the
  oracle refuses ("Can't compute mass from shapes"). `NxPhysicsCoreDumpTests` gives its dynamic
  actors explicit masses until then. Recorded on phys_fn_000008 and phys_fn_000795.
- **000791 / 000782**, the solver's force path from the effector slot 003979: deferred stub; the
  effector test zeroes the roots' +0x1f8 so it is not reached.
- **Mesh dump rows** 001472 (stub), 004046 and 004035 (written, unreachable), and the pair-flag
  rows 000525/000527 (stub): no mesh shape and no pair flag can be made in the candidate.
- **The dump's elapsed-time header** (004051, Scene+0x544 non-zero): printed only after a simulate,
  which the candidate does not have.
- **Compound-shape recycle order in `releaseActor`**: the effector test covers single-shape actors;
  the order for a compound's children is recorded in the contract (Task 2 review), not compared.
- **The body record's +0x14 pad**: 0xcdcdcdcd in the oracle (never written), 0 in the candidate
  (its record is `memset`); not printed by any target.
- **Address reuse after a free**: the core-dump test restarts its pointer ordinals for scene C,
  because an address a freed object had may be handed out again depending on the process's
  address-space history.

## Rate

Method as in `evidence/joint-families.md`: written rows and bytes against the timing table's own
hours (impl.), and moved rows against the window between final commits.

| Task | Impl. h | Written rows | Written B | Written rows/h | Written B/h |
|---|---:|---:|---:|---:|---:|
| 1 (contract) | 0.53 | 0 | 0 | - | - |
| 2 (effector, Scene rows, 000713) | 0.80 | 43 | 4,244 | 54 | 5,305 |
| 2 review (000722) | 0.38 | 1 | 127 | 3 | 334 |
| 3a | 0.43 | 27 | 11,793 | 63 | 27,425 |
| 3b | 0.77 | 23 | 11,630 | 30 | 15,104 |
| 4 (000267, test) | 0.58 | 1 | 221 | 2 | 381 |
| 5 (coverage, records) | 0.58 | 0 | 0 | - | - |
| **Block** | **4.07** | **95** | **28,015** | **23** | **6,883** |

Window: 07:59:14 (`25acaf2`, the plan) to the Task 5 records commit, about 4.1 h for 61 moved rows
(26,599 B): about 15 rows/h, 6,500 B/h. The dump writer (3a/3b, 23,423 B with its readers) was
transcribed at about 19,000 B/h and needed no correction once the transcript ran; the effector
and the object-state fixes around it are where the time went per byte.

## Verification

On the final tree (Task 5):
- `cmake -S . -B build -A Win32 --fresh` exit 0; `cmake --build build --config Release --clean-first`
  exit 0, no errors.
- Public headers: `git diff 259dc52 -- Physics/include Foundation/include` is empty;
  `verify_public_headers.py --root Physics/include --manifest public_header_hashes.json` passes
  (files=80), as do the gates' `immutable_headers` checks.
- Tool tests: `python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'`
  -> 753 tests OK.
- Validator: `validate_inventory.py inventory.json` -> `inventory=pass`, exit 0.
- Gates: 2 pass; 3 pass (103/103); 4 pass (135/135); 5 fails only on `candidate
  CANDIDATE-MISSING family=vtables` (871/871, 12/12 staged-pair targets `stdout_delta=0`); 6 pass
  (855/855, 6/6 differentials `stdout_delta=0`); 7 pass (728/728).
- Stable-ID form: every `// phys_fn_` line in `Physics/src/core/*.cpp` matches
  `// phys_fn_NNNNNN (0x........, N B)`; 0 violations.
- CRT use in the block's `core/` files: `SceneDump.cpp` calls only stdio, string and time
  functions, where the oracle calls its static CRT (fopen, fprintf, sprintf, strstr, fclose,
  strftime, time, localtime, tzset, operator new, free; plus string copies); its three `fabs` compile to the x87 `fabs`
  instruction (0x5a02f, 0x5a040, 0x5a051 in the candidate, no call), as the oracle's 004035 does;
  `SpringAndDamperEffector.cpp` and `NpSpringAndDamperEffector.cpp` call no CRT math (their square
  roots go through `X87Sqrt.h`). The joint files' existing `sin`/`cos`/`tan`/`atan2` uses are the
  joint families' and unchanged here.
