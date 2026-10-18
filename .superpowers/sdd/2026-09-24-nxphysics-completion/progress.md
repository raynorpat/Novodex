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


### M7.12 — contact-stream visualization row closure (`phys_fn_000869`)

The existing public plane/sphere contact visualization fixture reaches the renderer after 60 fetched steps. A no-op row mutation renders zero contact lines and fails the fixture assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=9`); restored control exactly matches the oracle's three contact lines. Phase 7 remains at 1,711/1,711 and records 203 closed / 357 deferred. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-contact-visualization-000869.md.


### M7.13 — spring-patch material classifier row closure (`phys_fn_000871`)

The registered public simulation contact/friction fixture detects forcing `cpmPatchSpring0871` to classify all patches as spring contact: the transcript changes by 116 bytes with both exits zero and exact stderr. Restoring the two-material flag OR matches exactly. Phase 7 remains at 1,711/1,711 and records 204 closed / 356 deferred. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-contact-spring-patch-classification-000871.md.

### M5 follow-up — approved five-family destructor model revalidated (2026-10-13)

The approved layout, five final shape-family types, oracle destructor differential, and registered member-only negative control were already present on `main`; no implementation diff was needed. Fresh absolute-path `NxPhysicsShapeVtableTests` differential: all five secondary destructor cases and deleting variants match, all five primary deleting destructor cases match, and the member-only mutation yields five mismatches. Fresh full Phase 5 gate rebuilt Release and passed at 2,685/2,685 coverage assertions. Current candidate DLL SHA-256: `ce977bc9988c0af5209f6b7020e15fa81d98f2875dab887def2d2e1996bdb3b7`. Public headers and unrelated dirty paths were left untouched.

### M7.14 — NpScene secondary destructor reconstruction (`phys_fn_000412`, `phys_fn_000414`)

Reconstructed `NpScene` with `EmbeddedHookBase` at `this + 8`, so MSVC emits the oracle's secondary deleting adjustor and the lock links remain at `+0xc/+0x10`. The constructor test directly invokes both secondary-vtable destructor variants on scratch scenes, checks root adjustment, both vptr transitions, and frees (6 nondeleting, 7 deleting), with a member-only negative control. A no-op destructor mutation is rejected (`candidate_exit=1`, `stdout_delta=6`). Inventory and Phase 7 closure ledger now record both rows; Phase 7 closes 214 rows and defers 346. Fresh Win32 Release Phase 7 gate passes at 1,721/1,721. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-secondary-destructor-000412-000414.md`.

### M7.15 — NpScene actor-count row (`phys_fn_000416`)

The existing `NpScene::getNbActors` implementation is now row-mapped to its candidate symbol and exercised explicitly by the registered bounds-plane target for six boundary actors, one ground-plane actor, and seven combined actors. A zero-return mutation exits 1 and is rejected with `stdout_delta=7`; the restored oracle/candidate control is exact. Phase 7 now records 215 closed / 345 deferred. Fresh Win32 Release Phase 7 gate passes at 1,722/1,722. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-getnbactors-000416.md`.

### M7.16 — NpScene actor-array getter (`phys_fn_000418`)

Restored the oracle's read-lock enter/read/leave sequence around `NpScene::getActors`. The registered bounds-plane fixtures verify the returned actor array for six boundary planes, the ground plane, and the combined seven-actor scene. A null-return mutation exits 1 and is rejected with `stdout_delta=5`; restored control is exact. Phase 7 now records 216 closed / 344 deferred. Fresh Win32 Release Phase 7 gate passes at 1,722/1,722. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-getactors-000418.md`.

### M7.17 — NpScene fluid-manager count (`phys_fn_000420`)

Restored the oracle's scene read lock around the optional fluid-manager count read. The registered simulation fixture covers both empty and enabled manager states; a zero-count mutation fails enabled fluid creation and is rejected with `stdout_delta=53`, while the restored control is exact. Phase 7 now records 217 closed / 343 deferred. Fresh Win32 Release Phase 7 gate passes at 1,722/1,722. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-getnbfluids-000420.md`.

### M7.18 — NpScene fluid-array getter (`phys_fn_000421`)

Restored the oracle's scene read lock around `NpScene::getFluids` and its optional manager array read. The registered simulation fixture verifies the enabled public-fluid array and empty manager case. A null-return mutation is rejected with `candidate_exit=1`, `stdout_delta=53`; the restored control is exact. Phase 7 now records 218 closed / 342 deferred. Fresh Win32 Release Phase 7 gate passes at 1,722/1,722. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-getfluids-000421.md`.

### M7.19 — Scene gravity helper (`phys_fn_000507`)

Row-mapped `NxSceneInternal::setGravity` stores the three gravity words at the oracle offsets `+0x520/+0x524/+0x528`. The registered simulation fixture checks the initial and changed gravity values and trajectories. A no-op mutation changes the staged transcript by `stdout_delta=2` with both processes alive; restored control is exact. Phase 7 now records 219 closed / 341 deferred. Fresh Win32 Release Phase 7 gate passes at 1,722/1,722. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-setgravity-000507.md`.

### M7.20 — Actor-pair report clear (`phys_fn_000511`)

Added a public pair-flag regression that stores allocated `NX_NOTIFY_ON_TOUCH`, clears it with zero, and verifies the getter returns zero. A no-op `cpmClearActorPairFlags` mutation leaves `0x8` visible and is rejected (`candidate_exit=1`, `stdout_delta=5`); restoring source byte-for-byte returns an exact control. The Phase 7 row state and closure ledger now record 220 closed / 340 deferred. Raised the Phase 7 coverage floor to 1,723; the full Phase 7 gate passes at 1,723/1,723, the gate-target unittest module passes 39 tests, and inventory validation passes with zero unexplained rows. Evidence and compressed transcripts: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-flags-clear-000511.md`.

### M7.21 — Scene pair-flag getters (`phys_fn_000513`, `phys_fn_000515`)

