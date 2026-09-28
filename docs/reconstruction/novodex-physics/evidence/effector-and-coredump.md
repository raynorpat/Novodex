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

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T07:58:00 (approx.; the first build started just before 08:00) | 2026-09-28T08:30:00 | 0 | 0 | Contract only. Fresh configure and Release build of the worktree (`build/`); Phase 6 gate baseline `status=pass` (403/403 coverage assertions). Bundles for `gap:fluids\NpImplicitMesh.cpp..NpSpringAndDamperEffector.cpp`, `NpSpringAndDamperEffector.cpp`, `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` generated. Supplement rerun with the 34 existing plus 9 new requests (0x8ed50 0x8edb0 0x8f100 0x8fc00 0x8fc50 0x8fcb0 0x8fd00 0x91940 0x91de0), 43/43 `ok`, existing entries unchanged. Effector: 30 rows 3,325 B plus 12 Scene/NpScene rows 887 B; the candidate body record has no Observable part, which the effector's observer calls need (Task 2 prerequisite). Core dump: 41 rows 22,965 B plus 003981; always returns false; text and binary differ only in the float token; the file embeds the date and heap pointers, which the test must normalise. Split: 2 = 4,212 B, 3a = 11,793 B (24 rows incl. 003981), 3b = 11,630 B, 4 = 221 B. |
| 2 | 2026-09-28T08:32:00 (approx.) | 2026-09-28T09:20:00 | 43 (40 hand-written, 3 compiler-generated: 003932 003938 003954) | 4,244 | 30 effector rows 3,325 B, 12 Scene/NpScene rows 887 B, 000713 32 B; deferred 000791 (133 B, needs 000782). Body record Observable (placement at +0, no field moved, no allocation), the 0x100 notify and `~Observable` in `releaseActor`, 000575 in `nxSceneDelete`. New staged-pair target `NxPhysicsEffectorTests`, 77 registered oracle lines, `stdout_delta=0`. Defect found by the transcript: shape id recycled after the shape free in `releaseActor` (fixed). Left: the record +0x14 pad (zeroed by the candidate) and the missing 000722 island snapshot (+0x1f8). Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
| 2 (review) | 2026-09-28T09:22:00 (approx.) | 2026-09-28T09:45:00 | 1 (000722) | 127 | Body construction now runs 000760 then 000722 (000797 0x1b6fb/0x1b702); root +0x1f8 and the island words identical to the oracle and registered (2 lines; floors 6/7 = 482/355). Trace re-taken (000722, 000760 hit 4 times each). 003970/003972 caveat added; compound-shape recycle order recorded. Gates 2/3/4/6/7 pass, 5 only the vtables marker. |
