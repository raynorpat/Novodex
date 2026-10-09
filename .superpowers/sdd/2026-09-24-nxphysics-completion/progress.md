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