Added private NxSceneInternal getter paths matching the oracle's same-shape and same-actor validation, diagnostic lines 916 and 948, and valid pair-value lookup. The public actor getter now resolves the internal actor pointers and uses the Scene read lock. The new same-object tests were red before implementation; row-specific no-op mutations are rejected (`stdout_delta=23` and `11`), with both restored differentials exact. Phase 5 passes at 2,685/2,685; Phase 6 at 1,614/1,614; Phase 7 at 1,725/1,725. The Phase 7 ledger now records 222 closed / 338 deferred. Inventory validation and the 39 gate-target unit tests pass.
M7.22 Scene pair-flag setter rows `phys_fn_000589`, `000590`, and `000592` complete. Corrected the Scene call to explicitly select row 000589's `const NxU8*` overload; the older `void*` overload had made the recovered row unreachable. Same-shape and same-actor setter diagnostics plus store/clear behavior are in NxPhysicsPairFlagTests. Independent no-op mutants rejected at stdout_delta=23/26/15, restored focused control exact. Phase 5 passes 2,685/2,685, Phase 6 passes 1,614/1,614, and Phase 7 passes 1,727/1,727. All 814 tooling tests and inventory validation pass (6,338 functions, 5,138 data objects, zero unexplained); both 80-file public-header manifests pass. Public Physics headers unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-flag-setters-000589-000590-000592.md`.


Phase 2 actor-group pair-flag wrappers `phys_fn_000269` and `000271`: moved the existing round-trip fixture to run with two live Scenes so both lock loops execute. Their no-op mutations are rejected at stdout_delta=6 and 2; restored NxPhysicsSDKTests output is exact. Phase 2 closure now records 63 closed / 80 deferred; Phase 2 passes at 7/7 and Phase 3 at 561/561. A fresh Phase 3 link exposed missing FluidCollision.cpp and NpFluidEmitter.cpp sources in NxPhysicsCollisionTests; adding them resolves all seven link errors. Public headers unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-actor-group-pair-flags-000269-000271.md`.

### M6 follow-up — actor-group pair hash rows (`phys_fn_004153`, `phys_fn_004155`)

The registered `NxPhysicsSDKTests` differential now runs in Phase 6 with two live Scenes, exercising pair insertion, forward/reverse lookup, zero replacement, and lookup after releasing one Scene. Independent no-op mutations are rejected at `stdout_delta=2` and `6`; the restored control is exact. Phase 6 closure is 159 closed / 274 deferred. The full Phase 6 gate passes at 1,619/1,619; the fresh Phase 5 gate also passes at 2,685/2,685 with all five destructor families and the five-failure member-only control. Inventory validation passes with 6,338 functions, 5,138 data objects, and zero unexplained rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-actor-group-hash-004153-004155.md`.

### M2 continuation — triangle-mesh release registry miss (phys_fn_000470, 2026-10-14)

Added empty and populated-registry foreign-handle cases to NxPhysicsTriangleMeshApiTests. Oracle reports NXE_INVALID_OPERATION at PhysicsSDK.cpp:569; the previous candidate silently returned. A missing-diagnostic no-op mutant is rejected (stdout_delta=27), restored staged pair is exact. Phase 4 passes at 529/529 assertions; Phase 2 passes at 7/7; inventory passes with Phase 2 at 66 closed / 77 deferred and 6,338 code rows / 5,138 data objects / zero unexplained. Evidence: docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-000470.md and evidence/raw/phase2-triangle-mesh-release-000470-20261014/.

Task M2.6: complete (`phys_fn_000470`; red foreign-handle differential; no-op mutation caught at stdout_delta=27; restored control exact; Phase 2 gate 7/7; Phase 4 gate 529/529; inventory pass; public headers unchanged).


### M2 continuation — public triangle-mesh creation wrapper (phys_fn_000242, 2026-10-14)

The public API fixture rejects a null-return mutant (stdout_delta=26); restored staged-pair control is exact. Existing NxPhysicsSimulationTests exercises Scene lock contention. Phase 4 full gate remains 529/529; Phase 2 now records 67 closed / 76 deferred. Internal mesh creation remains open. Evidence: docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-create-000242.md.\nTask M2.7: complete (phys_fn_000242; mutation caught at stdout_delta=26; restored exact; report and inventory validation pass; public headers unchanged).


### M2 continuation — triangle-mesh release wrapper lock refusal (phys_fn_000244, 2026-10-14)

NxPhysicsTriangleMeshReleaseLockTests verifies a real locked release refusal and successful release after unlock. A no-op diagnostic mutation is caught at stdout_delta=5; restored output matches. Phase 2 advances to 68 closed / 75 deferred; inventory passes, public headers unchanged. Internal mesh release remains open. Evidence: docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-lock-000244.md.\nTask M2.8: complete (phys_fn_000244; mutation caught at stdout_delta=5; restored exact; report/inventory pass).

### M2.9 — Triangle-mesh release reverse-unlock continuation (`phys_fn_000246`)

Expanded the release-lock fixture to two live Scenes. A held lock in Scene 2 forces the wrapper to enter the outlined unwind for Scene 1; an external worker verifies Scene 1 is writable afterward. Replacing the reverse-unlock body with a counter decrement is caught (`stdout_delta=5`, oracle `unwind_first=1`, candidate `unwind_first=0`). Restored oracle/candidate control is exact. Phase 2 passes at 8/8 coverage assertions; inventory records 69 closed / 74 deferred, and public headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-unwind-000246.md`.

Task M2.9: complete (`phys_fn_000246`; two-Scene unwind mutation caught; restored exact; Phase 2 gate passes at 8/8; 39 gate-target tests pass; inventory validates; public headers unchanged).


### M2.10 — SDK material wrapper and reverse-unlock continuation (`phys_fn_000254`, `phys_fn_000256`)

Reconstructed the addMaterial write-lock protocol from the pinned oracle. A two-live-Scene test holds Scene 2's lock and verifies error line 173, index 0, unchanged material count, and a separate probe acquiring Scene 1 after the wrapper unwinds. The no-forward mutant is rejected (`stdout_delta=18`); the no-op reverse-unlock mutant is independently rejected (`stdout_delta=19`); restored control is exact. Phase 2/3/6 gates pass at 10/10, 563/563, and 1,642/1,642. Inventory validates with Phase 2 at 70 closed / 73 deferred; headers unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-sdk-add-material-lock-000254-000256.md`.

Task M2.10: complete (rows 000254/000256; both row-specific mutants caught; Phase 2, 3, and 6 gates pass; 39 gate-target tests pass; inventory has zero unexplained rows).
Ruling: select phys_fn_000732 after the shared simulation helper and Scene lifecycle packets already present in current main — the plan requires sleep/wake behavior through deterministic public simulation, and NxPhysicsSimulationTests already has a sleep scenario, so row-specific mutation can test whether that fixture exercises the saved-velocity branch without adding synthetic layouts. Cost if wrong: an insensitive fixture means we must add a public sleep-then-wake case before closure; do not demote the row on a zero delta.


### M6.19 — Island sleep/wake body-state update (`phys_fn_000732`)

