# SDD ledger — plan: docs/superpowers/plans/2026-09-24-nxphysics-completion.md

Task M5.1: complete (commit HEAD; tests: baseline NxPhysicsCoreDumpTests exact; destructor purge mutation caught at stdout_delta=2 with 14/17 allocations; restored control exact; tools suite 784/784; inventory=pass; public_headers=pass 80 files).

Task M5.2: complete (D6 global-anchor/global-axis rows phys_fn_004437 and phys_fn_004441 dynamically gated; each isolated getter mutation changed its staged-pair output and produced stdout_delta=24; restored controls exact; inventory=pass; public_headers=pass 80 files; validator=pass; completion report tests 11/11; work-unit tests 12/12).

Task M5.3: complete (D6 public setGlobalAxis row phys_fn_004439; staged-pair public setter/readback fixture exact at baseline/restored, forced-axis mutation caught with stdout_delta=6; Phase 6/7 registration floors updated; full gate-target tests 37/37; inventory and public-header validation pass (6,338 functions, 5,138 data objects, 0 unexplained; 80 headers unchanged)).

Task M5.4: complete (D6 public setBreakable row phys_fn_004445; staged-pair readback exact at baseline/restored, forced-zero maxForce mutation caught with stdout_delta=4; registered Phase 6/7 coverage assertion added; coverage-floor and registry tests pass; completion-report tests 11/11; work-unit tests 12/12; inventory pass (6,338 functions, 5,138 data objects, 0 unexplained); 80 public headers unchanged).

Task M5.5: complete (D6 public setLimitPoint row phys_fn_004447; staged-pair readback exact at baseline/restored, forced-zero point mutation caught with stdout_delta=2; registered Phase 6/7 coverage assertion added; coverage-floor and registry tests pass; completion-report tests 11/11; work-unit tests 12/12; inventory pass (6,338 functions, 5,138 data objects, 0 unexplained); 80 public headers unchanged).

Task M5.6: complete (D6 addLimitPlane refusal row phys_fn_004449; public return/iterator output exact at baseline/restored, forced-true return mutation caught with stdout_delta=2; registered Phase 6/7 assertion added; coverage-floor and registry tests pass; completion-report tests 11/11; work-unit tests 12/12; inventory pass (6,338 functions, 5,138 data objects, 0 unexplained); 80 public headers unchanged).

Task M5.7: complete (D6 public setName row phys_fn_004453; staged-pair readback exact at baseline/restored, no-op mutation caught with stdout_delta=2; registered Phase 6/7 assertion added; coverage-floor and registry tests pass; completion-report tests 11/11; work-unit tests 12/12; inventory pass (6,338 functions, 5,138 data objects, 0 unexplained); 80 public headers unchanged).

### M5.8 — D6 descriptor save row

Closed `phys_fn_004459` (`NpD6Joint::saveToDesc`) from the existing public D6 descriptor-save fixture. A no-op mutation was caught by `NxPhysicsJointStagedPairTests` at `stdout_delta=92`; restored output is exact. No new coverage assertion was added.

### M5.9 — D6 drive-position write-lock path

Closed `phys_fn_004461` with the registered staged-pair lock-contention fixture. Suppressing the expected invalid-operation callback is caught at `stdout_delta=24`; restored output is exact. One Phase 6/7 coverage line was added.

### M5.10 — D6 drive setter write-lock paths

Closed `phys_fn_004463`, `phys_fn_004465`, and `phys_fn_004467` with the staged-pair contention fixture. Each suppressed invalid-operation callback is caught at `stdout_delta=24`; restored output is exact. Three Phase 6/7 coverage lines were added.

### M5.11 — D6 wrapper constructor

Closed `phys_fn_004469`. Nulling its internal D6 pointer is caught by the staged-pair fixture as a candidate access violation (`stdout_delta=2916`); restored output is exact.

M1.2 implementation commits: 4cf9bfb8 and 505c1f1a (both on local main). Merged-main verification: fresh Phase 6 1,238/1,238; Phase 7 1,429/1,429; full tools suite 800/800; retained JSON proofs equal the gate outputs.


### M1.3 � Joint matrix oracle-only proof

Added a `joint_matrix` proof format and pinned the real `NxPhysicsJointTests` transcript: 122 cases over ten joint families, 284 descriptor and transformed-fixture input lines, and 2,873 oracle output lines. The proof excludes only machine-specific pair paths/module counts while binding the oracle image and normalized fixture source. Committed as 9a6409a9 on main. Phase 6 passed 1,238/1,238, Phase 7 passed 1,429/1,429, all three retained proof files match runner outputs, and the full tools suite passed 803/803.

### M5.14 — Shared joint global-anchor setter

Closed `phys_fn_004099` with a new public D6 `setGlobalAnchor`/`getGlobalAnchor` readback in the registered joint staged-pair fixture. A +1.0 local-X mutation is caught with `stdout_delta=4`; restored output is exact. Updated the pinned oracle matrix to 2,874 output lines and refreshed all retained self-hashed proof artifacts. Phase 6: 1,239/1,239; Phase 7: 1,430/1,430; inventory: 6,338 functions, 5,138 data objects, zero unexplained. Public headers remain unchanged.

### M5.15 — Shared joint global-axis setter

Closed `phys_fn_004101` using the existing rotated joint staged-pair matrix and public D6 axis setter/readback. Adding 1.0f to the normalized X axis before tangent construction is caught with `stdout_delta=6`; the restored differential is exact. Phase 6 now has 87 closed / 346 deferred function rows. No additional fixture line was needed.

### M5.16 — Joint actor attachment

Closed `phys_fn_004107` using the ten-family public joint staged-pair matrix. An immediate-return mutation causes the candidate to access-violate during joint construction (`candidate_exit=-1073741819`, `stdout_delta=3160`); the restored differential is exact. Phase 6 now has 88 closed / 345 deferred function rows.

### M5.17 — Joint limit-point setter

Closed `phys_fn_004109` on the existing D6 public setter/readback case. Adding 1.0f to the local X storage is caught with `stdout_delta=2`; the restored differential is exact. Phase 6 now has 89 closed / 344 deferred function rows.

### M5.18 — Joint break-event slot

Closed `phys_fn_004111` using the registered joint-slot break fixture. An immediate-return mutation changes the break state and support flags (`stdout_delta=4`); the restored differential is exact. Phase 6 now has 90 closed / 343 deferred function rows.

