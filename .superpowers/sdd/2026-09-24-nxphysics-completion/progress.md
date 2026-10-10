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