The registered public sleep/wake differential rejects an immediate-return row mutation (candidate_exit=1, stdout_delta=6720; automatic sleeping changes from 1 to 0). The restored source bytes and rebuilt DLL hash match the baseline; restored output is exact. Phase 7 ledger moves to 307 closed / 253 deferred. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-body-sleep-wake-000732.md.

Task M6.19: complete (`phys_fn_000732`; mutant rejected at stdout_delta=6720; restored exact; full Phase 7 gate passes at 1,793/1,793; 831 tool tests and 742 subtests pass; public headers unchanged; inventory/backlog validate).


Ruling for M6.20: `phys_fn_000714` was already mutation-falsified in current main (stdout_delta=5918), so select the adjacent open dirty-island rebuild dispatch `phys_fn_000720`. The public simulation fixture is registered, the row is called by the simulation step, and its rebuild callee has dynamic proof; a row-specific no-op differential will determine whether existing cases actually exercise this branch.


### M6.20 result — Dirty island rebuild dispatch (`phys_fn_000720`)

The immediate-return mutant passed with stdout_delta=0 and exact stderr. The archive source was restored byte-for-byte; the fixture does not hit a dirty-island rebuild, so the row remains open.

### M6.21 — Island joint/contact solver dispatch (`phys_fn_000728`)

The registered friction simulation rejects an immediate-return mutant (oracle_exit=0, candidate_exit=1, stdout_delta=6874); restored exact control is recorded at D:\Novodex-m6-21-728-restored-gate-20261014.log. Phase 7 ledger moves to 308 closed / 252 deferred. Post-merge full Phase 7 passes at 1,793/1,793; 831 tool tests and 742 subtests pass; inventory/backlog and both header manifests pass. Mainline commit d5122e4d contains the packet; row 000720 remains open.


Ruling for M6.22: select `phys_fn_000730`, the 42-byte null-safe traversal that dispatches the now-closed per-body solver row 000728 along each island sleep-group chain. The public friction simulation has detected a no-op of 000728, so it is a strong existing fixture for the caller; the row mutation will confirm the chain path is observable.


### M6.22 — Island body-chain solver traversal (`phys_fn_000730`)

The registered public friction simulation rejects an immediate-return mutation (oracle_exit=0, candidate_exit=1, stdout_delta=6874); the byte-restored and recompiled control is exact. Phase 7 ledger moves to 309 closed / 251 deferred. Mainline commit 7f5a0d6b contains the packet; the post-merge full Phase 7 gate passes at 1,793/1,793 and its compressed transcript is preserved with the raw evidence. Row 000720 remains open.

Ruling for M6.23: close `phys_fn_000720` with a direct private-row DLL probe after the public Scene transition produced no no-op delta. The test resolves the oracle at its pinned RVA and the candidate via `NxPhysics.map`, then checks dirty-bit clearing, traversal marking, and the rebuilt object. Keep the Scene transition as secondary coverage and do not describe it as the detecting path.

### M6.23 — Dirty island rebuild dispatch (`phys_fn_000720`)

The row-specific immediate-return mutation is rejected by the registered `NxPhysicsSimulationTests` differential (`candidate_exit=1`, `stdout_delta=4000`). Baseline and restored output are exact; the direct oracle and candidate probes both report `dirty_cleared=1 traversed=1 island=1 mallocs=4 frees=1 live_delta=3`. Phase 7 moves to 310 closed / 250 deferred. The full gate passes at 1,798/1,798 assertions before and after merge to main (`b7fff5fb`). The public Scene transition fixture remains supplementary because a later body update can mask this row's work. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-dirty-rebuild-000720.md`.

Ruling for M6.24: select `phys_fn_000724`, the reconstructed sleep-group contact-pair linker called by the now-active simulation step. The existing friction/scene simulation exercises the row; an immediate-return mutation determines whether the resulting contact and trajectory state is observable.

### M6.24 — Sleep-group island pair linking (`phys_fn_000724`)

The registered Phase 7 simulation rejects an immediate-return mutation (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=6606`, exact stderr); restored source and DLL output are exact. The changed output includes triangle-mesh box-contact response and broadphase/stack trajectories. Phase 7 now records 311 closed / 249 deferred rows. The latest full Phase 7 gate on the same source and test tree passed at 1,798/1,798; after the fast-forward to main (`a61e6c5d`), every registered Phase 7 differential target passed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-pair-link-000724.md`.

Ruling for M6.25: select `phys_fn_000740`, the reconstructed CCD swept-bounds calculation. Its existing public collision fixture does not expose the row-specific no-op, so add a direct DLL probe to the already-registered CCD target with a controlled sphere result and signed sweep.

### M6.25 — CCD swept bounds (`phys_fn_000740`)

The direct `NxPhysicsCcdSimulationTests` probe verifies the row's valid bit and bounds min=(-6,-6.5,-4), max=(9,9,10). An immediate-return mutation is rejected (`candidate_exit=1`, `stdout_delta=6`); restored source and DLL output are exact. The public CCD scene alone is insensitive. Phase 7 now records 312 closed / 248 deferred. The new direct line is registered in Phases 6 and 7, and the CCD target is added to Phase 7. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-ccd-swept-bounds-000740.md`.


### M6.26 — Dirty island record reset (`phys_fn_000764`)

The registered Phase 7 simulation rejects a row-only immediate return: the oracle exits 0, the candidate access-violates, the transcript differs by 3,898 bytes, and stderr is exact. The byte-restored rebuild matches exactly. Phase 7 moves to 313 closed / 247 deferred rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-record-reset-000764.md`.

Ruling for M6.27: test `phys_fn_000762`, the joint-island linker, by a row-only mutation against the existing registered simulation first; add a direct DLL probe only if that public path does not detect it.


### M6.27 — Joint island linking (`phys_fn_000762`)

The registered Phase 7 simulation rejects a row-only immediate return because the joint-break notification is no longer reported (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=876`). The byte-restored rebuild is exact. Phase 7 moves to 314 closed / 246 deferred rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-island-link-000762.md`.

Ruling for M6.28: select the next falsifiable row from the current Phase 7 ledger.


### M6.28 — Joint contact suppression scans (`phys_fn_000750`, `000752`)

A direct `NxPhysicsSimulationTests` probe seeds joints in one body's list at a time and reports `owner_scan` and `fallback_scan`. Independent scan-only mutations flip the corresponding result and fail the staged differential (`candidate_exit=1`, `stdout_delta=4000`); restored controls match exactly. Phase 7 moves to 316 closed / 244 deferred rows. The shared target adds one required assertion in Phases 5, 6 and 7, bringing their floors to 2,728, 1,653 and 1,802. Evidence: the two joint-contact filter reports.


### M6.29 — Body CCD step update (`phys_fn_000770`)

The immediate-return mutant is rejected by both the registered simulation (`stdout_delta=7134`) and CCD (`stdout_delta=5`, candidate response velocity is out of range); restoring the source returns exact controls. Phase 7 moves to 317 closed / 243 deferred. Evidence: the row 000770 report and raw mutation packet.


### M6.30 — Joint island dissolve (`phys_fn_000778`)

The registered simulation rejects a row-only immediate return (`stdout_delta=4`, both exits 0, exact stderr); joint staged-pair and allocator tests are exact under the mutation. Restored controls match all three targets. Phase 7 moves to 318 closed / 242 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-dissolve-000778.md`.