### M5.19 — Shared joint base descriptor loader

Closed `phys_fn_004121` using the ten-family public descriptor matrix. An immediate-return mutation removes base descriptor state and is caught with `stdout_delta=2806`; the restored differential is exact. Phase 6 now has 91 closed / 342 deferred function rows.

Task M5.20: complete (commit 83a60001; phys_fn_004125; +1 computed-X mutation caught by NxPhysicsJointStagedPairTests at stdout_delta=246; restored control exact; inventory closure 92/341; Phase 6 1,239/1,239 and Phase 7 1,430/1,430; inventory tests 251 passed, oracle-proof tests 16 passed).

Task M5.21: complete (commit 81f1370d; phys_fn_004131; zero-return mutation caught by NxPhysicsCoreDumpTests at stdout_delta=4448; restored control exact; inventory closure 93/340; Phase 6 1,239/1,239 and Phase 7 1,430/1,430; inventory tests 251 passed, oracle-proof tests 16 passed).

Task M5.22: complete (commit a7ca8aba; phys_fn_004129; +1 computed-X mutation caught by NxPhysicsJointStagedPairTests at stdout_delta=246; restored control exact; inventory closure 94/339; Phase 6 1,239/1,239 and Phase 7 1,430/1,430; inventory tests 251 passed, oracle-proof tests 16 passed).


### M1.4 - Initial call-cleanup ABI audit

Corrected 15 mismatched layout-harness call typedefs/argument counts against the pinned DLL rows, and fixed the Capstone cleanup parser to stop on direct, indirect, and register tail jumps. The new parser tests pass (3/3), the audit reports zero decidable mismatches, the rebuilt `NxPhysicsObjectLayoutTests --self` passes with zero candidate mismatches, and Phase 5 passes 2,605/2,605 coverage assertions. Nonvolatile-register and structure-return ABI checks remain open.


### M5.24 — Fluid emitter flag accessors and collision input coverage

Committed the local flag mask setter/getter implementation and registered its public-interface probe (`c326054a`). `phys_fn_003852` is closed after a fresh throwaway CMake mutation shifted the getter read from `mInternal+0x10` to `+0x14`; the flag probe caught it with `mismatches=1`, and restored control is exact. `phys_fn_003850` remains open for backend callbacks on masks 4, 8, and 16. The follow-up gate-registry audit found missing input and coverage registrations for direct mesh/mesh collision probes; added semantic input digests and a dispatcher coverage line. Phase 3 passes 543/543, Phase 5 passes 2,616/2,616, and Phase 7 passes 1,444/1,444. The five-family collision-object destructor differential was rebuilt and re-run: all five secondary and five primary deleting paths match, and the member-only negative control reports five mismatches. The focused inventory/floor assertions pass; the repository-wide Python module remains uncompleted because its source-wide scan did not finish in a bounded wait.

Task M7.1: Ruling: the completion plan defines M0–M8 checklists but no numeric Task N brief, so this packet extracts one bounded M7 fluid-support contract into task-M7.1-brief.md; it follows the plan's dependency/evidence loop and does not mark M7 complete. Cost if wrong: the packet may omit a prerequisite from the future FluidModel loader, so the broader FluidModel work remains an open M7 dependency.


Task M7.1: complete (phys_fn_003593 internal backend-flag dispatch and phys_fn_003850 public write-lock wrapper; direct candidate-DLL setter resolved from NxPhysics.map; pinned-oracle callback differential covers six backend set/clear transitions and one non-backend flag; wrong callback mutation caught at callback_mismatches=2; no-op wrapper mutation caught at 11 total mismatches; restored runs exact; Phase 5/7 coverage floors 2617/1445; evidence: docs/reconstruction/novodex-physics/evidence/fluid-emitter-set-flag-backend-003593-003850.md).

Task M6.1 ruling: the plan specifies the M0–M8 milestone sequence but no packet-level task IDs, so this bounded task brief extracts the next central M6 scheduler contract for `phys_fn_000659`. It follows the established milestone evidence loop and does not close M6; the downstream solver, queues, callbacks, and extension rows remain independently tracked.