### M6.31 — Joint island continuation array push (`phys_fn_000780`)

The direct DLL probe checks the joint-array continuation independently; suppressing only the push changes `queued=1` to `queued=0` and fails the registered differential (`candidate_exit=1`, `stdout_delta=4000`). Restored source is exact. Phase 7 moves to 319 closed / 241 deferred. The shared simulation target adds one required line in Phases 5, 6 and 7, bringing their floors to 2,729, 1,654 and 1,803. Evidence: the row 000780 report and raw packet.

### M6.32 — Material combine selector (`phys_fn_000855`)

A direct `NxPhysicsSimulationTests` DLL probe checks all five relevant selector results: average, minimum, multiply, maximum, and unknown-mode maximum. Replacing only the row with an immediate return of the first argument is rejected (`candidate_exit=1`, `stdout_delta=4000`); the restored oracle/candidate differential is exact. Phase 7 moves to 320 closed / 240 deferred. Full Phase 5/6/7 gates pass at 2,730/2,730, 1,655/1,655 and 1,804/1,804. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-material-combine-000855.md` and its raw packet.


### M6.33 — Body-base kinematic destructor (`phys_fn_000799`)

The existing kinematic actor release case detects an empty-body mutation of `DynamicBodyBase::destruct`: allocator traffic drops from six frees to five (`stdout_delta=8`, both exits zero). Restored output is exact. The body-creation differential is now registered to Phase 7; the raised full Phase 7 gate passes at 1,975/1,975. Phase 7 moves to 321 closed / 239 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-body-base-dtor-000799.md`.


### M6.34 — Contact friction parameters (`phys_fn_000861`)

An empty-body row mutation preserves the normal force but zeros the registered simulation friction force (`stdout_delta=266`, both exits zero); the restored differential is exact. Full Phase 7 passes at 1,975/1,975. Phase 7 moves to 322 closed / 238 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-friction-params-000861.md`.

### M7.7 — NpScene scalar deleting destructor (`phys_fn_000423`)

The populated-scene teardown target now asserts that `NpScene` is allocated as 40 bytes and freed on release. The registered no-op mutation replaces the candidate's generated deleting thunk with `mov eax, ecx; ret 4`; it is rejected at the wrapper-free assertion (`candidate_exit=1`, `stdout_delta=28`). The restored staged pair is exact. Inventory passes with Phase 7 at 333 closed / 227 deferred and zero unexplained bytes. A fresh full Phase 7 gate passes at 1,980/1,980. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-scalar-delete-000423.md` and its raw packet.


### M7.8 — Box shape descriptor default (phys_fn_002314)

The row was mis-mapped to the triangle-mesh initializer; IDA's vtable at 0x10108818 identifies NxBoxShapeDesc::setToDefault. The CMake candidate DLL matches all 80 initialized bytes against the oracle. Patching the map-resolved candidate method to ret is caught with 76 mismatches; the restored full layout differential passes. Phase 7 moves to 339 closed / 221 deferred rows. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-shape-desc-default-002314.md.


### M7.9 — Scene owner-pair entry (phys_fn_000517)

The registered Phase 7 simulation reaches the owner-pair helper through row 000521. Changing only 000517's owner load is caught: the candidate retains the root record (stdout_delta=4007). Restoring the source byte-for-byte and rebuilding returns the exact differential. The candidate function also contains continuation 000519; that row remains open. Phase 7 moves to 340 closed / 220 deferred rows. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-owner-pair-record-remove-000517.md.


### M7.10 — Scene owner-pair loop continuation (phys_fn_000519)

Suppressing only the matching-record removal in the row 000519 continuation is caught through the registered Phase 7 simulation (stdout_delta=4007). Restoring and rebuilding returns the exact differential. The candidate helper combines 000517 and 000519, now independently falsified. Phase 7 moves to 341 closed / 219 deferred rows. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-owner-pair-record-remove-000519.md.


### M7.11 — Pair-flag shape index (phys_fn_000525)

The registered Phase 7 pair-flag test exercises actor and shape pairs. Clearing only 000525's shape-ID index is caught by the candidate access violation (stdout_delta=24); restoring and rebuilding returns the exact differential. Continuation 000527 remains open. Phase 7 moves to 342 closed / 218 deferred rows. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-pair-flag-shape-index-000525.md.


### M7.12 — Pair-flag handle translation (phys_fn_000527)

Nulling only 000527's first actor-handle assignment changes objects=0.1 to objects=255.1 and is rejected by the registered Phase 7 pair-flag differential (stdout_delta=2). The restored candidate matches exactly. Row 000525's shape-index setup is independently closed. Phase 7 moves to 343 closed / 217 deferred rows. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-scene-pair-flag-translation-000527.md.


Phase 7 row packet: `phys_fn_000529` (`NxFilterShapePair`) is reconstructed and mutation-falsified. Registered `NxPhysicsCollisionTests` as a Phase 7 oracle differential and raised the Phase 7 coverage floor by its 350 required assertions. An unconditional-allow source mutation produced three wrong filter cases out of seven; the restored source rebuild produced `matrix contact_filter checks=7 wrong=0`. Evidence: `phase7-shape-pair-filter-000529.md`.


Phase 7 row packet: `phys_fn_000531` (`nxSceneAddShape`) is mutation-falsified. Added `NxPhysicsActorShapeMutationTests` to the Phase 7 staged-pair set and increased its coverage floor by 287 recorded assertions. A no-op mutation yields stdout_delta=142; the restored control is exact. Evidence: `phase7-scene-add-shape-000531.md`. The broad simulation target was also checked and did not reach this row, so it is not used as proof.


Phase 7 row packet: `phys_fn_000535` (`nxSceneRemoveDynamicShape`) is mutation-falsified through the Phase 7 actor-shape mutation differential. A no-op returning true crashes the candidate (stdout_delta=220); restored control is exact. Evidence: `phase7-scene-remove-dynamic-shape-000535.md`.


Phase 7 row packet: `phys_fn_000537` (x87 control-word save) is mutation-falsified through the simulation differential. Replacing the `fnstcw` save with zero makes the candidate abort with a floating-point exception (`stdout_delta=3997`); the restored control is exact. Evidence: `phase7-x87-control-word-000537.md`.


Phase 7 row packet: `phys_fn_001639` (`nxIceIdentityPoses`) is mutation-falsified. Registered `NxPhysicsThirdPartyTests` as a Phase 7 oracle differential and raised the coverage floor by 217 assertions. The uninitialized-global mutation yields 7 mismatches; restored control yields 0. Evidence: `phase7-identity-poses-001639.md`.


### M6.43 — PhysicsSDK scalar deleting destructor flag 1

Extended the registered populated-scene teardown target to assert that `PhysicsSDK::release()` frees its 56-byte internal SDK allocation through deleting-destructor flag 1; the direct flag-0 vtable test remains. The pinned oracle and restored candidate match exactly. A candidate-only thunk mutation suppressing the flag-1 free is rejected (`freed=0`, `candidate_exit=1`, `stdout_delta=7`). The initial full-gate failure exposed a stale oracle-proof fixture hash committed after `f7f9c839`; refreshing the hash preserved oracle digest `774d629d`. Clean-checkout Phase 3, 5, and 7 gates pass at 974/974, 2737/2737, and 2867/2867. The user's separate uncommitted fixture edit remains untouched. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-scalar-delete-flag1-000494.md` and its raw evidence folder.

### M6.44 — Plane descriptor validation (phys_fn_000585)

Mapped row 000585 to `NxPlaneShapeDesc::isValid()` through its vtable, the descriptor default routine, and `NX_SHAPE_DESC_LIST`'s `next` pointer at +0x48. The unchanged public header implementation already matches the oracle contract. Added a dedicated direct oracle/candidate DLL differential over ten plane and base-descriptor cases; its runtime false-return mutation is caught on the valid case. The target is registered in Phase 7, raising the coverage floor from 2,867 to 2,868. Focused differential passes with zero mismatches. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-plane-shape-desc-valid-000585.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-plane-shape-desc-valid-000585-20261014/`.

### M2 capsule height rows (phys_fn_000997 / phys_fn_000999)

Mapped the direct capsule height setter/getter rows from IDA and extracted them as named, non-inline `CapsuleShape` methods; public shape handlers now dispatch through those methods. Added a public oracle/candidate marker for requested height, stored half-height, and owner epoch. The setter mutation is caught (`stdout_delta=4`), the getter mutation is caught (`stdout_delta=10`), and the restored control is exact. Phase 5 passes at 2,747/2,747 assertions; inventory and all 816 tooling tests pass. Phase 3 closure is 79 closed / 313 deferred after the Phase 5 discharge. The first full Phase 3 attempt transiently saw the pinned oracle process access-violate in the existing `NxPhysicsPopulatedSceneTeardownTests`; isolated runs then passed twice, and a fresh full Phase 3 rerun passed at 974/974. The box-only teardown fixture does not execute either capsule height row. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-capsule-height-000997-000999.md`.

M2 sphere radius wrappers: `phys_fn_001183` getter read-lock behavior is exercised; Phase 5 differential closes `phys_fn_001185` public setter. Exact restored differential; try-lock mutant stdout_delta=663, getter-bracket mutant stdout_delta=2. Phase 5 2,761/2,761; Phase 3 87 closed / 305 deferred; all 816 tooling tests pass. Evidence: docs/reconstruction/novodex-physics/evidence/phase3-sphere-public-radius-locks-001183-001185.md.


### M7 continuation — `phys_fn_003611` global-to-local emitter pose

IDA maps RVA `0x89410` to the shared emitter global-pose conversion. Added the candidate helper and a registered direct oracle differential for unattached, matrix-frame, and dynamic-body quaternion cases. A local-X translation mutation is rejected in both attached cases; the restored control is exact (3/3 callbacks, 3/3 stack checks). The helper is in `Physics/src/ZzFluidEmitterPose.cpp` so CMake links it after `PhysicsSDK.cpp` and keeps `NxCreatePhysicsSDK` at its required baseline RVA. Phase 5 passes at 2,768/2,768; Phase 7 passes at 2,882/2,882; all tooling tests pass (831 tests, 743 subtests).

### Phase 6 continuation — `phys_fn_003936` Effector base destructor (2026-10-15)

The registered `NxPhysicsEffectorTests` lifecycle reaches the base Effector destructor through the derived deleting-destructor chain. A candidate naked-return mutation suppressing the compiler-generated Observable destructor is caught (candidate access violation, `stdout_delta=75`, exact stderr); restored control is exact. Phase 6 closure moves to 163 closed / 270 deferred, with a fresh gate passing at 1,666/1,666. The approved core-dump date/time differential also passes exact on the fresh Win32 Release build. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-effector-base-destructor-003936.md` and its raw transcript folder.

### Phase 6 continuation — `phys_fn_003938` Effector scalar deleting destructor (2026-10-15)