Task M6.1: complete (`phys_fn_000659`, `NxSceneInternal::simulateFrame`; whole-row immediate-return mutation rejected by the registered `NxPhysicsSimulationTests` Phase 7 staged-pair differential at `stdout_delta=3829`, restored control exact; Phase 7 gate passes 1,447/1,447 coverage assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; focused tools tests pass: inventory 251, completion 11, work units 12, gate targets 39; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-scheduler-000659-mutation.md`.

Task M6.2: complete (`phys_fn_000653`, `NxSceneInternal::releaseJoint`; whole-row immediate-return mutation rejected by the registered `NxPhysicsJointStagedPairTests` Phase 7 staged-pair differential at `stdout_delta=494` with both exits zero and exact stderr; restored control exact; Phase 7 gate passes 1,447/1,447; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; focused tools tests pass: inventory 251, completion 11, work units 12, gate targets 39; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-joint-000653-mutation.md`.

Task M6.3 ruling: the next lifecycle row is `phys_fn_000661` (`Scene::addJoint`). Its candidate body and dynamic calls are already traced, but the Phase 7 ledger still defers it for lack of mutation falsification. The previous clean archive's implementation files are byte-identical to current `main`; overlaying the intervening documentation-only commit leaves a disposable archive of current `main` without forcing an unrelated full rebuild.


Verification update for Task M7.1: final candidate DLL rebuild/probe passed with zero constructor, raw ABI, flag, backend callback, state, and aggregate mismatches; Phase 5 and Phase 7 passed at 2,617/2,617 and 1,445/1,445; inventory validation passed (6,338 functions, 5,138 data objects, zero unexplained); focused inventory/gate/completion tests passed 251/251, 39/39, and 11/11; public headers passed 80/80. The fixture drives the successful write-lock path; lock-contention reporting is disassembly-backed but not separately exercised.

Task M1.5: Ruling: promote the six fluid-emitter aggregate getters into an actual candidate-DLL public-vtable probe using the candidate linker map and constructed wrapper -- M1 explicitly requires actual-DLL routing, and the current aggregate probe exercises a test-executable copy while resolving only setFlag into the candidate DLL -- cost if wrong: the abstract wrapper's synthetic construction may not cover the disabled factory's unreachable production route, so no claim will be made about fluid creation or the remaining emitter methods. Contract: task-M1.5-brief.md.

Task M1.5: complete (commit fc651e7c; six aggregate getters called through the candidate NxPhysics.dll vtable after candidate-map constructor resolution; exact oracle bytes, hidden result pointers, and stack balance; DLL-only local-position offset mutation caught, restored candidate hash 63F29A35BE21FD9F23E2FA2B333C44F98C0FFBAFEB3A02DC81A001F7D4F73B57 passes direct probe; Phase 5 2,619/2,619, Phase 7 1,447/1,447, Python tools 816 tests + 732 subtests; evidence: docs/reconstruction/novodex-physics/evidence/fluid-emitter-candidate-dll-aggregate-abi-2026-10-09.md). M1 remains open for lifecycle and other harness gaps.

Task M1.6: Ruling: add a runner-level rejection for positive `candidate failures=N` transcript counters before resuming reconstruction — a gate must not accept matching but internally failing oracle/candidate diagnostics, as the historical Phase 5 investigation demonstrated — cost if wrong: a future test that intentionally emits a positive counter would need to represent that separately rather than as an accepted differential.

Task M1.6: complete (commit d7def63d; `run_differential.ps1` rejects positive failure counters on both staged children; zero/positive/unrelated transcript controls pass; full Python suite 819 passed + 732 subtests; Phase 5/7 pair gates intentionally not rerun before mainline integration). Evidence: docs/reconstruction/novodex-physics/evidence/phase5-gate-nonzero-counter-guard-2026-10-09.md.

Task M6.3: complete (`phys_fn_000661`, `NxSceneInternal::addJoint`; whole-row immediate-return mutant rejected by the registered `NxPhysicsJointStagedPairTests` Phase 7 staged-pair differential with oracle exit 0, candidate access violation, `stdout_delta=3282`, exact stderr; restored control exact; Phase 7 gate passes 1,447/1,447; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; focused tools tests pass: inventory 251, completion 11, work units 12, gate targets 39; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-add-joint-000661-mutation.md`.

Task M6.4 ruling: extract `phys_fn_000663` (the 906-byte Scene deleting destructor) as the next bounded M6 lifecycle packet. It is still discovered in the inventory, but `nxSceneDelete` already owns the core teardown path and `NxPhysicsPopulatedSceneTeardownTests` is registered with populated lifetime fixtures; this gives a direct whole-row mutation proof without inventing a new test harness. Cost if wrong: the no-op proves the row is behaviorally load-bearing but does not prove every destructor branch; keep Phase 8 terminal closure open. Brief: task-M6.4-brief.md.

Task M6.4: complete (phys_fn_000663 Scene scalar deleting destructor; the whole-row no-op mutant was rejected by NxPhysicsPopulatedSceneTeardownTests with oracle exit 0, candidate exit 1, stdout_delta=18; restored control exact; full Phase 7 gate passes 1,491/1,457 assertions; inventory 6,338 functions, 5,138 data objects, zero unexplained; completion report tests 11/11; public headers unchanged). Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-destructor-000663.md.

Task M6.5 ruling: prioritize phys_fn_000559 (NxSceneInternal::getNbJoints) over the adjacent empty abstract-base deleting thunk phys_fn_000283; the getter directly affects the public scene count and the registered joint staged-pair target already observes joint lifecycle state, enabling a focused wrong-offset mutation without a new harness. Cost if wrong: the fixture may not assert the count on each lifecycle branch; keep the row open unless the targeted mutation changes its recorded contract. Brief: task-M6.5-brief.md.

Task M6.5: complete (phys_fn_000559, NxSceneInternal::getNbJoints; +0x6c4 wrong-offset mutant rejected by NxPhysicsJointStagedPairTests with count=0 while numerated=1, both exits zero, stdout_delta=296, exact stderr; restored control exact; full Phase 7 gate passes 1,491/1,457 assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion tests 11/11, work-unit tests 12/12; public headers unchanged). Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-joint-count-000559.md.

Task M6.6 ruling: continue with `phys_fn_000561` (`NxSceneInternal::getNbEffectors`) rather than another joint-lifecycle row. Its exact +0x6c4 source read and the registered `NxPhysicsEffectorTests` fixture directly expose the count across create, release, and actor teardown, allowing a low-cost adjacent-field mutation. Keep Phase 8 terminal closure open.

Task M6.6: complete (`phys_fn_000561`; the +0x6c8 joint-count mutant was rejected by registered Phase 7 `NxPhysicsEffectorTests`, both exits zero, exact stderr, `stdout_delta=14`; source restored byte-for-byte and the control was exact; Phase 7 gate passes 1,491/1,457 coverage assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion report, work-unit, and inventory tests pass; full tool suite has seven existing failures in Phase 6 plan/gate metadata and retained oracle-baseline expectations). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-count-000561.md`.


Task M6.7 ruling: select `phys_fn_000565` (`resetEffectorIterator`) because its existing row trace hits nine times in `NxPhysicsEffectorTests`, and the registered fixture explicitly serializes iteration order after create, release, and actor teardown. A cursor-null mutation directly tests this row with no new harness or product behavior change; keep Phase 8 terminal closure open.

Task M6.7: complete (`phys_fn_000565`; null-cursor mutant rejected by `NxPhysicsEffectorTests`, both exits zero, exact stderr, `stdout_delta=14`; source restored byte-for-byte and clean control exact; full Phase 7 gate passes 1,491/1,457 coverage assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-iterator-000565.md`.


Task M6.8 ruling: continue through the effector list consumer `phys_fn_000569` (`getNextEffector`) because the registered `NxPhysicsEffectorTests` target has a recorded 20-hit breakpoint trace and prints the returned iterator identities across populated, empty, and release states. Mutating the non-null return directly tests this row; keep Phase 8 terminal closure open.

Task M6.8: complete (`phys_fn_000569`; null-return mutant rejected by `NxPhysicsEffectorTests`, both exits zero, exact stderr, `stdout_delta=14`; source restored byte-for-byte and clean control exact; full Phase 7 gate passes 1,491/1,457 coverage assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-next-000569.md`.


Task M6.9 ruling: choose `phys_fn_000573` (`removeEffector`) because its registered effector trace is reached during explicit releases and actor teardown. A whole-row no-op directly tests list unlinking; the failure consequence is a freed effector remaining reachable from the Scene list, which the existing fixture can detect without new setup. Keep Phase 8 terminal closure open.

Task M6.9: complete (`phys_fn_000573`; no-op leaves a freed effector linked and `NxPhysicsEffectorTests` rejects it with oracle exit 0, candidate exit -1073741819, stdout_delta=48, exact stderr; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-remove-effector-000573.md`.


Task M6.10 ruling: continue the Scene effector lifecycle with `phys_fn_000575` (`releaseEffectors`). The existing `NxPhysicsEffectorTests` deliberately leaves live effectors for `sdk.releaseScene`, directly reaching this row; an immediate-return mutation should be visible in the allocator teardown record without new fixture behavior. Keep Phase 8 terminal closure open.

Task M6.10: complete (`phys_fn_000575`; immediate-return mutant skips the effector destructor walk and the registered Phase 7 `NxPhysicsEffectorTests` rejects it with oracle exit 0, candidate exit 0, stdout_delta=2, exact stderr; scene-release transcript records four fewer frees; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes at 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-effectors-000575.md`.


Task M6.11 ruling: continue the scene event lifecycle with `phys_fn_000577` (`processJointBreakEvents`). The registered simulation fixture has two direct joint-break callback cases (retain and release); an immediate-return mutation tests the event-drain and dispatch path with the callback-count assertion. Keep Phase 8 terminal closure open.

Task M6.11: complete (`phys_fn_000577`; immediate-return mutant suppresses both joint-break callbacks and `NxPhysicsSimulationTests` rejects it at the callback-count assertion, oracle exit 0, candidate exit 1, stdout_delta=801; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes at 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-joint-break-events-000577.md`.


Task M6.12 ruling: select `phys_fn_000579` (`getDebugRenderable`) because the registered `NxPhysicsSceneVisualizeTests` fixture directly exercises the lazy renderable across world axes, populated geometry, and Scene teardown. A null-return mutation gives a direct, observable test of the row. Keep Phase 8 terminal closure open.

Task M6.12: complete (`phys_fn_000579`; null-return mutant crashes the candidate under `NxPhysicsSceneVisualizeTests` and is rejected with oracle exit 0, candidate exit -1073741819, stdout_delta=869, exact stderr; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes at 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-debug-renderable-000579.md`.


Task M6.13 ruling: choose `phys_fn_000594` (`releaseEffector`) because the existing effector fixture reaches public release repeatedly and exposes the resulting list count, iterator head, and allocator frees. An immediate-return mutation directly tests the full Scene release sequence. Keep Phase 8 terminal closure open.

Task M6.13: complete (`phys_fn_000594`; immediate-return mutant leaves the count at 1, iterator at `np`, and skips frees; registered `NxPhysicsEffectorTests` rejects it with oracle exit 0, candidate exit 0, stdout_delta=48, exact stderr; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes at 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-effector-000594.md`.


Task M6.14 ruling: select `phys_fn_000587` (`createSpringAndDamperEffector`) to close the other half of the Scene effector lifecycle. The registered effector fixture observes successful creation, parameters, list order, release, actor teardown, and Scene teardown; a null-return mutation directly falsifies this row. Keep Phase 8 terminal closure open.

Task M6.14: complete (`phys_fn_000587`; null-return mutant changes creation from yes to no and is rejected by `NxPhysicsEffectorTests`, oracle exit 0, candidate exit 0, stdout_delta=85, exact stderr; source restored byte-for-byte and clean control exact; fresh Phase 7 gate passes at 1,491/1,457; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-create-effector-000587.md`.

Task M6.15 ruling: reconstruct and independently falsify the two loops in `phys_fn_000581`/`000583` (`nxSceneVisualizeCollisionShapes`). IDA shows a static-pruner walk followed by the selected dynamic-pruner walk; the registered Scene visualization fixture can observe both using visible static and dynamic boxes. Keep Phase 8 terminal closure open.

Task M6.15: complete (`phys_fn_000581` static loop and `phys_fn_000583` selected dynamic loop; independent no-op mutations rejected by `NxPhysicsSceneVisualizeTests` with `stdout_delta=1382` and `1322`, respectively, both exits zero and exact stderr; restored control exact; full Phase 7 gate passes 1,496/1,467 coverage assertions; inventory passes with 6,338 functions, 5,138 data objects, zero unexplained; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251; public headers unchanged). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-collision-shape-pruners-000581-000583.md`. Separate sphere/capsule visualization rounding remains open.

Task M6.16 ruling: take phys_fn_000396 (NxScene::checkResults) status-mask behavior because M6 explicitly names result-state behavior, IDA establishes a concise branch contract, and the existing public staged-pair simulation fixture already drives the completion event; adding the missing unrequested-status assertion permits a row-specific mutation without creating a new harness. The umbrella plan has milestone checklists but no packet-level Task 16 heading, so this brief records the smallest dependency-ordered slice under its M6 contract. Cost if wrong: the status-zero input may be outside the intended public API set; it remains pinned to the shipped oracle and is not generalized to undocumented statuses.

Task M6.16: complete (phys_fn_000396 status-mask behavior asserted through public NxScene; changing the bit-clear early return to false was rejected by NxPhysicsSimulationTests, oracle exit 0/candidate exit 1, stdout_delta=3846; restored target exact, stdout_delta=0 and stderr_exact=true; Phase 7 full gate passes 1,649/1,649; inventory 6,338 functions, 5,138 data objects, zero unexplained, Phase 7 84 closed/477 deferred; public headers unchanged at 80/80; all 810 tooling tests pass. Exact mutant and restored staged-pair logs retained under docs/reconstruction/novodex-physics/evidence/raw/phase7-checkresults-status-mask-000396/. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-checkresults-status-mask-000396.md.)

Task M6.17 ruling: select phys_fn_000392 (NpScene::isWritable) because M6 explicitly requires writable-state behavior and the registered public simulation fixture already calls isWritable and records its success result, providing a direct row-specific mutation witness without new test scaffolding. Cost if wrong: the initial successful probe does not cover lock contention; closure is limited to the observed unlocked success branch, and the false-on-contention branch remains backed by the pinned IDA contract until separately exercised.

Task M6.17: complete (phys_fn_000392 successful unlocked path; changing the successful return to false was rejected by NxPhysicsSimulationTests, oracle/candidate exit 0, stdout_delta=2, exact stderr; restored candidate returned stdout_delta=0 with exact stderr; full Phase 7 gate passes 1,649/1,649; all reconstruction tooling tests pass 810/810; inventory contains 6,338 functions, 5,138 data objects, zero unexplained, Phase 7 85 closed/476 remaining; public headers unchanged at 80/80. Exact mutation and restored transcripts are retained losslessly under docs/reconstruction/novodex-physics/evidence/raw/phase7-iswritable-000392/. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-iswritable-000392.md.)


Task M6.18 ruling: close phys_fn_000622 (NxSceneInternal::releaseFluid) against the existing public simulation fixture. IDA shows the row owns manager release dispatch, empty-manager deleting destruction, and Scene-slot clear; the fixture also drives seeded populated-array removal and deleting dispatch. The whole-row immediate-return mutant isolates this row and produces a focused transcript difference. Closure is limited to these observed disabled-backend paths; callback reentry and extension-backed live-fluid notification remain open.

Task M6.18: complete (`phys_fn_000622`; immediate-return mutant rejected by NxPhysicsSimulationTests, oracle/candidate exit 0, stdout_delta=4, exact stderr; the public empty-manager release and seeded array removal/deleting-dispatch cases both changed. Restored control returned stdout_delta=0 and exact stderr. Full Phase 7 gate passes 1,649/1,649; all 810 reconstruction-tool tests pass; inventory reports 6,338 functions, 5,138 data objects, zero unexplained, Phase 7 86 closed/475 remaining; public headers unchanged. Exact mutant and restored staged-pair logs retained losslessly under docs/reconstruction/novodex-physics/evidence/raw/phase7-scene-release-fluid-000622/. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-release-fluid-000622.md.)


Task M7.1 ruling: take phys_fn_003605 (Fluid emitter world-pose recomposition) before its caller phys_fn_003624: the caller directly invokes this helper and the existing callback fixture can observe the complete pose without requiring an enabled FluidModel. This keeps the packet directly falsifiable while building toward the full post-simulation update path. Cost if wrong: the oracle's x87 temporary lifetimes may require a more literal implementation than NxMat34 composition; full-pose differential will expose that before closure.

Task M7.1: complete (`phys_fn_003605`; direct oracle differential covers unattached, asymmetric matrix-frame actor, and asymmetric quaternion-body actor cases, comparing all callback pose words, tuple, return value, and stack balance. Non-binary inputs exposed one-ULP errors; explicit x87 accumulation order and the oracle's translation spill restore bitwise equality. A +1.0 actor-backed world-pose X mutation is rejected with `mismatches=2`; restored control has zero mismatches. Fresh Phase 7 gate passes at 1,653/1,653 and Phase 5 gate passes at 2,626/2,626; all 810 reconstruction-tool tests pass; inventory validates at 6,338 functions / 5,138 data objects / zero unexplained; completion backlog regenerates successfully; all 80 public headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-world-pose-003605.md`. The extension-backed post-simulation behavior remains open.)

Task M7.2 ruling: take the direct body-state publication contract in `phys_fn_003624` next. It is the direct caller of just-closed `phys_fn_003605` and exposes three ordered, independently observable body-vector callbacks with an existing pinned-oracle harness. Use the attached-body branch and an empty-actor branch to falsify the caller without requiring a real enabled FluidModel simulation lifecycle. Cost if wrong: direct callback slot routing may need separate reconstruction because the pinned image does not import FluidModel statically.


Task M7.2: complete (phys_fn_003624; the direct oracle differential covers the no-actor early return and attached body with asymmetric non-binary position/velocity vectors, comparing ordered callbacks, full callback records, return value, and x86 stack balance. The +0x154 position-offset mutant is rejected with mismatches=1; restored control is exact. Fresh Phase 7 passes 1,654/1,654 and Phase 5 passes 2,627/2,627; the full reconstruction tooling suite passes 810 tests; inventory validates at 6,338 functions / 5,138 data objects / zero unexplained; all 80 public Physics headers are unchanged. Viewer CTest passes 48/48 across all 39 available scene entrypoints; five known pinned-oracle asset cases are skipped under their established signatures. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-body-state-003624.md. The extension-backed post-simulation lifecycle remains open.)


Task M7.3 ruling: select `phys_fn_003419` (implicit fluid surface-mesh dispatch) as a compact extension callback with a direct oracle-visible contract. The existing fluid emitter ABI harness can bind the callback IAT and compare exact cdecl arguments, return value, and stack balance; the empty-mesh branch can be checked for no callback without asserting its stale EAX. Keep Phase 8 terminal closure open.

Task M7.3: complete (`phys_fn_003419`; direct pinned-oracle/candidate callback differential matches context, fluid handle, mesh handle, triangle count, return, and x86 stack; empty-mesh case emits no callback and preserves stack. The `/3` to `/6` mutation is rejected with `mismatches=1`; restored control reports zero mismatches. Fresh Phase 5 passes 2,629/2,629, Phase 7 passes 1,656/1,656, inventory validates at 6,338 functions / 5,138 data objects / zero unexplained, and completion backlog regenerates successfully. Public Physics headers remain unchanged at 80/80. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-surface-mesh-003419.md`. Full reconstruction remains open with 2,787 code rows awaiting Phase 8 terminal closure. )

Task M7.4 ruling: take `phys_fn_003663` (`NxImplicitMesh::setMeshData`) as the next direct row. IDA gives a finite validation/copy/error contract, and the existing `NxPhysicsFluidEmitterAbiTests` resolves private candidate symbols from the candidate DLL map and runs in both Phase 5 and Phase 7. Add a private implementation surface only as needed to emit the candidate method symbol; do not change public headers or claim the disabled public implicit-mesh creation path is implemented. Cost if wrong: this isolated method differential will close only the setter, leaving the public feature lifecycle open.

Task M7.4: in progress. The existing SDD workspace and ledger identify this completion plan. The `sdd-workspace` helper could not resolve the worktree's linked Git directory under WSL; continuing in the established plan workspace without changing Git metadata.

Task M7.4: complete (`phys_fn_003663`; private `NpImplicitMesh::setMeshData` matches the pinned oracle for 3 valid and 8 invalid descriptors, exact error records, state preservation, and x86 stack balance. A +0x0c copy-offset mutation is rejected with `mismatches=3`; restored control reports zero. Fresh Phase 5 passes 2,630/2,630 and Phase 7 passes 1,657/1,657; all 810 reconstruction tooling tests pass; inventory validates at 6,338 functions / 5,138 data objects / zero unexplained, Phase 7 94 closed / 467 remaining; completion backlog regenerated; public Physics headers unchanged and verified. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-mesh-data-003663.md`. Full reconstruction remains open with 2,787 code rows awaiting Phase 8 terminal closure.\n
Task M7.5: complete (`phys_fn_000619` and `phys_fn_003475`; two-fluid direct oracle differential and three row-specific mutations; fresh Phase 5 2,633/2,633 and Phase 7 1,660/1,660; 810/810 tooling tests; inventory and both 80-file public-header checks pass; merged to main as PR #7 commit b238993f). Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-post-step-000619-003475.md`.

Task M0.1: complete (regenerated `completion-backlog.json` from current inventory and closure ledgers, byte-identical on repeated generation; regenerated `work_units.json`; raised registered Phase 5/7 floors to exact measured counts; 810/810 tooling tests, Phase 5/7 gates, inventory, and both public-header manifests pass).

Task M7.6 ruling: take `phys_fn_003630` (`FluidManager::stepFluids`) as the next packet because `phys_fn_000659` calls it after each substep and the remaining inventory records only the disabled warning path, omitting dirty static-collision refresh and live-fluid dispatch. Recover `phys_fn_003495` and `phys_fn_003505` as prerequisites before claiming whole-row closure. Cost if wrong: the static-collision/extension branch may be feature-gated in shipped consumers, but it is present in the pinned DLL and required by full reconstruction.

Task M7.6 progress: reconstructed the empty-emitter `phys_fn_003495` substep and the clean-static-collision two-fluid traversal in `phys_fn_003630`. Actual DLL differential checks callback order/payload, timestep bits, state clear, result, and x86 stack. The manager-loop skip mutation is rejected (`oracle_callbacks=2`, `candidate_callbacks=0`, `mismatches=3`); restored control is exact. Phase 5 passes 2,635/2,635 and Phase 7 passes 1,662/1,662; inventory and 80/80 public-header manifests pass. Dirty collision (`003505`) and nonempty-emitter (`003601`) paths remain open, so no row closure is claimed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-substep-003495-manager-clean-003630.md`.

Task M7.6 verification update: fresh Release CMake rebuild and Phase 5/7 gates pass at 2,635/2,635 and 1,662/1,662 against candidate SHA-256 `970bc0866483b8fe0943ff02fe2f8f8db88d50fa134de168b1113c85c054811c`. All 810 reconstruction-tool tests pass. Viewer CTest exercised all 39 registered scenes: 34 passed; five known oracle-baseline asset/PMap failures were skipped by their exact existing signatures. Viewer scene replacement/cancellation/exit also passes. Inventory validates at 6,338 function rows, 5,138 data rows, and zero unexplained; public headers remain 80/80 unchanged. The dirty-collision and nonempty-emitter branches remain open.

Task M7.6 ruling: exercise the enabled bodyless-actor branch of `phys_fn_003601` before attempting the dirty static-collision rebuild. It is a small direct prerequisite with a callback and ABI contract observable against the pinned oracle; this yields useful proof without inventing allocator geometry. Cost if wrong: it covers only part of `003601`, leaving the live-body apply-force branch and the larger `003505` dependency unresolved.

Task M7.6 partial: the bodyless branch now compares the pinned oracle row at RVA `0x881c0` with the candidate DLL's mapped helper. It checks the three callback records and order, force/torque bit patterns, mass, result, and x86 stack. Changing the enabled-check polarity is caught (`oracle callbacks=3`, `candidate callbacks=1`, mismatches=1); restored control is exact. Phase 5/7 coverage registrations now require this assertion. Candidate DLL SHA-256 `CDFAB521682B8B0BA1103145B166E69BA5DD59125AEEDB25F4F0AC6EFB4B03C2`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-body-repulsion-003601.md`. `003601` remains open because no live-body fixture exercises force application.

Task M7.6 ruling: move the live-body repulsion proof to a new isolated staged-pair target. The prior in-process experiment paired candidate Physics with the already-loaded oracle Foundation and faulted during actor record access; the staged-pair test creates each actor with its matching Physics/Foundation modules and compares normalized output across separate oracle/candidate processes. Cost if wrong: another narrowly scoped target and one explicit support DLL in those pair directories, in exchange for avoiding a cross-module object-layout artifact.

Task M7.6 partial: the enabled live-body `phys_fn_003601` path now runs through SDK-created dynamic actors in isolated oracle/candidate pairs. It verifies callback arguments and vector/mass payloads, body application, resulting body-state words, and stack balance. A no-mass-scaling mutant is rejected with both child exits zero and `stdout_delta=2`; restored output is exact. The candidate's previous indexed x87 scaling loop faulted on its first body-backed vector; scalar float operations now match the oracle words. Phase 5 and Phase 7 each register the new isolated differential. Candidate SHA-256 `93515f4a6ea0a9cd492640212638bfac8f565a0564e75929ad4c99eda5221326`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-live-body-repulsion-003601.md`. The complete `003624` nonempty-emitter caller, all `003601` early guards, dirty collision builder `003505`, and full manager row `003630` remain open.

Task M7.6 live-body verification: fresh Release build and Phase 5/7 gates passed at 2,637/2,637 and 1,664/1,664 coverage assertions. The tooling suite passed 810/810; inventory and both 80-file public-header manifests passed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-live-body-repulsion-003601.md`.

Task M7.6 nonempty-emitter follow-up: the actual `003495` oracle and candidate entries now run with one bodyless emitter. The probe checks the `003624` pose callback, `FluidRun` payload, state clear, bodyless `003601` skip, return, and x86 stack. Removing the per-emitter update call is rejected (`pose_callback=0`, `mismatches=1`); restored output is exact. Fresh Phase 5/7 pass at 2,638/2,638 and 1,665/1,665; all 810 tooling tests pass. Inventory and both public-header manifests pass. Candidate SHA-256 `8180eeee170d23d07139445d6e91edb888a454524cdf61d3d5cc6a9d643445b9`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-nonempty-emitter-substep-003495.md`. The attached full caller path, early repulsion guards, and dirty static-collision builder `003505` remain open.

Task M7.6 partial: reconstructed `phys_fn_003514`, the dirty-static-collision builder's bucket-prefix leaf, and added a direct four-record plus empty-range oracle differential. A contribution mutation is rejected (`oracle_total=9`, `candidate_total=13`, `mismatches=1`); the restored test matches both cases including x86 stack. Fresh Phase 5/7 gates pass at 2,640/2,640 and 1,667/1,667; all 810 tooling tests pass; inventory and both immutable 80-file public-header manifests pass. Candidate SHA-256 `a794af9c11567222f7baf9a1a544463990eb481cc092273eeca1294183437e0c`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-bucket-prefix-003514.md`. The static builder `003505` and dirty manager path `003630` remain open; this leaf does not claim closure of either row.

Task M7.6 partial: reconstructed `phys_fn_003516` as the cell-prefix stage following bucket accumulation in dirty builder `003505`. The direct oracle differential covers three bucket rows and three cells, expected prefixes `0,3,4`, pending-count clearing, total `6`, return `3`, unrelated-word preservation, and stack balance; the empty-range case preserves all state. Mutating every output prefix by `+1` is caught (`prefixes=0`, `counts_cleared=1`, `mismatches=1`); the restored control is exact. Fresh Phase 5/7 gates pass at 2,642/2,642 and 1,669/1,669; all 810 tooling tests pass; inventory and both 80-file public-header manifests pass. Candidate SHA-256 `0021acdd295e407fa51ec11c7ac64a3c70fbeddb4c83a2b2f9519b05e55f4c1b`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Dirty builder `003505` and manager path `003630` remain open.

Task M7.6 partial: reconstructed `phys_fn_003511` and `phys_fn_003549` on mainline. `003511` routes 13 static/dynamic/unsupported/excluded/refresh cases with exact counters, returns, and cdecl stack; the dynamic type-2 offset mutant is caught (`mismatches=1`). `003549` walks scene shape nodes and expands type-5 compound members across empty, mixed, and dynamic fixtures; a compound-loop mutant is caught in both nonempty cases (`mismatches=2`). Candidate SHA-256 `b91e40c51c76cb01dc6ec849c57976c786971054d6ea422ffa3c1f336a7e3c34`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Phase 5/7 pass at 2,653/2,653 and 1,680/1,680; full tooling tests pass 810/810; inventory validates at 6,338 functions / 5,138 data objects / zero unexplained. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-shape-counter-003511.md` and `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-scene-shape-count-003549.md`. The direct collector `003555` and enclosing `003505`/dirty manager path remain open.

Task M7.6 partial: reconstructed `phys_fn_003543` (sphere typed-array appender) and `phys_fn_003545` (capsule typed-array appender), direct dependencies of the fluid collision shape router. Sphere tests selected type 1 and ignored type 2; a `+0xe0` to `+0xe4` source mutation is detected (`mismatches=1`). Capsule tests selected type 3 and ignored type 2, distinct geometry words, defined record prefix, stride/count/returns, tail, and x86 stack; sourcing its second geometry word from `+0xe0` is detected (`mismatches=1`). Restored controls match. Candidate SHA-256 `dc7da7a81ba7fde2782423ba5ca8971a8d4fd04a1f6a45653ca8443434ddba28`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Fresh Phase 5/7 gates pass at 2,655/2,655 and 1,682/1,682; all 810 tooling tests pass; inventory validates at 6,338 functions / 5,138 data objects / zero unexplained. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-sphere-append-003543.md` and `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-capsule-append-003545.md`. `phys_fn_003541` mesh, `003547` box, `003551` plane, router `003553`, collector `003555`, and builder `003505` remain open.

Task M7.7 ruling: take `phys_fn_003547` (box typed-array appender) next. It is a direct callee of the router `003553`; closing the remaining shape appenders is a prerequisite for the router and scene collector, which directly advances the dirty-static-collision builder `003505`. The current direct oracle harness can distinguish selected type 2 from ignored shapes and validate the record layout without public-header changes. Cost if wrong: a leaf proof still does not close the router or builder, so those rows remain explicitly open.

Task M7.7 partial: reconstructed `phys_fn_003547` as the type-2 box-array appender. IDA confirms the three geometry words begin at shape+0xe4; the initial +0xe0 assumption was exposed by the oracle differential (oracle words `11223344.99aabbcc.00000000`, candidate `55667788.11223344.99aabbcc`) and corrected before closure. The selected-box and ignored-type cases now match the candidate record prefix, count/return, tail, and x86 stack; changing the third word source from +0xec to +0xe8 is caught (`mismatches=1`). Restored candidate SHA-256 `04e2a65495d126c9e373f7b3bccf491a131c03dde8c7acad316db6a8cf86faca`; mutant SHA-256 `8191738d1e02cd90fc078272513a4319037bc588487c6efa1bb803f53f01bfb4`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Inventory now validates at Phase 7 118 closed / 443 remaining, with 6,338 functions, 5,138 data objects, and zero unexplained. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-box-append-003547.md`.

Task M7.7 ruling: take `phys_fn_003551` (plane typed-array appender) next. It is the remaining primitive appender in the same dependency set consumed by router `003553`, and closing it unlocks meaningful router coverage across the available primitive families. Cost if wrong: the mesh append and callback-backed dirty builder still block the full scene collector and `003505`, so those dependencies remain next in the packet order.
### Fluid-emitter attribute callback packet (phys_fn_003569–003591)

Complete: implemented 12 backend callbacks and six discovered public wrappers without changing public headers; added candidate-DLL/oracle differential and independent mutations for all 18 rows. Focused output: 12 cases, 12 candidate-DLL cases, 24 callback observations, zero mismatches. Fresh verification: Phase 5 2,667/2,667; Phase 7 1,694/1,694; tooling suite 810/810; inventory validation passes (6,338 functions, 5,138 data objects, zero unexplained). The remaining open work is callback lock-rejection/re-entry negative-path coverage and the broader Phase 7 backlog.

### Continuation — Fluid-emitter branch integration and gate repair (2026-10-12)

Integrated the relevant emitter constructor branch with main's later owner/frame getter work, resolving the shared source, fixture, inventory, gate-floor, and work-unit files. The ABI test now loads a byte-verified uniquely named oracle image for fixed-RVA calls and runs only against the pinned oracle pair, avoiding dependency-name rebinding when candidate Foundation is already loaded. It hashes and reports the exact `NxPhysicsCandidate.dll` used for direct candidate-row calls; the standalone differential and phase gate pass, and a deliberately incorrect candidate digest is rejected. Other targets retain their existing CLI. Fresh Phase 5 and Phase 7 gates pass at 2,669/2,669 and 1,696/1,696 assertions; the tooling suite passes 810/810; inventory has 6,338 functions, 5,138 data objects, and zero unexplained bytes. Public Physics headers are unchanged. Full gate logs and ABI transcript are in `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-emitter-merge-20261012/`.

### M3 continuation — Phase 4 PMap compute and export

Rows `phys_fn_002047` (`PenetrationMap::create`) and `phys_fn_002049`
(`NxCreatePMap`) are now independently mutation-falsified through the public
triangle-mesh path. Density 32 matches the oracle at 10,444 bytes/hash
`9a70de00aaf0edd4`; density 80 matches at 144,272 bytes/hash
`1c6814b928a39f10`. A zero-sample mutation in the compute arm changes the
candidate to 4,120 bytes/hash `387b40b2b1eb1513` and is caught with
`stdout_delta=26`; a wrapper early-return mutation is separately caught with
the same output delta. Both restored staged differentials are exact. The fresh
Phase 4 gate passed at 519 evaluated assertions (the recorded floor was 518 at
that run); the floor is now 519 and its 39-test registry suite passes. Inventory
validates at 34 closed / 1,019 deferred for Phase 4 and zero unexplained bytes.
Public Physics headers remain unchanged. Allocation-failure and alternate
OPCODE-tree branches remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase4-pmap-compute-002047.md`, `docs/reconstruction/novodex-physics/evidence/phase4-pmap-export-002049.md`.


### M5 continuation — five-family destructor gate repair (2026-10-12)

The approved private layout uses five final collision-object families with `EmbeddedHookBase` as the secondary base. The implementation and mutation probe already existed; the red gate came from `run_differential.ps1` omitting the expected staged hash and launching the oracle-relative harness a second time against the candidate image. The runner now passes the expected hash, validates the loaded oracle Physics identity, and runs this in-process source-vs-oracle probe once. Focused differential: five secondary destructors, five scalar deleting destructors, and five primary deleting destructors all match with zero mismatches; member-only mutation reports five mismatches. Full Phase 5 gate passes at 2,670/2,670 assertions. Oracle proof manifest now pins the normalized fixture source hash. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-five-family-destructor-model-2026-10-12.md`.
### M7 continuation — bodyless joint-list insertion

Closed `phys_fn_000557` (`NxSceneInternal::pushJointWithoutBodies`). The
standalone staged-pair fixture observes the detached joint as the sole member
of Scene+0x5a0 through the public `setDynamic` path. An immediate-return
mutation omits the insertion and is rejected (candidate exit 1,
`stdout_delta=2`); the restored differential is exact. The fresh full Phase 7
gate passes at 1,710/1,710 assertions. Phase 7 ledger: 197 closed / 363
deferred. Public Physics headers are unchanged. Evidence:
`docs/reconstruction/novodex-physics/evidence/phase7-joint-bodyless-list-000557.md`.
### M7 continuation — `StaticCollisionDestroy` tail forwarding

Closed Phase 7 row `phys_fn_003509` (`0x863f0`). The object-layout oracle
fixture's candidate marker had been hardcoded, allowing a no-forward mutation
to print a mismatch while the row still appeared green. The marker now reports
measured oracle/candidate forwarding, argument, and return status. A no-op
candidate returns zero, loses the callback, and changes the marker; the full
Phase 7 gate rejects it at the required coverage check. Restored Phase 5/6/7
gates pass at 2,678/2,678, 1,607/1,607, and 1,710/1,710. Phase 7 ledger:
198 closed / 362 deferred. Evidence:
`docs/reconstruction/novodex-physics/evidence/phase7-indirect-tail-003509.md`.


Task M7.8 ruling: close `phys_fn_000501` next because its public bounds-plane fixture already checks the row’s six-face behavior and is registered to the Phase 7 gate; a one-face omission is an isolated falsification. Cost if wrong: this closes only the helper’s tested descriptor-driven path and says nothing about other scene creation behavior.

Task M7.8 progress: changing the six-face loop limit to five in a throwaway source copy is rejected (`oracle actors=6`, `candidate actors=5`, `candidate_exit=1`, `stdout_delta=7`). Restored source control matches (`stdout_delta=0`, exact stderr). The Phase 7 closure ledger now has 199 closed / 361 deferred rows; public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-bounds-planes-000501.md`.


Task M7.9 ruling: close `phys_fn_000907` by extending the existing registered visualization differential with a fetched simulation contact pair; this directly reaches the 8-byte contact-pair thunk. Cost if wrong: it proves only current-stamp pair visitation and contact-point rendering for this fixture, not all contact channels or manager lifecycle behavior.

Task M7.9 progress: the plane/sphere contact fixture emits three lines on oracle and restored candidate with exact staged output. A no-op `row000907` mutant emits zero and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=9`, `stderr_exact=False` on the candidate failure path). Phase 7 passes at 1,711/1,711; ledger now records 200 closed / 360 deferred. Public headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-contact-pair-000907.md`.


Task M7.10 ruling: falsify `phys_fn_000863` with the newly registered live contact visualization fixture, which reaches the per-step pair refresh and reads back the contact stream. Cost if wrong: proves reset behavior only for this reported plane/sphere pair path; other narrow-phase stream layouts remain open.

Task M7.10 progress: a no-op cpmActorPairResetStream0863 mutation preserves stale stream records and increases rendered contact lines from 3 to 78 (`both exits=0`, `stdout_delta=87`, exact stderr). Restoring the row returns exact output. Phase 7 remains 1,711/1,711; ledger now records 201 closed / 359 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-contact-stream-reset-000863.md`.


### M7.11 — contact-anchor storage row closure (`phys_fn_000865`)

The registered public NxPhysicsSimulationTests multi-step contact scenarios exercise the friction-patch anchor store. A row-specific early-return mutation changes the staged transcript by 120 bytes (both exits zero, exact stderr); the restored control matches exactly. Phase 7 remains at 1,711/1,711 and records 202 closed / 358 deferred. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-contact-anchor-store-000865.md.