Added a direct oracle-RVA/candidate-map scratch probe of the abstract Effector scalar deleting destructor for flags 0 and 1. It checks return identity, stack balance, and conditional 0x24-byte free. A candidate no-op allocator-delete mutation makes only the flag-1 probe fail (`mismatches=1`, candidate exit 1, `stdout_delta=12`); restored control is exact. Phase 6 closure moves to 164 closed / 269 deferred; the Phase 6 and 7 coverage floors increase to 1,668 and 2,884. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-effector-deleting-destructor-003938.md`.

### Phase 7 continuation — controller position getter (`phys_fn_002334`, 2026-10-15)

IDA confirms the four-byte row is `lea eax,[ecx+0x28]; ret`, implemented by `NxControllerCore::getPosition` in `Physics/src/Scene.cpp`. The registered simulation staged-pair target already reads two controller positions and the post-move position. A candidate-only offset mutation to `+0x2c`, followed by an explicit `NxPhysics` rebuild, corrupts the returned coordinates and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=62`, exact stderr); restoring and rebuilding the getter returns an exact differential. Phase 7 closure moves to 353 closed / 207 deferred with no new coverage assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-controller-getposition-002334.md` and its raw transcript/patch folder.

### Continuation packet — lock-API worker-adjacent row 002392
Task: complete (b655c10d, merged to local main). Fresh Release build passed; clean Phase 7 passed at 2,895/2,895, including 002392 mismatches=0; tool suite 816/816; inventory unexplained=0; raw evidence manifest verified. The user-local main test edits remain unstaged and outside the proof baseline.

### Task 1 — inspect adjacent unresolved lock-API row 002394
Status: in progress. Validate the current plan/base and recover RVA 0x5b980 from the pinned oracle with IDA before selecting a differential. Do not edit public headers. Task choice is based on the completed 002392 destructor's documented callers and adjacency; abandon if the callee does not provide a bounded, independently testable reconstruction slice.

Task 1: complete — IDA established 0x1005b980..0x1005b99b is the 30-byte scalar deleting thunk referenced by vtable 0x1010886c. It saves this, calls 002392, tests flags bit 0, calls the oracle CRT delete thunk at 0x100f41f0 only when set, returns this, and ret 4. This is a directly testable continuation of the worker-record destructor.
Ruling: implement 002394 as a private ABI-compatible thunk that calls 002392 and C++ global operator delete only for flag bit 0 — IDA identifies the oracle call as its CRT operator delete; matching the candidate CRT via ::operator delete is the correct translation and avoids hardcoding an allocator. Cost if wrong: cross-CRT heap behavior or flag ABI could differ; the paired flag-0/flag-1 fixture and branch mutation will test this.

Task 2: in progress — add the oracle/source differential first; verify the test fails at link for the missing row helper before implementation.

Task 2: complete — private helper and oracle/source fixture added; the missing-helper red link was confirmed before implementation. A null-return mutant was rejected with mismatches=2, and the restored helper reported mismatches=0. The owner delete flag branch was also mutation-checked (the inverted branch triggered heap corruption). Fresh Phase 5/6/7 gates pass at 2,780/1,680/2,896; all 816 tool tests pass; inventory validator reports unexplained=0. Immutable public header checks pass. Evidence and compressed raw logs: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-scalar-delete-002394.md` and `evidence/raw/phase7-lock-api-worker-scalar-delete-002394-20261015/`. Ready to commit and fast-forward local main.

### Task 3 — worker-record base constructor (`phys_fn_002377`)
Status: in progress. IDA shows the base constructor at RVA 0x5b820 installs vtable 0x1010886c, allocates 0x14 bytes through the Foundation allocator, clears the first two dwords and byte 8, and returns `this`. Build a direct oracle/candidate fixture with distinct patterned allocators so the allocator call and exact initialized bytes are observable. Keep all changes private; do not modify SDK headers.

Task 3: complete — IDA-confirmed base constructor, allocator-aware oracle/source fixture, and byte-8 mutation proof are recorded. Mutation reports mismatches=1, restored control mismatches=0. Fresh Phase 5/6/7 gates pass at 2,781/1,681/2,897; all 816 reconstruction-tool tests pass; inventory validator reports unexplained=0. Ready to commit and fast-forward local main.

### Task 4 — derived lock-API worker constructor (`phys_fn_002396`)
Status: in progress. IDA at RVA 0x5b9a0 shows a derived constructor that delegates base initialization to 002377, stores three stack arguments at owner+8/+0xc/+0x14, installs vtable 0x10108894, and returns this while leaving +0x10 untouched. The direct allocator-shim fixture and one-field mutation distinguish this row from its base constructor and the already reconstructed setter 002398.

Task 4: complete — direct oracle/source fixture passes and the third-argument bit-flip mutation is rejected (`mismatches=1`, exit 1); restored source is exact (`mismatches=0`). Fresh Phase 5/6/7 gates pass at 2,782/1,682/2,898 assertions. The reconstruction tooling suite passes all 816 tests; inventory validation reports unexplained=0. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-construct-002396.md` and compressed logs/manifest in `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-worker-construct-002396-20261015/`. Public SDK headers remain unchanged.

### Task 5 — phys_fn_000645 Scene fluid creation
Status: complete. Base 854ba2faa052dcd264051675da37b51a13a4dc71. The row-specific disabled/enabled fluid assertion is mutation-falsified and the restored staged differential matches exactly. Fresh Phase 5/6/7 gates pass at 2,787/2,787, 1,688/1,688, and 2,904/2,904; the reconstruction-tool suite passes all 816 tests; both public-header trees match the 80-file manifest; inventory reports unexplained=0. Full-DLL work remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-fluid-create-000645.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-scene-fluid-create-000645-20261015/`.


### Task 6 — phys_fn_003649 FluidManager null backend result
Status: focused branch coverage complete; inventory row remains partial/open. The enabled FluidModel fixture can return null from FluidCreate for a valid descriptor. The row-specific simulation test requires retained wrapper publication, internal/public manager-array identity, manager cleanup, and no FluidDestroy callback for a null handle. A candidate early-return mutation is rejected (stdout_delta=60); restored control is exact. Fresh Phase 5/6/7 gates pass at 2,788/2,788, 1,689/1,689, and 2,905/2,905 assertions; all 816 tool tests pass; immutable public headers pass and inventory reports unexplained=0. The first Phase 5 attempt had one transient oracle child timeout in the pruner-pool growth-OOM test; isolated and complete reruns passed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-create-null-backend-003649-20261015/`.

### Task 7 — `phys_fn_003649` descriptor-validation matrix
Status: descriptor-validation corpus integrated and all Phase 5/6/7 gates pass; `phys_fn_003649` remains partial/open. Expanded the simulation fixture to 48 invalid descriptors; the earlier 35-case `desc.isValid()` bypass mutant was rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=73`). The approved object-layout fixture edit and its source-hash baseline are committed, with all 14 retained proof records rebound to the new baseline hash. Phase gates pass at 2,788/2,788, 1,689/1,689, and 2,905/2,905 assertions; the tooling suite passes 831 tests and 743 subtests; headers and inventory checks pass. An initial Phase 6 allocator mismatch was transient; the direct object-layout rerun and complete Phase 6 rerun pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md`.

### Task 8 — `phys_fn_003423` particle cell quantization

Status: partial reconstruction integrated on main. IDA at RVA `0x00084a80` confirms the oracle rounds position/velocity/lifetime byte strides down to multiples of four, stores one rounded float reciprocal of cell size, multiplies each coordinate, then floors the product. `NxPhysicsFluidStrideAlignmentTests` supplies non-multiple strides (13, 13, 5), confirms oracle stepping (12, 12, 4), and rejects a mutation that removes the stride masks. `NxPhysicsFluidCellRoundingTests` distinguishes floor from truncation: the oracle stores `2,-3` for scaled inputs `2.75,-2.75`, while the original candidate stored `2,-2`; a `truncf` mutation is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). `NxPhysicsFluidCellReciprocalTests` exercises a precision boundary where the oracle stores `70` but direct division stores `71`; replacing reciprocal multiplication with direct division is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`). `NxPhysicsFluidNullParticleCountTests` verifies the early return when the count pointer is null; removing the guard is rejected. `NxPhysicsFluidCellSizeZeroTests` poisons the cell words and proves the oracle writes zeroes when cell size is zero; removing the candidate stores is rejected. A continuation audit clears x87 status in the cell-size callback and confirms the zero-cell oracle/candidate transcript agrees exactly (`fpu-status=0001`, `divzero=0`); the candidate now follows the oracle's count guard before computing the reciprocal. All restored differentials are exact. The helper maps to `nxFluidInitializeParticles` at candidate RVA `0x10067cb0`. Existing fixtures also cover standard strided channels, clipping, zero-count allocation, null backend return, allocation failure, array growth/append, and failed growth. The row remains `reconstructed` and deferred for the full instruction-level audit. Phase 5/6/7 pass at 2,791/2,791, 1,692/1,692, and 2,913/2,913. All 835 tooling tests and 743 subtests pass. Inventory validation reports zero unexplained rows. The approved layout fixture source hash is integrated and Phase 5 passes its pin. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-rounding-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-reciprocal-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-stride-alignment-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-null-count-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-size-zero-003423.md`, and the raw `evidence/raw/phase7-physfn-003423*` packets.

### Task 9 — `phys_fn_003626` FluidKey resolver

Status: complete on `main` at verified source state. Added a direct oracle/candidate differential to `NxPhysicsFluidEmitterAbiTests`, isolated from machine-wide registry state. Eleven inputs cover three licensed masks, unknown and malformed-but-safe tokens, numeric suffix/fourth-token parsing, wrong value type, missing value and missing product key. The source-to-oracle comparison matches (`input_digest=04893d50`, result digest `8d75d855`); a zero-return candidate mutation is rejected with five mismatches, and the restored control is exact. Updated the target oracle-only proof, refreshed the shared proof baseline bindings, added the Phase 5/7 coverage markers, mapped and dynamically gated the inventory row, and updated Phase 7 counts to 383 closed / 177 deferred. Fresh Phase 5/7 gates pass at 2,799/2,799 and 2,921/2,921; all 816 tooling tests pass; inventory has zero unexplained bytes. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-license-resolver-003626.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-license-resolver-003626-20261016/`.

### Task 10 — `phys_fn_003649` FluidCreate argument map

Status: complete as a behavior slice; the full row remains open. Added distinct descriptor values to the enabled simulation fixture and verify all fourteen non-pointer arguments captured by the FluidModel callback. Oracle and restored candidate match (`digest=1d2e206d`); swapping the candidate's word-19 source to word 18 is rejected (`candidate_exit=1`, `stdout_delta=64`). Registered the marker for Phases 5/6/7 and raised their floors to 2,800, 1,693, and 2,922. The three fresh gates pass at those floors; all 816 tool tests pass; inventory validation reports `unexplained=0`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-create-argument-map-003649-20261016/`.

### Task 11 — `phys_fn_003653` / `phys_fn_003655` implicit-mesh post-step copy

Status: complete on main. Extended the licensed surface-mesh simulation fixture to verify callback-backed index, vertex-position, and vertex-normal copies plus output counts after one fetched simulation step. IDA identifies the tested routine as the function spanning census rows 003653 and 003655. The staged oracle/candidate control matches exactly. A candidate early-return mutation is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=66`); restoring the helper returns exact output. Registered one coverage assertion in Phases 5/6/7, mapped and dynamically gated both rows, and updated Phase 7 closure to 385 closed / 175 deferred. Full phase gates pass at 2,804/2,804, 1,697/1,697, and 2,926/2,926; the tooling suite passes 835 tests / 743 subtests. Particle-stream row 003651 remains open; these rows remain nonterminal for the full-image audit. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-post-step-copy-003653-003655.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-post-step-copy-003653-003655-20261016/`.

### Task 12 — `phys_fn_003651` fluid post-step particle copy

Status: dynamically gated on main. Extracted the IDA-confirmed 228-byte copy routine into `nxFluidCopyPostStepParticleData`. The existing two-fluid oracle harness now asserts backend-count and caller-maximum caps, position/velocity/flags copies, independent strides, untouched padding, and a null output pointer. The restored control reports `mismatches=0`; a no-op mutation of only the helper is caught with 20 row-marker mismatches and 22 aggregate mismatches (candidate exit 1). The row-specific marker is registered for Phases 5 and 7. Candidate map symbol is `?nxFluidCopyPostStepParticleData@@YAPAXPAXPAI@Z` at `0x100677d0`. Fresh Phase 5/6/7 gates pass at 2,805/2,805, 1,697/1,697, and 2,927/2,927; all 835 tooling tests and 743 subtests pass; inventory validation reports unexplained=0; public headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-copy-003651.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-particle-copy-003651-20261016/`.

### Task 13 — fix the Phase 5 coverage floor and verify row 000445
Status: complete on main. Phase 5 had been incorrectly raised to 2,813 although its registered targets do not include `NxPhysicsSceneVisualizeTests`; the actual 2,812 assertions were correct. Restored Phase 5 to 2,812 and Phase 6 to 1,703, retaining the new visualization assertion only in Phase 7 at 2,941. Fresh CMake Win32 Release phase gates pass 2,812/2,812, 1,703/1,703, and 2,941/2,941. The complete reconstruction tooling suite passes 816 tests; gate-target tests pass 39/39; inventory has 6,338 functions, 5,138 data objects, and zero unexplained rows. Regenerated `completion-backlog.json` to match the approved Phase 2 closure ledger hash. The earlier failing Phase 5 transcript is preserved as a compressed diagnostic. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-debug-renderable-clear-000445.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase2-clear-debug-renderable-000445-20261016/`.

### Task 14 — `phys_fn_003423` sub-word stream strides
Status: implementation slice complete; full row remains open. Extended `NxPhysicsFluidStrideAlignmentTests` with 3-byte position, velocity, and lifetime streams. The oracle rounds each stride to zero, repeats the first position for both particles, and leaves optional channels zero. A raw position-stride candidate mutation fails (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4`); restored output is exact. Phase 7 passes 2,942/2,942, gate-target tests pass 39/39, and inventory validation reports zero unexplained records. Public headers and the production implementation are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-stride-subword-003423.md` and the adjacent raw packet.

Task 15: complete — `phys_fn_000668` (`NxSceneInternal::scalarDeletingDestructor`). The initial direct candidate probe was red on `result_self=0`; the private candidate method now returns `this`. The dedicated staged-pair test matches the oracle for flags 0 and 1, including internal allocation preservation/free and x86 stack balance. Disabling only the candidate flag-1 free is rejected (`oracle_exit=0`, candidate_exit=1, stdout_delta=2). Phase 5 passes 2,860/2,860; Phase 7 passes 2,996/2,996; inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows; both pinned public-header trees pass; all 817 tooling tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-scalar-delete-000668.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-scene-scalar-delete-000668-20261017/`. Full DLL reconstruction remains open.

Ruling: the ledger's earlier M7.7 pointers are stale; current inventory rows, Phase 7 closure state, row-specific evidence, and regenerated completion backlog take precedence. Do not redo rows already closed on main.

### Phase 7 continuation — persistent pruner pair refresh (`phys_fn_001971`)

Completed on main. IDA oracle RVA 0x4c100 maps to candidate helper `cpmRefreshPairCandidate` at 0x10017220. The new isolated coherent-broadphase simulation checks retained pair nodes and current-frame stamps across two fetched frames. Mutating the candidate stamp to `frame ^ 1u` is caught by the registered simulation differential (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4100`); restored output is exact. Phase 7 closes 439 rows / defers 121; fresh Phase 5/6/7 gates pass at 2,862, 1,729, and 3,001 assertions. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Full DLL reconstruction remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-pruner-pair-candidate-refresh-001971.md` and its raw archive.

Maintenance verification: refreshed the shared oracle-proof baseline bindings and regenerated `work_units.json` after the current mainline baseline/inventory changes; the complete reconstruction-tool suite now passes 817/817 tests.


### Phase 7 continuation — scene pruner descriptor setup (`phys_fn_001973`)

Mapped to `nxSceneApplyDescriptorFlags` at `0x1006a570` in Scene.obj. The existing simulation markers cover all selector mappings; changing only `NX_BROADPHASE_FULL` from engine mode 2 to 3 is caught (`stdout_delta=2`), and restoring returns an exact differential. Phase 7 closes 440 rows / defers 120. Inventory validation reports zero unexplained rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-pruner-descriptor-001973.md` and its raw archive. Full DLL reconstruction remains open.

Verification for `phys_fn_001973`: full Phase 7 passed 3,001/3,001; all 817 reconstruction tooling tests passed; inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows.


### Phase 3 continuation — shape global-orientation getter (`phys_fn_001065`)

Mapped the IDA row at RVA `0x00023430` to the read-locked shape global-orientation wrapper and the candidate `nxShapeHandleGetGlobalOrientation` handler. The existing rotated actor/shape fixture emits the same 3x3 matrix for oracle and candidate. A temporary zero-fill mutation was rejected by `NxPhysicsActorDynamicSetterTests` (`stdout_delta=4`); restored output is exact (`stdout_delta=0`). Updated the inventory, Phase 3 closure ledger/program counts to 109 closed / 283 deferred, and regenerated the completion backlog and work units. Full Phase 5 passes at 2,913/2,913 assertions; all 823 reconstruction-tool tests pass; inventory validation reports zero unexplained rows. Public SDK headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-shape-global-orientation-001065.md` and `evidence/raw/phase3-shape-global-orientation-001065-20261018/`. Full DLL reconstruction remains open.

### Phase 3 continuation — box shape dimensions (`phys_fn_001069`)

IDA maps RVA `0x000234c0` to the Scene-write-locked box `setDimensions` wrapper and its internal hull/owner update. The existing `NxPhysicsActorDynamicSetterTests` fixture verifies dimensions, dirty state, bounds and descriptor output. A no-op mutation of `nxBoxHandleSetDimensions` is rejected (`stdout_delta=10`), and the restored control is exact (`stdout_delta=0`). Updated Phase 3 counts to 110 closed / 282 deferred and regenerated the backlog/work units. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-box-shape-dimensions-001069.md` and `evidence/raw/phase3-box-shape-dimensions-001069-20261018/`.


### Phase 3 continuation — box world OBB read lock (`phys_fn_001073`)

The public box world-OBB getter now reproduces the oracle read-lock enter/call/leave sequence. A seeded lock-state marker agrees on oracle and candidate (`0.1`); omitting the guard pair is rejected (`stdout_delta=2`), and restored output is exact. Phase 3 now records 111 closed / 281 deferred functions. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Phase 5 passes 2,914/2,914 coverage assertions and all 823 reconstruction-tool tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-box-world-obb-lock-001073.md` and `evidence/raw/phase3-box-world-obb-lock-001073-20261018/`. Full DLL reconstruction remains open.


### Phase 3 continuation — box secondary deleting-destructor adjustor (`phys_fn_001077`)

IDA shows the 8-byte secondary-base thunk subtracting `0x0c` from `ECX` before jumping to the box destructor body. The production NxPhysics map resolves the compiler-generated equivalent in ObjectModel.obj. The current `NxPhysicsShapeVtableTests` staged oracle run exercises all five family secondary slots: restored mismatches=0, while the member-only negative control reports mismatches=5. Phase 3 now records 112 closed / 280 deferred; inventory validation reports zero unexplained rows. Full DLL reconstruction remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-box-secondary-dtor-adjustor-001077.md` and `evidence/raw/phase3-box-secondary-dtor-adjustor-001077-20261018/`.


### Phase 3 continuation — five-family secondary destructor adjustors

Closed the Capsule, Plane, Sphere, and Mesh adjustor rows `phys_fn_001125`, `phys_fn_001161`, `phys_fn_001195`, and `phys_fn_001243` using the current five-family secondary-vtable differential. IDA confirms each `-0x0c` adjustment and jump to its destructor body; candidate map symbols resolve in ObjectModel.obj. All five restored families match (mismatches=0), and the member-only negative control fails all five (mismatches=5). Phase 3 now records 116 closed / 276 deferred. Inventory validation reports zero unexplained rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-five-family-secondary-dtor-adjustors-001077-001243.md`.


### Task 25 — locked-array data getter (`phys_fn_003479`, 2026-10-18)

The helper now reproduces the oracle's wrapped unsigned span test and begin-pointer return. Eight direct cases cover empty, sub-word, word, multiword, and wrapped spans; the restored differential matches with balanced x86 stack, while an unconditional null-return mutation is rejected with four mismatches. Updated inventory, Phase 7 closure, baseline proof bundle, work units, completion backlog, and registered coverage floors. Fresh Phase 5/6/7 gates pass at 2,931/2,931, 1,782/1,782, and 3,067/3,067; all 842 reconstruction-tool tests and 743 subtests pass. Public headers are unchanged. Full DLL reconstruction remains open.
