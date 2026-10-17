# NxPhysics Full Reconstruction Completion Plan

> **For agentic workers:** Use `superpowers:executing-plans` for bounded execution packets. Use `superpowers:subagent-driven-development` when parallel agent execution has been selected. Steps use checkbox syntax for tracking. This document is the program-level completion plan; recover each packet's machine-level contract before writing its implementation.

**Goal:** Finish a source-built, semantically and ABI compatible reconstruction of the entire pinned Win32 `NxPhysics.dll`, including rarely used paths, while preserving every public header and the existing CMake build.

**Architecture:** Continue the existing stable-ID census and Foundation-style C++ implementation. Execute dependency-ordered slices across the historical phases, connecting recovered internals to the real DLL's public interfaces and proving each slice against the shipped binary. Keep phase ownership stable unless evidence corrects it; distinguish passing a partial phase gate from completing the DLL.

**Tech stack:** CMake, MSVC Win32 Release, existing C++ public headers, reconstructed NxFoundation, pinned Ghidra 12.1.2 and Capstone 5.0.6, Python evidence tooling, PowerShell isolated-process runners. Use IDA or angr selectively when they answer a specific unresolved question.

**Status, updated 2026-10-16:** Full-DLL semantic and ABI reconstruction remains open: 2,786 nonterminal code rows, 3,552 classified compiler artifacts, and 5,138 classified data objects. Current function ledgers: Phase 2 88 closed / 55 deferred; Phase 3 87 / 305; Phase 4 43 / 1,010; Phase 5 205 / 0; Phase 6 169 / 264; Phase 7 400 / 160. Fresh Win32 Release gates pass at Phase 5 2,825/2,825, Phase 6 1,714/1,714, and Phase 7 2,958/2,958 registered assertions. The full reconstruction-tool suite passes 835 tests and 743 subtests; the full-image x86 cleanup audit resolves 278/278 sites with no convention mismatches, inventory validation reports zero unexplained records, and the 80-file Physics header manifest matches both worktree and Unreal source. M1 and the full-image M8 audit remain open; public headers remain unchanged and CMake remains the build path.

**M1 ABI audit continuation, 2026-10-15:** The static x86 cleanup scan now resolves all 278 cast sites with no unresolved rows or mismatches. The prior 0x13000 unresolved cast is proven by a byte-pinned direct tail jump to the inventory-bounded 0x116b0 row; its bare `ret` matches the declared `thiscall(void*)`. Unresolved rows now fail the audit, and every phase runner executes it against the oracle DLL. The fresh Phase 5 runner passes at 2,790/2,790, and the full tooling suite passes 835 tests / 743 subtests. This closes the static cleanup audit only; M1 remains open for runtime nonvolatile/stack probes and complete internal adapter coverage. Evidence: `docs/reconstruction/novodex-physics/evidence/abi-direct-tail-cleanup-000649-2026-10-15.md`.

### Continuation — fluid descriptor `userData` initialization (`phys_fn_003503`, 2026-10-15)

The public fluid constructor fixture now gives `NxFluidDesc::userData` a sentinel and verifies it appears in the returned `NxFluid` object's base member. The candidate wrapper had remained zeroed; initializing its `+4` member makes the oracle and candidate exact. Removing the store is rejected by the registered simulation differential (`stdout_delta=2`); restored output is exact (`stdout_delta=0`). Phase 5 passes at 2,790/2,790. Because the target is shared, Phases 6 and 7 floors also advance by one; the gate registry tests confirm 1,691 and 2,907. All 816 reconstruction-tool tests pass; inventory validation reports `unexplained=0`; public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-userdata-003503.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-userdata-003503-20261015/`.

After the shared assertion update, fresh Phase 6 and 7 gates also pass at 1,691/1,691 and 2,907/2,907. Full runner logs are retained in the local build directory for this verification turn.

### Continuation — implicit-mesh post-step copy (`phys_fn_003653` / `phys_fn_003655`, 2026-10-16)

The registered simulation fixture now creates a licensed callback-backed surface mesh, writes indexed data into caller buffers during a fetched simulation step, and checks the copied index, position, normal, vertex-count, and triangle-count values. IDA maps this copy routine to the single function spanning census rows `003653` and `003655`; row `003657` dispatches to it after retrieving the mesh callback. Oracle and restored candidate match exactly. Making `nxFluidCopyImplicitMeshData` return immediately is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=66`); the restored control returns to exact output. Phase 5/6/7 coverage floors each advance by one; Phase 7 moves to 385 closed / 175 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-post-step-copy-003653-003655.md` and its raw logs.

### Continuation — fluid post-step particle copy (`phys_fn_003651`, 2026-10-16)

IDA at RVA `0x0008a9a0` confirms the separate 228-byte helper copies position, velocity, and flags from 92-byte backend particle records into the corresponding `NxParticleData` outputs, respecting the caller maximum, backend count, independent byte strides, and null output pointers. Extracted the operation as `nxFluidCopyPostStepParticleData` and strengthened the existing two-fluid oracle fixture to check all words and untouched padding, including both count caps and a null flags output. The restored direct oracle/candidate differential matches exactly; a no-op mutation of this helper is rejected (`mismatches=22`). Registered the row-specific assertion in Phases 5 and 7, mapped and dynamically gated the row, and updated Phase 7 to 386 closed / 174 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-copy-003651.md` and its raw logs.

### Continuation — initial-particle aliased-channel ordering (`phys_fn_003423`, 2026-10-16)

IDA at RVA `0x00084a80` shows the initializer completes every position/cell/sentinel write before its optional velocity and lifetime passes. A new fixture aliases the ParticleArray's backing storage with the caller position buffer so this ordering is observable: the oracle's first-pass zeroing changes the next position X to `00000000`, while the former candidate interleaving copied `-6.75` from the velocity stream before reading that position (`stdout_delta=2`). Splitting candidate writes into the oracle's three passes restores exact output; putting them back in the per-particle loop is rejected by the registered differential. Phase 5/6/7 gates pass at 2,806/1,697/2,928. Phase 7 now records 387 closed / 173 deferred; `phys_fn_003423` remains in the full-image Phase 8 audit. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-alias-order-003423.md` and its raw logs.

### Continuation — fluid constructor default mutation (`phys_fn_003497`, 2026-10-16)

Replayed the registered seven-fluid `NxPhysicsSimulationTests` constructor differential against the current candidate: the restored output is exact. Setting `fluid+0x74` to one after the candidate constructor zeroes the 0x84-byte Fluid object is detected with `stdout_delta=2` while both processes exit normally. The restored control is exact, so `phys_fn_003497` is now closed in the Phase 7 mutation ledger. Its full-image instruction/code-generation and solver-base constructor audits remain open. Phase 7 moves to 388 closed / 172 deferred; no coverage floor changes. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-constructor-003497.md` and its raw logs.

### Continuation — spring-force branch matrix (`phys_fn_003970`, 2026-10-16)

Extended the public spring-effector simulation to six distance/configuration cases covering stretch and compression ramps, saturation limits, the relaxed-length boundary, and zero configured force. The fixture records complete linear-velocity vectors after two fetched steps. Oracle and restored candidate match exactly; independent stretch-ramp and compression-ramp mutations are rejected (`stdout_delta=6` and `2`). The shared simulation marker raises fresh Phase 5/6/7 coverage floors to 2,807/1,698/2,935. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-spring-force-branches-003970.md` and `evidence/raw/phase6-spring-force-branches-003970-20261016/`. This is a row-specific branch proof, not completion of the whole DLL.

### Continuation — two-body effector force dispatch (`phys_fn_003979`, 2026-10-16)

Added a public simulation with two dynamic actors connected by the spring effector. After two fetched steps the pinned oracle reports equal-and-opposite x velocities (`408e38e3` and `c08e38e3`). Disabling the candidate's second-body force dispatch leaves actor 2 at zero and is caught by the staged-pair differential (`stdout_delta=2`); restored output is exact. Phase 5/6/7 coverage floors rise to 2,808/1,699/2,936. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-two-body-force-003979.md` and `evidence/raw/phase6-spring-force-two-body-003979-20261016/`.

### Continuation — mass-frame scale and merge rows (`phys_fn_000837`, `phys_fn_000839`, 2026-10-16)

Added oracle and actual-candidate-DLL comparisons for six scale factors and 17 merges. The candidate helper addresses are resolved from the candidate's own `NxPhysics.map`. Each row's DLL-only mutation is rejected; the restored scale and merge digests are `4a90e1ae` and `91e369ad`. The existing public actor-mass scenarios did not reject these helper mutations, so M1/M2 public-route auditing remains open. The shared object-layout target advances Phase 5/6/7 gates to 2,812/1,703/2,940 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-massframe-scale-merge-000837-000839.md` and `evidence/raw/phase2-massframe-scale-merge-20261016/`.

### Continuation — per-step debug-renderable clear (`phys_fn_000445`, 2026-10-16)

The 60-frame public scene-visualization differential now requires exactly three contact lines after the final fetched frame. Suppressing only `PhysicsSDK::clearDebugRenderable` in a rebuilt candidate accumulates 99 lines and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=262`); the restored candidate matches exactly. The Phase 2 row remains deferred to its Phase 7 caller until that phase closes, but its previously static-only claim is now a row-specific public-DLL falsification. Phase 5/6 floors remain 2,812/1,703; the registered Phase 7 visualization differential raises its floor to 2,941. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-debug-renderable-clear-000445.md` and `evidence/raw/phase2-clear-debug-renderable-000445-20261016/`.


### Continuation — sub-word particle stream strides (`phys_fn_003423`, 2026-10-16)

The registered stride fixture now checks the aligned 13-byte case and the sub-word 3-byte edge. The oracle rounds each stride down to zero: it repeats the first position for particle two and leaves optional velocity and lifetime channels zero. The restored candidate matches exactly. Removing position-stride rounding changes the position output and fails the candidate (`oracle_exit=0`, `candidate_exit=1`); restoring the implementation returns `stdout_delta=0`. Phase 7 advances to 2,942/2,942 assertions. This closes the sub-word stride behavior slice only; `phys_fn_003423` remains open to the full-image instruction audit. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-stride-subword-003423.md` and its raw manifest.
### Continuation — lock-API initializer differential enforcement (`phys_fn_002367`, 2026-10-15)

The `NxPhysicsObjectLayoutTests` comparison for row 002367 now emits a registered row-specific coverage marker and contributes mismatches to the test's failing result. The differential runner supplies and verifies the staged oracle DLL identity, then treats this target as a candidate-source harness instead of launching it against the candidate DLL as a second oracle. A callback-argument mutation was rejected (`newreach row=002367 mismatches=1`, candidate failures=1); the restored control reports zero mismatches. Phase 7 advances to 356 closed / 204 deferred and the fresh full gate passes at 2,885/2,885; all 816 reconstruction-tool tests pass. The object-layout oracle fixture digest and retained proof self-hashes were refreshed for the harness change. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-physfn-002367-lock-api-init.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-physfn-002367-20261015/`.

### Continuation — lock-API call and test callbacks (`phys_fn_002371`, `phys_fn_002375`, 2026-10-15)

The candidate source harness now differentially checks the callback argument loaded from `[self]` and the zero/nonzero callback return mapping. Mutating the first callback value produces one mismatch for 002371; inverting the second row's return mapping produces two mismatches for 002375. Each mutation leaves the other row exact, the runner rejects both, and rebuilt restored controls report zero mismatches. Phase 7 advances to 360 closed / 200 deferred. Fresh Phase 5, 6, and 7 gates pass with 2,773, 1,673, and 2,889 registered assertions; all 816 reconstruction-tool tests pass. Public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-call-and-test-002371-002375.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-call-and-test-002371-002375-20261015/`.



### Continuation — lock-API worker thread exit (`phys_fn_002389`, 2026-10-15)

The row writes state `2` through `[[this+4]+4]` and calls the imported `ExitThread(0)`. A child-thread fixture calls the pinned oracle row and the candidate source helper in separate threads, then compares the state buffers and thread exit codes without terminating the test process. A state-store mutation to `1` is caught by row 002389 alone (`mismatches=1`, candidate exit 1); the restored control reports zero mismatches and both thread exit codes are 0. Phase 7 advanced to 365 closed / 195 deferred in that packet; the current Phase 7 ledger is 366 closed / 194 deferred. Its fresh Phase 5/6/7 gates pass at 2,779 / 1,679 / 2,895 assertions; all 816 reconstruction-tool tests pass, inventory validation reports unexplained=0, and both public-header trees match the 80-file manifest. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-thread-exit-002389.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-thread-exit-002389-20261015/`.

### Continuation — lock-API virtual callback rows (`phys_fn_002385`, `phys_fn_002390`, 2026-10-15)

The object-layout source differential now emits an independently registered marker for each row and includes each mismatch in candidate failure accounting. Changing the callback argument forwarded by 002390 with `arg ^ 1u` is caught by row 002390 alone; changing 002385's `0xffffffffu` callback argument to `0xfffffffeu` is caught by row 002385 alone. Both mutations produce `mismatches=1`, and rebuilt restored controls report zero mismatches and pass. Phase 7 advances to 364 closed / 196 deferred. Fresh Phase 5/6/7 gates pass at 2,777 / 1,677 / 2,893 assertions; all 816 reconstruction-tool tests pass, inventory validation reports unexplained=0, and both public-header trees match the 80-file manifest. Public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-virtual-callback-002385-002390.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-virtual-callback-002385-002390-20261015/`.

### Continuation — lock-API byte setter and getter (`phys_fn_002383`, `phys_fn_002387`, 2026-10-15)

Added the source-side byte setter/getter and a compact oracle/source differential outside the large layout harness stack frame. The getter checks both zero and nonzero pointee bytes; the setter compares the full pointed record after separate oracle and candidate calls. A store-to-zero mutation is caught only by row 002383 (`mismatches=1`); a constant-zero getter mutation is caught only by row 002387 (`mismatches=1`). Restored comparisons pass. Phase 7 advances to 362 closed / 198 deferred, and the three phase coverage floors increase by two each to 2,775 / 1,675 / 2,891. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-byte-setter-getter-002383-002387.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-byte-setter-getter-002383-002387-20261015/`.

### M2 sphere radius lock wrappers (phys_fn_001183 / phys_fn_001185)

The sphere public setter now refuses a foreign-owner write lock with the oracle line-0x29 error and keeps its owner/pruner update behavior; the getter brackets internal slot 15 with the scene read lock. The registered public differential is exact. Setter try-lock removal is caught (`stdout_delta=663`), getter bracket removal is caught (`stdout_delta=2`), and Phase 5 passes at 2,761/2,761. Phase 3 advances to 87 closed / 305 deferred. The approved Phase 6 scene-dump date/time assertion is already in the plan and its fresh 1,662/1,662 gate remains green. Evidence: `docs/reconstruction/novodex-physics/evidence/phase3-sphere-public-radius-locks-001183-001185.md`.

### Continuation — sphere invalid-radius reporting (phys_fn_001357, 2026-10-15)

The public sphere radius wrapper now dispatches through internal slot 14, whose invalid-value path reports through FoundationSDK at the oracle's `SphereShape.cpp:0x4a`, then updates its owner and scene dirty state. Public zero/negative-radius cases are exact against the shipped DLL. Removing the validation report is caught (`stdout_delta=670`); restored output is exact. The object-layout source harness uses a no-op dirty helper for its detached shapes, while the production DLL links the real `Scene.cpp` helper. The official Phase 5 gate passes at 2,764/2,764 across 25 targets, and all 816 tooling tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-shape-accessors-001001-001004-001357-001363.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase5-sphere-radius-validation-001357-20261015/`.

### Continuation — Pruner process-pool growth allocation failures (2026-10-15)

The `NxPhysicsPrunerPoolAllocationFailureTests` child-process differential now crosses the initial two-entry process-pool capacity by filling one Scene's static/dynamic registrations, then adding the first registration from a second Scene. It fails each growth array allocation (16 bytes for objects; three 8-byte U16 arrays). The oracle and candidate all raise access violation `0xC0000005`, and the parent confirms exact output. A candidate-only mutation that skips the growth branch lets all four children survive and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=10`, four mismatches). Fresh Phase 5/7 gates pass at 2,746/2,746 and 2,874/2,874, and all 816 tooling tests pass. Growth OOM remains unrecoverable by design in this evidence; no function row is closed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-pruner-pool-growth-allocation-failure-20261015.md`.

### Continuation — Pruner process-pool array allocation failures (2026-10-15)

The new `NxPhysicsPrunerPoolAllocationFailureTests` target fails the four parallel array requests individually: the 8-byte registration-object array and each of the three 4-byte U16 arrays. The shipped DLL and reconstructed DLL all raise access violation `0xC0000005` in a child process, and the parent verifies the same result without crashing the phase-gate runner. A candidate-only retry mutation makes all four cases survive and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`); the restored pair is exact. Fresh Phase 5/7 gates pass at 2,745/2,745 and 2,873/2,873, and all 816 tooling tests pass. This captures the unchecked OOM contract rather than adding recovery behavior; pool capacity boundaries and the full DLL backlog remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-pruner-pool-array-allocation-failure-20261015.md`.

### Continuation — Pruner process-pool owner allocation failure (2026-10-15)

The public Scene allocator differential now injects failure into the SDK allocator's next 0x1c-byte allocation, which is the `NxPrunerProcessPool` singleton owner. Scene and static actor creation still succeed, Pruner registration leaves the Scene usable, and Scene teardown returns the SDK count to zero. Oracle/candidate allocator output matches exactly with no cross-frees. Removing `nxPrunerRegister`'s null-allocation return crashes the candidate and is rejected (`candidate_exit=-1073741819`, `stdout_delta=70`); the restored control is exact. Fresh Phase 5 and Phase 7 gates pass at 2,744/2,744 and 2,872/2,872, and all 816 tooling tests pass. This is verification only: production source and public headers are unchanged; Pruner's four parallel-array allocation failures, pool growth failures, and the full DLL backlog remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-pruner-pool-owner-allocation-failure-20261015.md`.

**Type-3 pruner continuation, 2026-10-15:** The type-3 pruner factory, 0xa0-byte derived layout, trailing-Container destruction, UpdateObject, and shared pre-query tree-refit wrappers now have a registered static-proof fixture. Omitting the bottom-up refit is rejected; the restored fixture passes at 98 checks. The fresh Phase 4 gate passes at 533/533 coverage assertions. This is deliberately not counted as terminal row closure: there is no DLL-to-DLL differential for the type-3 query family, no public scene currently selects it, and the extra Container's runtime role remains unknown. See `docs/reconstruction/novodex-physics/evidence/phase4-type3-pruner-query-005448-005462-partial.md`. The code backlog and row closure totals remain controlling.

### Continuation — OPCODE AABB-tree mesh creation rows (`phys_fn_005517`, `005523`, 2026-10-14)

The public triangle-mesh API fixture now requires `triangle_mesh created=1`. Independent no-op mutations at `AABBTreeNode::_BuildHierarchy` and `AABBTree::Build` each prevent authored PMap fixture mesh creation (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=26`). `Pruner::Pruner` (`phys_fn_005531`) is now checked through the registered Scene raycast fixture: after shape registration, the test inspects the constructed Pruner's six empty-bound words in the real DLL. An entry-return mutation leaves the guard pattern intact and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2,793`); the restored differential matches exactly. Phase 4 now records 37 closed / 1,016 deferred, and the fresh Phase 4, 5, and 7 gates pass at 531/531, 2,738/2,738, and 2,869/2,869. See `docs/reconstruction/novodex-physics/evidence/phase4-pruner-constructor-005531.md` and `docs/reconstruction/novodex-physics/evidence/phase4-aabbtree-005517-005523.md`.

### Continuation — buffered contact-report rows (`phys_fn_000917`, `000919`, `000921`, 2026-10-13)

The three oracle spans map to candidate `cpmBufferContactReports0917`. Setting its hash-loop count to zero is detected by the registered public simulation: contact callbacks fall from 26 to zero and the candidate exits 1 (`stdout_delta=669`); the independent deferred-queue control still passes. Restoring the implementation returns an exact staged-pair differential. After merge, the fresh full Phase 7 gate passes at 1,745/1,745; Phase 7 records 262 closed / 298 deferred functions and its coverage floor remains unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-report-buffer-000917-000919-000921.md`; the post-merge gate transcript is archived in the adjacent raw-evidence folder.

### Continuation — scene-release contact-report flush (`phys_fn_000913`, 2026-10-13)

Added a public release fixture that stops on the first plane/sphere start-touch callback, then destroys the Scene while the pair is still touching. The destructor delivers a final end-touch callback before actor teardown. Suppressing `cpmFireContactReports0913` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=439`); restored output is exact. Phase 7 now records 259 closed / 301 deferred rows and raises its registered coverage floor to 1,745. Rows `000917`, `000919`, and `000921` remain open for independent falsification. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-contact-report-000913.md`.

### Continuation — fluid per-fluid substep closure (`phys_fn_003495`, 2026-10-13)

Current-main audit superseded the older M7.6 brief: the dirty static-collision builder `003505`, its required helpers, the body-state caller `003624`, body-repulsion helper `003601`, and manager step `003630` already have measured Phase 7 closures. The per-fluid substep `003495` was implemented and exercised but remained mislabeled `decompiled`/deferred. Added its implementation symbol/map binding and moved it into the Phase 7 closure ledger. The registered empty, bodyless-emitter, and attached-body differentials pass; removing the pre-FluidRun emitter update is rejected (`stdout_delta=10`, `mismatches=1`). Fresh Phase 7 passes at 1,740/1,740. Phase 7 now records 244 closed / 316 deferred; full DLL closure remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-substep-003495-closure.md`.

### Continuation — actor-group pair-flag wrappers (`phys_fn_000269`, `000271`, 2026-10-13)

Moved the existing SDK round-trip fixture to run with two live Scenes, so the setter's write-lock loop and getter's read-lock loop execute. Separate no-op mutations are rejected (`stdout_delta=6` and `2`); the restored `NxPhysicsSDKTests` differential is exact. Phase 2 closes 63 rows and defers 80; its fresh gate passes at 7/7, and Phase 3 passes at 561/561. The Phase 3 rebuild also surfaced missing fluid implementation sources in `NxPhysicsCollisionTests`; adding the same two source files used by the internal target fixes its fresh link. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-actor-group-pair-flags-000269-000271.md`.

### Continuation — Scene pair-flag setters (`phys_fn_000589`, `000590`, `000592`, 2026-10-13)

The public pair-flag fixture now checks same-shape and same-actor setter errors, valid stored flags, and allocated-record clearing. The Scene path explicitly resolves the `const NxU8*` overload that implements row 000589; this corrected an overload selection that had left the oracle row unreachable. Independent no-op mutants are rejected for rows 000589/000590/000592 (`stdout_delta=23/26/15`), and the restored focused control matches exactly. Phase 7 records 225 closed / 335 deferred functions and passes at 1,727/1,727 assertions. See `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-flag-setters-000589-000590-000592.md`. Full-DLL reconstruction remains open.

### Continuation — Scene pair-flag getter reconstruction (`phys_fn_000513`, `phys_fn_000515`, 2026-10-13)

IDA confirmed that the Scene-level getters reject identical shape or actor references before reading the pair hash. The private Scene methods now reproduce the line-916 and line-948 diagnostics, resolve actor roots through their internal records, and route `NpScene::getActorPairFlags` through its read lock. The public pair-flag target covers both invalid and valid paths. Separate no-op mutations are rejected (`stdout_delta=23` and `11`); restored controls match. Phase 5, 6, and 7 pass at 2,685, 1,614, and 1,725 assertions; Phase 7 now records 222 closed / 338 deferred. Public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-flag-getters-000513-000515.md`.

### Continuation — public `NxFluid` configuration getters (`phys_fn_003768`–`phys_fn_003782`, 2026-10-13)

The solver wrapper now implements public vtable slots 37–44: max particles, kernel radius multiplier, rest particles per meter, rest density, rest particle distance, particle mass, simulation method, and collision method. IDA shows the spacing getter computes the reciprocal of `FluidGetRestParticleDistance`; the kernel getter is gated by licensed mask `0x1`, and collision method reports the oracle's line-374 warning. Product code 465 exercises the licensed path through an isolated test registry key. The public fixture verifies values, callbacks, lock-backed access, and exact warning diagnostics. Both a wrong-bit mutant and a no-forward mutant are rejected (`stdout_delta=52`); the restored run matches exactly. Phase 5 and Phase 7 pass at 2,685 and 1,718 assertions; Phase 7 closes 212 rows and defers 348. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-config-getters-003768-003782.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-npfluid-config-license-20261013/`.

### Continuation — public `NxFluid` collision scalar getters (`0x0008b990`, `0x0008ba70`, `0x0008bb50`, `0x0008bc70`, 2026-10-13)

The static-collision restitution and adhesion getters now call their corresponding FluidModel callbacks under the scene read lock. The dynamic-collision getters preserve the oracle's unavailable-feature warnings and exact lines 188 and 203. The fixture returns distinct scalars only for the live manager/fluid arguments; the initial test was red at the missing vtable slot, the restored differential matches exactly, and a no-forward static-restitution mutant is rejected (`stdout_delta=52`). Phase 5 passes at 2,683 assertions and Phase 7 at 1,716. Collision setters and the remaining fluid methods stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-collision-getters.md`.

### Continuation — public `NxFluid::getDamping` (`0x0008b8b0`, 2026-10-13)

The primary wrapper table now routes `getDamping()` through `FluidGetDamping(manager, fluid)` while holding the wrapper's scene read lock. The fixture validates both callback arguments by returning `0.375f` only for its live manager/fluid pair, and records exactly one invocation. The initial differential was red (`oracle=0x3ec00000`, candidate `0xffc00000`, calls `1/0`); restored output matches exactly. A no-forward mutant returns zero and is rejected (`candidate_exit=1`, `stdout_delta=52`). Phase 5 passes at 2,681 assertions and Phase 7 at 1,714. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-damping-getter.md`.

### Continuation — public NxFluid scalar getters (`0x0008b610`, `0x0008b760`, 2026-10-13)

The primary wrapper table now exposes stiffness and viscosity getters. The oracle checks the registry-backed FluidKey feature bit; on this host both getters return zero and issue their exact `NXE_DB_WARNING` diagnostics, which the simulation differential now verifies. A wrong stiffness warning line is rejected (`stdout_delta=2`), and restored output matches. Phase 5 passes at 2,680 assertions; Phase 7 passes at 1,713. Licensed FluidModel getter callbacks, the scalar setters and remaining public methods stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-scalar-getters.md`.

### Continuation — public NxFluid emitter count/list (`0x0008b1f0`, `0x0008b220`, 2026-10-13)

The public fluid wrapper now dispatches `getNbEmitters()` and `getEmitters()` through its primary vtable. Both methods use the patched scene read-lock link, follow the internal-fluid pointer, and expose the internal count and public-wrapper array. The registered simulation fixture verifies three retained emitters through the public API; a wrong count-array offset produces 5,936,165 and is caught (`stdout_delta=2`). Restored oracle/candidate output is exact. Phase 5 passes at 2,679 assertions and Phase 7 at 1,712. The other 45 public methods, the two earlier create/release slots, full vtable shape, and the full-DLL census remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-emitter-count-list.md`.

### Continuation — fluid descriptor-emitter creation (`phys_fn_003649`, 2026-10-13)

The enabled fluid path now processes one valid `NxFluidDesc.emitters` entry: it resolves the rest-particle-distance and emitter callbacks, creates the 0x78-byte internal emitter plus its 0x18-byte public wrapper, and appends both to their owner arrays. Oracle pointer probing corrected the public layout: the manager's primary array contains internal fluids, its secondary array contains public wrappers, and the wrapper's `+0x14` points back to the internal fluid. The test asserts that relationship and the callback sequence. The registered simulation differential now matches exactly (`both exits=0`, `stdout_delta=0`, exact stderr); the original missing-emitter-callback case was a clean red. This remains a bounded slice: additional emitters, failures, actor-relative frames, public fluid/emitter operations and lifecycle permutations are open, so `phys_fn_003649` and full reconstruction stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-create-003649.md`.

### Continuation — null-backend fluid emitter retention (`phys_fn_003649`, 2026-10-13)

The descriptor fixture now creates three emitters and forces the second `EmitterCreate` callback to return null. The oracle retains all three internal objects and wrappers, runs both global-pose callbacks for the null handle, and grows both arrays on the third append. Removing the backend-handle rejection restores an exact differential; a growth-disabled mutation leaves the candidate at two entries and is rejected. The Phase 5 coverage record requires the three entries, growth, and null-backend retention. Fresh Phase 5 and Phase 7 gates pass at 2,678/2,678 and 1,711/1,711 assertions. Larger lists, allocation failures, actor-relative frames, public operations, and lifecycle work remain open, so the function row stays open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-create-003649.md`.

### Continuation — expanded fluid descriptor validation (`phys_fn_003649`, 2026-10-13)

Extended the enabled-manager differential from one bad-density descriptor to fourteen invalid cases: scalar limits, both particle-buffer pair/stride checks, surface-mesh validation, invalid embedded emitter validation, and the 20-emitter cap. Each returns null with exactly one `NXE_INVALID_PARAMETER` at line 140. Size-targeted injection confirms manager allocation (`0x34`) leaves the scene untouched, fluid allocation (`0x84`) returns null without side effects, and emitter allocation (`0x78`) skips its distance callback before later emitters are processed. Both gates require these markers. Array-growth failures, later allocations, and broader lifecycle behavior remain open, so `phys_fn_003649` stays open. Evidence: `docs/reconstruction/novodex-physics/evidence/fluid-enabled-descriptor-validation.md`.

### Continuation — actor-relative emitter construction (`phys_fn_003649`, 2026-10-13)

The registered public simulation fixture now attaches an emitter to a dynamic actor with an exact 180-degree rotation and nonzero translation. It checks the stored internal actor-body pointer and the composed global pose read back through the public emitter wrapper. The oracle and candidate staged differential matches exactly; Phase 5 and Phase 7 require `frame-actor=1 frame-pose=1`. Public emitter mutation/release behavior and manager lifecycle combinations remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-create-003649.md`.

### Continuation — actor-pair contact stream reset (`phys_fn_000863`, 2026-10-12)

The live contact visualization fixture also exercises the per-step pair refresh that calls the actor-pair contact-stream reset thunk. Replacing its row 002354 forward with a no-op leaves stale stream contents: the oracle renders 3 contact lines, the mutant renders 78, and the registered paired differential rejects it (`both exits=0`, `stdout_delta=87`, exact stderr). The restored control matches exactly. Phase 7 remains at 1,711/1,711 assertions and now records 201 closed / 359 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-contact-stream-reset-000863.md`.

### Continuation — contact-anchor storage (`phys_fn_000865`, 2026-10-12)

The registered public `NxPhysicsSimulationTests` contact, box-contact, sphere-pair, and friction scenarios exercise the friction-patch anchor store. An immediate-return row mutation omitting count and both local-anchor writes changes the transcript by 120 bytes (both exits zero, exact stderr); the restored control matches exactly. Phase 7 remains at 1,711/1,711 assertions and now records 202 closed / 358 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-anchor-store-000865.md`.

### Continuation — spring-patch material classification (`phys_fn_000871`, 2026-10-12)

The registered public simulation contact/friction fixture rejects a row-specific mutant that classifies every patch as a spring patch (`stdout_delta=116`, both exits zero, exact stderr). The restored material-flag OR matches the oracle exactly. Phase 7 remains at 1,711/1,711 assertions and now records 204 closed / 356 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-spring-patch-classification-000871.md`.

### Continuation — contact-stream visualization (`phys_fn_000869`, 2026-10-12)

The existing public plane/sphere contact fixture now mutation-falsifies the pair renderer: a no-op `row000869` produces zero contact lines and fails the visualization assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=9`). The restored candidate renders the same three lines as the oracle and matches exactly. Phase 7 remains at 1,711/1,711 assertions and now records 203 closed / 357 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-visualization-000869.md`.

### Continuation — contact-pair visualization thunk (`phys_fn_000907`, 2026-10-12)

The registered scene-visualization fixture now creates an overlapping plane/sphere pair, simulates and fetches 60 steps, then enables only contact-point rendering. Oracle and restored candidate each emit three contact lines and match exactly. Replacing `NxPairNode::row000907` with a no-op leaves zero contact lines and is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=9`, `stderr_exact=False` on the candidate failure path). The full Phase 7 gate passes at 1,711/1,711 assertions; Phase 7 records 200 closed / 360 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-contact-pair-000907.md`.

### Continuation — Scene bounds-plane builder falsification (`phys_fn_000501`, 2026-10-12)

A five-face loop mutant in `nxSceneBuildBoundsPlanes` is rejected by the existing `NxPhysicsSceneBoundsPlanesTests` target: the oracle creates six actors, the candidate creates five, and the staged transcript differs by 7 bytes. The restored Release DLL matches exactly. This closes `phys_fn_000501`; Phase 7 now records 199 closed / 361 deferred. The full DLL census remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-bounds-planes-000501.md`.

### Continuation — `NpScene::setGravity` row falsification (2026-10-12)

The next reconstructed-but-unfalsified Phase 7 row was `phys_fn_000289` (`0xC400`). A Win32 Release archive build from `main` matches the pinned oracle through `NxPhysicsSimulationTests` (`stdout_delta=0`, exact stderr). Suppressing the `mScene->setGravity(gravity)` forward in a throwaway source copy is caught by the same registered staged-pair target (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=2`, exact stderr). Restoring the source and rebuilding returns the differential to `stdout_delta=0`, exact stderr. The public fixture reports the changed gravity value, so this closes the row itself; broader scene and simulation behavior remains open. Transcripts: `docs/reconstruction/novodex-physics/evidence/raw/phase7-npscene-setgravity-000289-20261012/`.

### Continuation — classify the inline `NxScene` destructor artifact (2026-10-12)

The oracle `NxScene` base-vtable slot at `0x10105988` points to `phys_fn_000283` (`0xC2F0`). Its deleting-destructor behavior is emitted from the immutable inline `NxScene::~NxScene()` declaration in `Physics/include/NxScene.h`; the fresh candidate map emits `??_GNxScene@@MAEPAXI@Z` and `??_7NxScene@@6B@` in `NpScene.obj`, and the candidate vtable points to the emitted destructor. Both sides restore the base vptr, return `this`, use `ret 4`, and release through CRT `_free` (directly in the oracle, through sized delete in the candidate). Reclassified the row as a Phase 8 compiler artifact, removed its Phase 7 deferral, and regenerated the census outputs. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-nxscene-inline-scalar-dtor-000283.md`.

### Continuation — effector scene wrapper falsification (2026-10-12)

The adjacent effector scene wrappers `phys_fn_000327`, `000329`, and `000331` are now mutation-falsified through `NxPhysicsEffectorTests`. A zero-count mutant, an iterator that skips the first item after reset, and a null next-effector mutant each change the registered transcript (`stdout_delta=14`); restored control is exact. Phase 7 records 165 closed / 396 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-get-nb-effectors-wrapper-000327.md`, `...reset-effector-iterator-wrapper-000329.md`, and `...get-next-effector-wrapper-000331.md`.

### Continuation — joint scene wrapper falsification (2026-10-12)

The adjacent `NpScene` rows `phys_fn_000321`, `000323`, and `000325` now have direct row-specific mutations through the registered `NxPhysicsJointStagedPairTests` fixture. Returning zero for the count getter, advancing the iterator past the first item after reset, and returning null from the next-joint getter each change the transcript (`stdout_delta=296`); the restored control is exact. Phase 7 now records 162 closed / 399 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-get-nb-joints-wrapper-000321.md`, `...reset-joint-iterator-wrapper-000323.md`, and `...get-next-joint-wrapper-000325.md`.

### Continuation — controller release entrypoint falsification (2026-10-12)

The adjacent `phys_fn_000307` forwarding row is now mutation-falsified. Its registered public fixture releases a non-head controller and then the remaining head; removing the forward leaves a stale list link and is detected (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=2`, exact stderr). Restored control matches exactly. Phase 7 records 159 closed / 402 deferred functions. The broader controller teardown and virtual interface remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-controller-000307.md`.

### Continuation — controller entrypoint falsification (2026-10-12)

The live completion backlog's next small reconstructed Phase 7 row was `phys_fn_000305`, the 8-byte `NpScene::createController` forward. Its existing public fixture exercises rejection, two valid controller creations, actor count/position checks, movement, and releases. The restored staged-pair control matches exactly; a throwaway archive mutant that returns null is rejected (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=51`). Phase 7 now records 158 closed / 403 deferred; the broader controller class remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-create-controller-000305.md`.

### Continuation — static actor fluid shape-change notification (2026-10-12)

The existing `nxFluidManagerShapeChanged` helper was correct, but `nxSceneAddActorObject` returned from its static-pruner registration path without calling it. The static path now invokes the helper after registration. A public enabled-manager fixture checks static actor add/remove dirtying and clean dynamic add/remove; restored oracle/candidate output is exact. A throwaway archive mutation that suppresses the dirty-byte store is detected (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=52`). The registered Phase 5, 6, and 7 gates pass at 2,673/2,673, 1,604/1,604, and 1,703/1,703. Phase 7 closes at 157/404. The approved five-family `EmbeddedHookBase` secondary-destructor model is already present and is re-verified in Phase 5: five family controls pass and the member-only mutant yields five mismatches. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-shape-change-003628.md`.

### Continuation — first enabled fluid static-collision build (2026-10-12)

Removed the test's earlier +0x29 dirty-flag bypass. On first valid fluid creation, the oracle calls `FluidGetCellSize`, fetches the static-collision object, initializes/builds it through `phys_fn_003505`, and clears both manager dirty bytes. The candidate now resolves those callbacks and invokes the reconstructed builder with the Scene+`0x52c` scalar and callback cell size. The fixture-backed public differential compares callback counts, stored cell size, flag state, and full output; it passes exactly. A mutation omitting the builder is rejected (`candidate_exit=1`, `stdout_delta=52`). The Phase 7 gate also showed that the private `NxPhysicsInternalTests` executable needed to link `FluidCollision.cpp`; CMake now includes it. Phase 5, 6, and 7 pass at 2,670/2,670, 1,602/1,602, and 1,697/1,697. The static-actor shape-change dirty-byte path is now closed as `phys_fn_003628`; the combined manager dirty-refresh row `phys_fn_003630` has its own differential proof. Broader fluid callback coverage remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md`.

### Continuation — fluid scene-shape collector (2026-10-12)

Reconstructed and mutation-falsified `phys_fn_003540` (`0x87580`) as `nxFluidCollisionPackShapeData`. The direct pinned-oracle probe compares both selected geometry records, all 20 output words, the shape handle, shape flag, and x86 stack state. Mutating one packed rotation word is caught in both cases (`mismatches=2`); restored output is exact. Phase 5/7 gates now require this differential marker and pass at 2,661/2,661 and 1,688/1,688; the Phase 7 ledger moves to 123 closed / 438 deferred. The fixture exercises the selected-record and flag paths; AABB refresh and invalid-index fault branches remain static-proof only. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-shape-packer-003540.md`.

Reconstructed and mutation-falsified `phys_fn_003555` (`0x87b80`) as `nxFluidCollisionCollectSceneShapes`. The direct pinned-oracle differential covers an empty scene and a populated eight-entry collection with a six-member compound and an empty compound; all twelve leaves route through the existing typed-array router. Restricting compound traversal to its first child is caught with `mismatches=5`; restored control matches counts, defined record prefixes, geometry callback payloads, output tails, and stack balance. Phase 5/7 gates pass at 2,660/2,660 and 1,687/1,687, inventory validation passes with 122/439 Phase 7 functions closed/deferred, and all 80 public headers remain unchanged. The enclosing dirty builder `phys_fn_003505` and production FluidModel callback integration remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-scene-shape-collector-003555.md`.

### Continuation — static-collision rebuild reverse-engineering (2026-10-11)

IDA's 473-byte listing for `phys_fn_003505` now records its scene enumeration, grid parameter formulas, mesh hash/prefix/build/copy lifecycle, dirty-manager caller, and late-bound `FluidModel.dll` callback dependencies. Inventory metadata no longer carries unrelated 003514 evidence for this row. This is static analysis only; `003505` and its shape-collection prerequisites `003511`, `003540`-`003549` remain open. The first direct test must bind explicit oracle fixture callbacks and include a nonempty mesh case before row closure. See `docs/reconstruction/novodex-physics/evidence/phase7-fluid-static-collision-rebuild-003505.md`.


### Continuation — fluid static-collision triangle-index caller (2026-10-11)

Reconstructed and mutation-falsified the contiguous caller rows `phys_fn_003537/003538` (`0x872a0`, 17-byte entry plus 704-byte body) as `nxFluidCollisionBuildTriangleIndices`. Its direct oracle differential covers a two-triangle asymmetric affine mesh, empty input with seeded output state, and null output; bucket rows, complete records, mesh/triangle associations, cell outputs, returns, and x86 stack match. A `meshIndex + 1` mutation is detected (`mismatches=1`), and the restored control is exact. Phase 5 and Phase 7 pass at 2,651/2,651 and 1,678/1,678; all 810 tooling tests pass. The fixture supplies independent references for callbacks absent with `FluidModel.dll`; production callback integration and dirty builder `phys_fn_003505` remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-triangle-index-builder-003537-003538.md`.


### Continuation — fluid cell-to-triangle association helper (2026-10-11)

Reconstructed and mutation-falsified `phys_fn_003535` (`0x871d0`) as `nxFluidCollisionAppendCellIndex`. The direct pinned-oracle test covers a match, two matches in distinct buckets plus a miss, and empty input; it compares bucket rows, full cell records, association storage, returns, and x86 stack. A `triangleIndex + 1` mutation is detected in both nonempty cases (`mismatches=2`); restored control is exact. Inventory validation reports zero unexplained rows; Phase 7 now records 111 closed / 450 deferred. The After merge, fresh Phase 5/7 gates pass at 2,650/2,650 and 1,677/1,677; all 810 tooling tests pass and the 80-header manifest is clean. The oracle FindCDCell callback uses an independent fixture reference because `FluidModel.dll` is absent. Caller rows `phys_fn_003537/003538` and top-level builder `phys_fn_003505` remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-cell-index-003535.md`.

### Continuation — fluid static-collision indexed-grid wrapper (2026-10-11)

Reconstructed `phys_fn_003532`/`003533` (`0x86f00`, contiguous 17-byte entry and 700-byte body ranges) as one `nxFluidCollisionBuildIndexedGrid` routine. The direct differential covers a two-triangle asymmetric affine mesh, empty input, and null output; cell storage, bucket records, returns, and x86 stack match. A skipped-loop mutant is caught in the nonempty case (`mismatches=1`) and the restored control is exact. Inventory and Phase 7 closure now record 110 closed / 451 deferred rows; the Phase 5/7 assertion floors rise to 2,649/1,676. After merge, fresh Phase 5/7 gates pass at 2,649/2,649 and 1,676/1,676; all 810 reconstruction tooling tests pass. The oracle directory omits `FluidModel.dll`, so SAT, growth, and `FindCDCell` callbacks use independent fixture references; production callback integration and Unreal static-collision testing remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-indexed-grid-003532-003533.md`.

### Continuation — fluid static-collision cell hash and deduplication (2026-10-11)

Reconstructed and mutation-falsified `phys_fn_003530` (`0x86e00`) as `nxFluidCollisionAccumulateCellCounts`. The direct oracle differential covers seven signed-coordinate inputs, duplicate and cross-bucket cells, a seeded existing cell, and empty input. All bucket records, full cell storage, expected sample counts, returns, and stack balance match. A no-op mutation is caught in both nonempty cases (`mismatches=2`); restored control is exact. The oracle's indirect `FindCDCell` slot is bound to an independent lookup fixture because `FluidModel.dll` is absent. Production extension integration and Unreal validation remain open. Fresh Phase 5/7 gates pass at 2,648/2,648 and 1,675/1,675; all 810 reconstruction tooling tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-cell-hash-003530.md`.

### Continuation — fluid static-collision indexed triangle builder (2026-10-11)

Reconstructed and mutation-falsified `phys_fn_003528` (`0x86b40`) as `nxFluidCollisionBuildTriangleCells`. Direct oracle cases cover an identity-transformed mesh, an asymmetric affine transform, and an empty mesh. Final triangle cells, all bucket records, untouched output tails, and x86 stack balance match; a no-output mutation is caught in both nonempty cases (`mismatches=2`). The restored control is exact. The oracle's triangle SAT and growth callbacks are bound to independent fixture references because `FluidModel.dll` is absent from the supplied UE3 binary directory; production extension integration and Unreal-level validation remain open. Fresh Phase 5/7 gates pass at 2,647/2,647 and 1,674/1,674; all 810 reconstruction tooling tests pass; inventory validation reports zero unexplained rows. Dirty builder `phys_fn_003505` remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-triangle-builder-003528.md`.

### Continuation — fluid static-collision triangle-cell helper (2026-10-11)

Reconstructed and mutation-falsified `phys_fn_003524` (`0x866d0`) as `nxFluidCollisionTriangleCells`. Three direct oracle cases cover positive coordinates, negative coordinates with margin, and zero-capacity array growth. They compare the 3/10/1 packed cell outputs, capacity, return value, canaries, and x86 stack balance. Suppressing candidate cell writes is caught in all three cases (`mismatches=3`); the restored control is exact. Fresh Phase 5 and Phase 7 gates pass at 2,646/2,646 and 1,673/1,673; all 810 reconstruction tooling tests pass. The oracle fixture binds independent SAT and array-growth callbacks because `FluidModel.dll` is absent from the supplied UE3 binary directory; production callback integration and Unreal static-collision validation remain open. The builder `phys_fn_003505` and caller `phys_fn_003528` remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-triangle-cells-003524.md`.

### Continuation — Scene fluid post-step traversal rows (2026-10-11)

Reconstructed and mutation-falsified `phys_fn_000619` and `phys_fn_003475`. The direct oracle differential checks the ordered bounds, optional implicit-mesh, particle, and emitter callbacks across two fluids; it also checks mesh-cache streams, per-channel particle strides/capacity, and emitter poses. Scene-manager no-op and per-fluid no-op mutations each produce 72 mismatches; a mesh-copy-only mutation produces four. Fresh Phase 5 and Phase 7 gates pass at their corrected exact registration floors, 2,633/2,633 and 1,660/1,660. The full 810-test reconstruction-tool suite passes. This closes the two rows only; enabled FluidManager substep processing and full-DLL closure remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-post-step-000619-003475.md`.
### Continuation — fluid-emitter world-pose helper row closure (2026-10-11)

Reconstructed `phys_fn_003605` at RVA `0x88310`. The helper recomposes the local emitter pose against no actor, an asymmetric matrix-backed actor, or a dynamic-body actor with an asymmetric quaternion, then forwards all twelve world-pose words through `EmitterSetGlobalPose`. Non-binary inputs exposed one-ULP differences; the explicit x87 operation order and oracle spill points now match bit for bit. The direct oracle differential compares callback payloads, callback tuple, return value, and stack balance in all three cases. Adding `1.0f` to actor-backed world-pose X translation is rejected by both actor cases (`mismatches=2`); the restored focused test reports zero mismatches. The fresh Phase 7 gate passes at 1,653/1,653. This closes the helper only; the extension-backed post-simulation path remains open.

### Continuation — fluid-emitter body-state caller row closure (2026-10-11)

Reconstructed `phys_fn_003624` at RVA `0x89930`. After refreshing world pose, the caller returns early without a frame actor; otherwise it reads the attached body's position, linear velocity, and angular velocity and publishes those vectors through three ordered FluidModel callbacks. The direct oracle differential covers both paths with asymmetric non-binary state values and compares full callback records, ordering, return value, and x86 stack balance. A `+0x154` position-offset mutation is rejected (`mismatches=1`); the restored control is exact. Fresh Phase 7 passes at 1,654/1,654 and Phase 5 passes at 2,627/2,627; all 810 reconstruction tooling tests pass. The Viewer selection passes 48/48 CTests across all 39 available scenes, with five existing pinned-oracle asset skips. The extension-backed post-simulation lifecycle remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-body-state-003624.md`.

### Continuation — fluid-emitter actor binding row closure (2026-10-11)

Reconstructed `phys_fn_003622`'s attach/update path and routed the reconstructed operation through the private `NpFluidEmitter::setFrameActor` implementation without changing public headers. The direct pinned-oracle differential now covers the previously tested detach path plus a matrix-backed actor frame and a dynamic-body quaternion frame. All local-pose words, backend world-pose callback payloads, attached-state values, callback order, and return values match. Forcing the attach callback argument to zero is rejected by both new cases (`mismatches=2`); the restored focused control is exact. The full Phase 7 gate passes at 1,652/1,649 registered assertions. Phase 7 now records 90 closed / 471 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-actor-release.md`.



**Phase 7 fluid collision bucket-hash leaf, 2026-10-11:** Reconstructed `phys_fn_003526` (`0x86ad0`) as the signed-16-bit XYZ hash/count stage used by the static-collision cell builder. Seven positive/negative coordinate triples and an empty range match the pinned oracle, including unchanged non-count row words and x86 stack balance. Incrementing each bucket count by two is rejected (`mismatches=1`). Fresh Phase 5/7 gates pass at 2,645/2,645 and 1,672/1,672; all 825 reconstruction tooling tests pass. This closes only the helper; `phys_fn_003528`, dirty builder `003505`, and full-manager path `003630` remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-hash-count-003526.md`.

**Phase 7 active-root collection, 2026-10-10:** Closed `phys_fn_000608`, represented by the active-root pass in `NxSceneInternal::simulateFrame`. A mutation that omitted all awake self-parented roots was rejected by the registered staged-pair simulation differential (`stdout_delta=6726`, candidate exit 1 at the joint-break notification assertion); the restored control is exact. Phase 7 now records 52 closed / 509 remaining rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-active-root-collection-000608.md`. Full-DLL reconstruction remains open.

**Phase 7 step-body storage, 2026-10-10:** Closed `phys_fn_000600`, the Scene's count-prefixed 0x60-byte per-step body buffer. Omitting allocation and capacity growth crashes the candidate on the registered simulation corpus (`candidate_exit=-1073741819`, `stdout_delta=3731`) while the oracle exits 0; the restored staged-pair differential is exact. Phase 7 records 53 closed / 508 remaining rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-step-body-array-000600.md`. Full-DLL reconstruction remains open.

**Phase 7 substep island reset, 2026-10-10:** Closed `phys_fn_000635`, which rebuilds dirty joint island links, resets the Scene joint-list cursor, and clears each body's island links between pair refresh and active-root collection. Omitting the row's passes is caught by the registered simulation differential (`candidate_exit=-1073741819`, `stdout_delta=3728`); restoring the passes returns to exact output. Phase 7 records 54 closed / 507 remaining rows. Its downstream `phys_fn_004172` was closed later by an independent mutation-falsified simulation differential. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-joint-island-reset-000635.md` and `docs/reconstruction/novodex-physics/evidence/phase6-island-rebuild-004172.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene scalar deleting destructor, 2026-10-10:** Closed `phys_fn_000663` with a whole-row no-op mutation against the registered populated-scene teardown differential. The mutant retained all 51 tracked allocations instead of releasing the oracle’s 39; oracle exit 0, candidate exit 1, and `stdout_delta=18`. Restoring and rebuilding returned an exact differential. Phase 7 now records 55 closed / 506 remaining rows. The full Phase 7 gate passes at 1,491 assertions against its 1,457 floor. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-destructor-000663.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene joint-count getter, 2026-10-10:** Closed `phys_fn_000559` (`NxSceneInternal::getNbJoints`) with the existing Phase 7 joint staged-pair differential. Reading the adjacent effector count at `+0x6c4` instead of the joint count at `+0x6c8` reports `count=0` while `enumerated=1`; the mutation is rejected with `stdout_delta=296`, both exits zero, and exact stderr. Restoring the correct field returns an exact differential. Phase 7 now records 56 closed / 505 remaining rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-joint-count-000559.md`. Full-DLL reconstruction remains open.

**Phase 7 retained-body-record cleanup, 2026-10-10:** Reconstructed `phys_fn_000602` as the post-joint-list pass over `Scene+[0x56c,0x570)`, calling `DynamicBody::destruct` and freeing each remaining record. The populated-scene differential keeps a 608-byte record after ordinary actor/root teardown; disabling the helper reports `freed=0` and is caught (`stdout_delta=2`). The restored candidate reports `freed=1` with exact oracle output. Phase 7 now records 51 closed / 510 deferred rows. Fresh Phase 3/5/6/7 gates pass at 551/2,621/1,267/1,457 assertions, and all 810 tooling tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-body-record-destroy-000602.md`. Full-DLL reconstruction remains open.

**Phase 7 pruning-engine continuation, 2026-10-10:** Closed `phys_fn_001982` using the populated-scene teardown differential. The fixture now pins the contact-pair hash’s first allocation to oracle sizes 8/8/16; a capacity-eight mutant is rejected as 32/32/64. Disabling pair-storage teardown leaves 18 instead of 15 tracked allocations and is caught (`stdout_delta=2`). Phase 3 passes at 545 assertions and Phase 7 at 1,452. Reconstructed `phys_fn_001963` owner cleanup is recorded as reconstructed but remains open until a nonempty-pruner mutation fixture directly exercises it. Public headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-pruning-engine-storage-001982.md`.

**M1 follow-up, 2026-10-09:** `NxPhysicsShapeVtableTests` now pins all 84 BOX-sweep input tuples (`46df40a9`) in addition to the oracle output digest (`2c5d5c09`); the retained proof is `docs/reconstruction/novodex-physics/evidence/oracle-only-proofs/phase5-shape-vtable-boxsweep.json`, and the three joint proofs were regenerated against the updated manifest. A fresh Phase 5 run exposed a harness defect: `NxPhysicsBodyCreationTests` selected the oracle RVA when `oracle` appeared anywhere in the candidate path. Exact leaf-name selection and a negative-control fixture now pass Phase 3 (544/544) and Phase 5 (2621/2621); Phase 6 (1265/1265) and Phase 7 (1447/1447) also pass. The full tooling suite passes 810 tests. Evidence: `docs/reconstruction/novodex-physics/evidence/m1-shape-boxsweep-proof-and-pair-path-2026-10-09.md`. This closes one harness defect and adds one retained proof; M1 and full reconstruction remain open.

**M1.5 Phase 4 release-guard follow-up, 2026-10-09:** `phys_fn_002258` (`TriangleMesh::release`) is now `dynamically_gated` through the registered `NxPhysicsTriangleMeshApiTests` live-reference refusal line. Bypassing its `mReferenceCount` guard in a throwaway build produces `stdout_delta=6`; restoring it returns the focused pair to an exact match. Phase 4 records 31 closed / 1,022 deferred function rows, and the fresh gate passes at 269/268 required coverage assertions. This closes the refusal branch only; the successful-release cleanup rows remain open. See `docs/reconstruction/novodex-physics/evidence/phase4-row-002258-release-guard-mutation-2026-10-09.md`.

**M2 actor-force follow-up, 2026-09-24:** Eight public force/torque variants now match the pinned DLL on 39 fresh-process assertions, including five modes, local and at-position transforms, rotated mass-local offsets, wake state, and kinematic no-ops. A linear-force mutation produced 30 mismatching lines and restoring it returned the staged differential to zero. The Phase 5 floor is 552 and still fails only at the explicit final-vtable marker; phases 2, 3, 4, 6, and 7 pass. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-force.md`. Full reconstruction remains open.

**M2 center-of-mass getter follow-up, 2026-09-24:** Six public actor getters now match 28 pinned lines over identity, translated/rotated, mass-frame, and static cases. A targeted wrong-offset mutation caused four differential mismatches. The Phase 5 floor rises to 580 and remains red at the final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-cmass-getters.md`. Setters and full-DLL closure remain open.

**M2 mass-local setter follow-up, 2026-09-24:** Three local-offset setters now update the record's local and world mass frames, revision, quaternion, dirty bits, and low-wake state to match 30 additional oracle lines. A targeted quaternion mutation produced four mismatches; the restored differential passes. The Phase 5 floor is 610 and still fails at the final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-cmass-local-setters.md`. Global-offset setters and full-DLL closure remain open.

**M2 world-space mass-offset follow-up, 2026-09-24:** Three global-offset setters now inverse-transform requested world center-of-mass pose into the actor's mass-local frame while keeping the actor pose fixed. Twenty-five new oracle lines include pose/quaternion record aliases and one-bit floating-point round trips. A Y-axis transform mutation produced 26 mismatches. The Phase 5 floor is 635 and remains red at the final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-cmass-global-offset-setters.md`. Global mass-pose movement and whole-DLL closure remain open.

**Execution note, updated 2026-10-08:** The three known OPCODE artifact misclassifications have been corrected in the inventory. `Segment::SquareDistance` passes both registered oracle families exactly (100,000 compared words) and now has a row-specific mutation caught by the direct-linked `NxPhysicsThirdPartyTests` harness; the rebuilt DLL map retains that symbol and `qh_pointdist` through explicit link options. Public-DLL routing remains unproven, so the current DLL is still a partial reconstruction. The oracle's `SweepAndPrune::Init` calls NovodeX helper `phys_fn_004816` at `0x000b4530`; the helper and continuation are reconstructed and mutation-closed, but archive linkage and DLL-level routing remain open. See `docs/reconstruction/novodex-physics/evidence/phase4-falsification.md` §§10–13. Continue with actual-DLL driving and the remaining M0/M1 work before advancing full-image closure.

**Execution status, updated 2026-10-08:** The completion backlog has been regenerated from current inventory and closure ledgers. `phys_fn_000001` (MSVC vector constructor iterator) now has an explicit source/map mapping and row-specific mutation proof; Phase 4 closed it on `NxPhysicsThirdPartyTests` (18 constructor calls in the baseline, 30 hull-leaf mismatches under wrong stride, zero after restore). Phase 2 now records 61 closed / 82 deferred. Full-DLL reconstruction and all later-phase work remain open. `phys_fn_002411` (dynamic-body auxiliary-record unregister) now checks the record table through public `NxScene::releaseActor`; omitting its clear is caught with `stdout_delta=2`, and the restored candidate is exact. Phase 5 now records 205 closed / 0 deferred function rows; its latest full gate passes all 2,595 registered coverage assertions, while full-DLL and final census closure remain open. The range-iteration helper phys_fn_000002 now has an explicit source and candidate-map mapping; its static mutation proof remains distinct from oracle runtime evidence. The public actor position/pose getter mutation proof for phys_fn_000092 is in evidence/phase5-global-position-000092.md.

**Viewer sweep note, 2026-10-08:** The approved all-scenes Viewer smoke design was run with `ctest --test-dir build -C Release -R '^Viewer' --output-on-failure`. All 48 selected tests passed, including the 39 available scene entrypoints. Five scenes reported the existing asset-based skips (`CowPile`, `PMapTest10`, `PMapTest12`, `PMapTest8`, and `TruckDemo`); their signed skip diagnostics remain visible in CTest. This validates viewer startup/render/shutdown coverage only; the standalone active-physics and Unreal milestones remain open.

**Capsule/mesh public-path note, 2026-10-08:** The Phase 5 gate now passes after a focused public DLL capsule/mesh regression. Contact and overlap dispatch slots are wired, and triangle plane records are built after mesh topology. An isolated two-patch simulation requires callbacks on both patches; removing the contact dispatch entry makes the candidate fail (`candidate_exit=1`, `stdout_delta=2`), while restoring it returns to an exact staged-pair differential. The Phase 5 run evaluated all 2,563 coverage assertions and passed. This closes the exercised contact route only: capsule/mesh trigger overlap and `Segment::SquareDistance` DLL routing remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/capsule-mesh-public-dispatch.md`.

**Integration gates and DemoGame smoke, 2026-10-08:** Phase 6 passes with 1,067 assertions and Phase 7 passes with 1,388 assertions. A staged oracle/candidate `PhysTest` run also exits 0 on both sides and confirms each process loaded Physics and Foundation from its own pair. Both load and unload the map and emit the same four center-of-mass diagnostics. This is startup/map coverage; no physics callback or numeric body state was captured, so active Unreal simulation remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/demogame-phystest-smoke-2026-10-08.md`.

**M5 joint lifecycle follow-up, 2026-10-09:** Closed `phys_fn_004095` (`Joint::~Joint`) with the existing public core-dump release fixture. Omitting the destructor's limit-plane purge changes the post-release allocator count from 14 to 17; the registered `NxPhysicsCoreDumpTests` staged-pair differential rejects the mutation (`stdout_delta=2`) and the restored candidate is exact. The Phase 6 full gate now includes this closure and remains a partial gate; joint/effector reconstruction, Phase 7 simulation, and the full-DLL audit remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004095-destructor.md`.

**M5 D6 axis setter follow-up, 2026-10-09:** Expanded the public D6 joint case to call `setGlobalAxis` after construction and assert the world-space readback. This is distinct from the descriptor's global-axis helper and reaches `phys_fn_004439`; forcing the wrapper to forward `(0, 1, 0)` changes the readback and is caught by the registered staged-pair test. The existing approved three-step off-center spring/damper fixture remains part of Phase 6/7 gate coverage. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004439-d6-set-global-axis.md`.

**M5 D6 getter follow-up, 2026-10-09:** Closed `phys_fn_004437` (`getGlobalAnchor`) and `phys_fn_004441` (`getGlobalAxis`) against the public D6 case in `NxPhysicsJointStagedPairTests`. A D6-only +1.0 X mutation of each getter is caught with `stdout_delta=24`; both restored controls are exact. The full joint and simulation contracts remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004437-004441-d6-getters.md`.

**Current consumer smoke, 2026-10-09:** After fresh standalone Phase 6/7 passes and the all-scenes Viewer sweep, oracle and candidate DemoGame staging roots both ran `PhysTest` to clean exit, loading Physics and Foundation only from their own roots and initializing the D3D adapter. The installed oracle pair remained unchanged. This is Unreal map/startup integration evidence; the run did not assert per-step body state. Evidence: `docs/reconstruction/novodex-physics/evidence/demogame-phystest-smoke-2026-10-09.md`.

**M5 spring/damper adjustor follow-up, 2026-10-09:** Added a scratch-copy probe to the registered `NxPhysicsEffectorTests` target for `phys_fn_003954`, the `EmbeddedHookBase` secondary-vtable adjustor. Removing its `-8` receiver adjustment changes the registered read-link observation (`read_link_same=yes` to `no`); the restored staged-pair control is exact. Phase 6 now records 76 mutation-falsified rows and 357 deferred rows. The adjacent deleting-destructor and abstract destructor rows remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-effector-003954-adjustor-thunk.md`.

**M5 spring/damper deleting-destructor follow-up, 2026-10-09:** Closed `phys_fn_003956` using the existing `NxPhysicsEffectorTests` release transcript. A no-op class deallocator removes the wrapper's 0x18-byte frees (`stdout_delta=8`); restoring the allocator call returns the staged-pair differential to exact. Phase 6 now records 77 mutation-falsified rows and 356 deferred rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-effector-003956-deleting-destructor.md`.

**M5 fixed-joint descriptor follow-up, 2026-10-09:** Closed `phys_fn_004242` on the existing public fixed-joint save/readback case. Removing the base descriptor save changes the saved anchors and produces `stdout_delta=92`; the restored differential is exact. Phase 6 now records 78 mutation-falsified rows and 355 deferred rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-fixed-joint-004242-save-to-desc.md`.

**Gate note, 2026-09-24:** Both PowerShell runners now accept explicit repository, build, oracle, and pair-output roots. In the isolated worktree, the Phase 4 gate passes with 100 of 100 coverage assertions and the Phase 2 staged-pair gate passes. A Phase 4-only process previously printed `status=pass` but returned the differential runner's intentional skip code 3; the gate now returns 0 on successful completion.

**Pruning/AABB note, 2026-09-24:** Recovered the oracle's `0x000b4530` NovodeX complete-box-pruning helper and its `0x000b46b0` continuation, then switched `NxOpcode` and its consumers to the oracle's min/max AABB representation. A fresh whole-archive Win32 DLL link now succeeds. The new direct-link helper differential is exact on 730 oracle words and separately falsifies the sorting and pair-output portions; Phase 2, 3 and 4 gates pass, with Phase 4 at 101 of 101 coverage assertions. These are intermediate closures: actual-DLL public routing, lifecycle behavior, and the remaining census remain open. The earlier whole-archive failure recorded above is historical and has been resolved. See `docs/reconstruction/novodex-physics/evidence/phase4-falsification.md` §11.

**Historical Phase 5 reproduction, 2026-09-24 (superseded):** On that whole-archive Win32 build, the gate exited 1 on a synthetic missing-vtable marker. Later object-model work removed that marker and Phase 5 became green. A fresh standalone Phase 5 run on 2026-10-09 now passes all 2,597 registered coverage assertions; see the current status above and the gate evidence. This historical failure is not the current blocker.

**M2 shape mutation follow-up, 2026-09-24:** Public actor `createShape` now promotes a one-box actor to a two-child group with the oracle's five allocation sizes and stable original handle. `releaseShape` removes the added child from both group arrays without an immediate free; actor teardown matches the measured allocation/free transcript. Nine registered oracle lines raise the Phase 5 floor to 829, with zero staged stdout delta. Appending to an existing group, single-shape removal, and final-vtable closure remain open. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-shape-mutation.md`.

**Historical gate matrix, 2026-09-24 (superseded):** Phase 5 was then red on the synthetic vtable marker above. Later work removed the marker. Fresh current runs for `completed` and standalone Phase 5 passed on 2026-10-09, while Phase 6/7 pass at their latest recorded partial-gate counts. These remain intermediate phase gates with substantial deferred census rows, not a claim that the DLL is complete.

**M2 actor packet, 2026-09-24:** `NxPhysicsActorLifecycleTests` now drives static and dynamic box actors through both staged DLL pairs. The candidate matches the oracle's `isDynamic()` and position words; a static actor owns the oracle's 0x50-byte outer body. Deliberate mutations of each virtual result failed the staged differential. See `docs/reconstruction/novodex-physics/evidence/phase5-actor-lifecycle.md`.

**M2 dynamic graph follow-up, 2026-09-24:** The candidate now matches the oracle's outer 0x50 and dynamic record 0x260 sizes and their backlink through the staged DLL test. A later ownership probe established that record+0x19c points back to that same outer body; it is not a third pose allocation. The exported joint-descriptor row disassembly confirms the graph, and its staged pair remains exact after updating the traversal. Record state remains open; the Phase 5 vtable marker remains red.

**M2 rotation follow-up, 2026-09-24:** Static identity and dynamic half/quarter-turn cases now match oracle quaternion storage and public orientation getters byte for byte through staged DLL pairs. The quarter-turn exposed x87 intermediate precision, and separate matrix/quaternion getter mutations failed the differential. See `docs/reconstruction/novodex-physics/evidence/phase5-actor-rotation.md`. Other rotation contexts and actor mutations remain open.

**M2 pose follow-up, 2026-09-24:** Slot 5's full `NxMat34` return matches all twelve oracle words for four actor cases, and an aimed pose-translation mutation failed the staged differential. The Phase 5 coverage floor is 152; lock behavior and other pose contexts remain open.

**M2 ownership follow-up, 2026-09-24:** The public actor wrapper is 0x18 bytes, and the 0x50-byte outer body is also the pose object. The candidate now matches the guarded oracle's fourth-actor allocation order, shared scene lock aliases, dynamic record array growth, and five-block release sequence. Actor count changes from four to three and the released actor disappears from the public list. See `docs/reconstruction/novodex-physics/evidence/phase5-actor-ownership.md`. The tested box lifecycle is not the full shape/body teardown matrix; the Phase 5 floor is now 176.

**M2 static release follow-up, 2026-09-24:** A static box release now matches the oracle's four-block free sequence and public count/list transition. The Phase 5 floor is 180. Multi-shape and other teardown paths remain open.

**M2 two-shape lifecycle, 2026-09-24:** The dynamic two-box actor is now a registered staged-pair case. Its 0x110 group, two child arrays, broadphase `2/4 → 5/8 → 2/8` transition, recycled IDs, creation allocation/free order, and eleven-block release order match the oracle. Twenty-nine new assertions raise the Phase 5 floor to 209. See `docs/reconstruction/novodex-physics/evidence/phase5-multi-shape-group.md`; other shape geometries, final vtables, and the first dynamic actor's wider Scene initialization remain open.

**M2 owner-link follow-up, 2026-09-24:** The oracle group+4 points to its outer body, body+4 points to the internal Scene, and Scene+0x48 points to a 0xa8 auxiliary manager. The candidate now matches those links after fixing two dword-index/byte-offset stores in the Scene constructor. The auxiliary manager's registration/growth behavior remains open; the ordinary Phase 5 actor differential and Phase 6 joint differential remain exact.

**M2 dynamic initialization gap, 2026-09-24:** The separate opt-in `NX_PHYSICS_PROBE_DYNAMIC_INIT=1` drive counts 17 oracle allocations and nine candidate allocations on the first dynamic box actor. The registered two-box lifecycle is exact, but its preceding Scene setup is not closed. Trace the missing initial structures before claiming the actor factory complete.

**M2 dynamic Scene auxiliary initialization, 2026-09-24:** The first dynamic box actor now initializes the Scene's five 256-slot auxiliary arrays with the oracle's 17 allocation and three scratch-buffer free sizes in exact order. Registered staged-pair checks also match array counts and sampled indices through subsequent dynamic actors and a release. Fourteen new assertions raise the Phase 5 floor to 223; the gate remains red only on the explicit final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-dynamic-auxiliary-manager.md`. The earlier nine-allocation observation above is historical and resolved for this path; untested capacity growth, non-last release, and full teardown remain open.

**M2 non-last actor release, 2026-09-24:** Releasing the first of two dynamic actors now preserves auxiliary physical slots while compacting the active-index list, recycles the body-held actor ID through Scene+0x6d4, and matches the oracle's 0x18 allocation plus six-block free order. The ordinary staged-pair transcript is exact. Eighteen new assertions raise the Phase 5 floor to 241. See `docs/reconstruction/novodex-physics/evidence/phase5-nonlast-actor-release.md`. The previous note's non-last release gap is resolved for this tested case; subsequent vacant-slot reuse and other actor classes remain open.

**M2 vacant-slot reuse, 2026-09-24:** The following dynamic actor takes auxiliary slot 0, appends that slot after surviving slot 1 in the active list, and reuses actor/shape ID 1. Its five allocations and broadphase transition match the staged oracle. Eleven more registered checks raise the Phase 5 floor to 252. The preceding note's reuse gap is resolved for this measured path; different shape classes, capacity growth, and Scene destruction remain open.

**M2 public box handles, 2026-09-24:** `getNbShapes()`/`getShapes()` now expose separate 0x1c-byte public handles for single and two-box actors. Four box-handle virtuals (`getActor`, `getType`, `is`, `getDimensions`) match the staged oracle and each has an aimed failing mutation; negative type-casting and reference aliasing are also checked. Thirty-seven new assertions raise the Phase 5 floor to 289. See `docs/reconstruction/novodex-physics/evidence/phase5-public-box-shape-handles.md`. The other 31 box slots and all other shape finals remain open; the Phase 5 final-vtable marker is intentionally still red.

**M2 static-shape auxiliary manager, 2026-09-24:** The first static box now creates the five Scene auxiliary arrays and registers all internal shape objects, including groups. The 11-allocation prefix, table counts, sampled contents, release compaction, and recycled-slot reuse match staged oracle/candidate drives. Thirty-seven registered checks raise the Phase 5 floor to 326. See `docs/reconstruction/novodex-physics/evidence/phase5-static-shape-auxiliary-manager.md`. Eleven further first-static-actor allocations and the final-vtable marker remain open.

**M2 first static OPCODE pruner, 2026-09-24:** The remaining eleven first-static-actor allocations now match, completing that path's 24-allocation sequence and three scratch frees. The Scene/shape pruner links, initial capacity/count, and static release state also match. Six more registered checks raise the Phase 5 floor to 332. See `docs/reconstruction/novodex-physics/evidence/phase5-static-pruner-initialization.md`. The previous note's allocation gap is resolved for this exact drive; dynamic-first, pruner growth, spatial payloads, internal virtuals, and teardown remain open.

**M2 dynamic-first OPCODE pruner, 2026-09-24:** A separate fresh-process target now matches the oracle's 34-allocation, six-scratch-free first dynamic actor path, including the Scene cache arrays before the `0x3c` dynamic pruner, the shared OPCODE pool, the pending buffer, and the shape/pruner link. Five registered checks raise the Phase 5 floor to 337. See `docs/reconstruction/novodex-physics/evidence/phase5-dynamic-first-pruner.md`. The previous note's dynamic-first initialization gap is resolved for one box; spatial payloads, growth, multi-shape removal, internal virtuals, and teardown remain open.

**M2 box group/material packet, 2026-09-24:** The public box final now routes `setGroup`, `getGroup`, `setMaterial`, and `getMaterial` through the internal shape, with group bitmask and scene dirty marking. A staged pair checks the default, a group-5/material-1 mutation, and a second box created directly from group-7/material-2 descriptor values; four registered lines raise the Phase 5 floor to 341. See `docs/reconstruction/novodex-physics/evidence/phase5-box-group-material.md`. This is a bounded slice of the final table, not closure of the box or shape family.

**M2 box flag packet, 2026-09-24:** Public box slots 5 and 6 now match the oracle's 16-bit shape-flag mask, including returning the mask through `NX_BOOL` rather than normalizing to `1`. Default, enable, clear, and descriptor-copy observations add four registered lines and raise the Phase 5 floor to 345. An aimed boolean-normalization mutation failed the staged differential. See `docs/reconstruction/novodex-physics/evidence/phase5-box-flags.md`. Dirty queueing for a cleared auxiliary entry and the remaining public slots are still open.

**M2 box name packet, 2026-09-24:** Public box slots 29 and 30 now use a pointer-preserving global name table. First insert, replacement, clear, descriptor name loading, the 0x10/0x10 allocation pair, and the named actor's five-block release match the pinned oracle. Twelve registered checks raise the Phase 5 floor to 357. An aimed replacement mutation failed the staged differential. See `docs/reconstruction/novodex-physics/evidence/phase5-box-name-registry.md`. Registry growth, stale entries, SDK teardown, other shape finals, and the final-vtable marker remain open.

**M2 empty Scene release, 2026-09-24:** The internal Scene constructor now owns and builds its 0x28-byte public wrapper and the condition's 0x14-byte state block in oracle order. `releaseScene` removes the internal Scene from the SDK array and frees the empty Scene's nine blocks in exact order; SDK release's seven-block tail is also exact. A separate fresh-process target adds six registered checks and raises the Phase 5 floor to 363. See `docs/reconstruction/novodex-physics/evidence/phase5-empty-scene-release.md`. Populated Scene teardown is much larger: 45 oracle frees versus nine candidate frees in the current actor drive, with 14 versus seven SDK-release frees. This is the next ownership dependency, not a claim of Scene closure.

**M2 populated Scene ownership frontier, 2026-09-24:** The candidate now releases both live public actors during `releaseScene`, matching the oracle's `2.2` pointer-free observation. One registered check raises the Phase 5 floor to 364. The candidate currently frees 19 of the oracle's 45 Scene-release blocks and makes none of its two teardown allocations; internal actor destruction, auxiliary arrays, pruners, and SDK name-table teardown remain open. See `docs/reconstruction/novodex-physics/evidence/phase5-populated-scene-teardown-frontier.md` for the full oracle sequence.

**M2 actor metadata follow-up, 2026-09-24:** The concrete actor now propagates descriptor flags and group into the 0x50-byte body and implements public `raiseActorFlag`, `clearActorFlag`, `readActorFlag`, `setGroup`, and `getGroup` virtuals. A fresh-process staged pair checks initial state, mutations, direct body words, and allocation neutrality. Six registered lines raise the Phase 5 floor to 396; the gate remains red only on the explicit final-vtable marker. Setter and descriptor-propagation mutations both failed the differential. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-metadata.md`. Full actor virtual coverage and populated Scene ownership remain open.

**M2 actor body flags and locks, 2026-09-24:** Dynamic record flags, public body-flag slots 78–80, and the record's Scene auxiliary dirty queue now match a fresh-process oracle drive. This exposed and fixed uninitialized public Scene critical sections and a one-byte read of the record's 32-bit flag word. Ten registered lines raise the Phase 5 floor to 406; reader and dirty-bit mutations both fail the differential. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-body-flags.md`. Kinematic mode, lock contention, and the remaining actor virtuals remain open.

**M2 dynamic mass and kinematic follow-up, 2026-09-24:** Explicit-mass dynamic descriptor fields, public mass/damping/velocity getters, one identity-box density mass/inertia computation, and the unshared record's kinematic allocate/restore transition now match the pinned oracle. The new staged differential adds thirteen assertions, raising the Phase 5 floor to 419; four mutations separately falsify mass, density, kinematic allocation, and dirty-bit behavior. See `docs/reconstruction/novodex-physics/evidence/phase5-dynamic-record-and-kinematic.md`. General mass properties, shared-record kinematic transitions, and the other dynamic actor slots remain open.

**M2 dynamic setter follow-up, 2026-09-24:** Public mass, inertia, damping, and velocity setters now match the pinned oracle for valid dynamic inputs, including reciprocal fields, shadow velocities, wake initialization, and six dirty-queue masks. A fresh-process staged differential adds seventeen assertions and raises the Phase 5 floor to 436. A deliberate dirty-mask mutation failed the gate; after restoration the staged pair and Phases 2, 3, 4, 6, and 7 pass. The full Phase 5 gate remains red only on the existing final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-dynamic-setters.md`. Invalid inputs and diagnostics, general wake/sleep behavior, and remaining actor virtuals are open.

**M2 sleep/wake follow-up, 2026-09-24:** Public actor slots 67–74 now reproduce individual/group sleeping, threshold getters/setters, wake and forced-sleep transitions, and their record/dirty-queue state for the probed static, dynamic, and two-record group cases. Twenty-three new staged assertions raise the Phase 5 floor to 459. A wake-bit mutation fails the differential; the restored build passes the eight-target staged comparison, tooling tests, header check, and Phases 2, 3, 4, 6, and 7. The full Phase 5 gate remains red only on the existing final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-sleep-state.md`. Simulation-driven sleep and real joint-connected groups remain open.

**M2 momentum follow-up, 2026-09-24:** Dynamic actor maximum angular speed, linear/angular momentum getters and setters, and kinetic energy now match the pinned oracle for identity and 90-degree rotated box actors. The dynamic record initializes the inertia frame, quaternion-derived rotation, and cached world inverse tensor. A changed-inertia probe establishes that the getter uses new local inertia while the setter still reads the old cached tensor; a deliberate immediate-refresh mutation fails. Thirty-four registered staged lines raise the Phase 5 floor to 493. The nine-target staged comparison, tooling tests, header check, and Phases 2, 3, 4, 6, and 7 pass. The full Phase 5 gate remains red only on the final-vtable marker. See `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-momentum.md`. Arbitrary inertia-frame orientations, force accumulation, and simulation-time matrix refresh remain open.

**M2 global inertia follow-up, 2026-09-24:** Public global inertia and inverse-tensor getters now match the oracle before and after local inertia changes, including the rotated off-diagonal floating-point words and the static actor's identity result. A nonidentity 90-degree mass-local inertia frame is copied into the dynamic record and combined with the body orientation for the cached inverse tensor. Twenty more staged assertions raise the Phase 5 floor to 513; a frame-field mutation fails the differential. The nine-target staged comparison, 628 tooling tests, 80-header hash check, and Phases 2, 3, 4, 6, and 7 pass; the full Phase 5 gate remains red only on the final-vtable marker. See the follow-up in `docs/reconstruction/novodex-physics/evidence/phase5-public-actor-momentum.md`. General combined rotations, translated mass offsets, and simulation-time cache updates remain open.

### Continuation — reverse strided range helper (`phys_fn_000002`, 2026-10-15)

Promoted the existing Phase 5 static-only proof to a direct oracle differential at RVA `0x00001030` against candidate symbol `_phys_fn_000002@16`. Six cases cover positive and negative strides, zero and negative counts, zero stride, and a one-element walk; the fixture compares callback sequence, return value, and x86 stack delta. The restored oracle/candidate digest is `4c02e27d356771c1` with zero mismatches. Changing `address -= stride` to `address += stride` produces candidate digest `9446a40071e62479` and `mismatches=9`. Phase 5 passes at 2,767/2,767 coverage assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase5-range-iteration-000002-20261015/`.


## 1. Inputs and boundaries

- Repository: `D:\github\Novodex`, inspected at `0df8821f1c5e3668e566ad4182b1e8e568f632d6`.
- Verified oracle: `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, 1,253,376 bytes, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- The literal supplied path `D:\FlamingEnt\_\_\Unreal\_3\Binaries` does not exist on this workstation. The verified path above is the one recorded by the existing reconstruction and matches its hash.
- Compatibility target: shipped x86 DLL and SDK identity `2.1.2.6000`; all 41 named exports plus the public C++ virtual interfaces. Building x64 is not evidence for this ABI.
- Immutable: `Physics/include/**` and shared `Foundation/include/**`. Preserve signatures, macros, packing, enum values, inline code, and file bytes. Derive analysis types into the existing Ghidra shim; never edit public headers to accommodate reconstruction or the compiler.
- Foundation remains the baseline implementation/dependency. Re-run its applicable regression gates with the final pair. A discovered Foundation defect becomes an explicit dependency issue; it does not justify changing shared public headers.
- Maintain the existing `NxPhysics`, `NxFoundation`, `NxQhull`, and `NxOpcode` CMake targets. New implementation belongs in private source/header files. Preserve upstream third-party sources; oracle-specific corrections belong under `External/*/novodex/`.
- Product code must build without the oracle DLL, Ghidra, IDA, angr, analysis caches, or runtime forwarding to the original DLL. These tools and binaries support reconstruction and verification only.
- Full reconstruction means source and behavioral coverage of the shipped image. Do not add features solely because a newer PhysX version supports them. A shipped refusal/no-op is valid only when its oracle behavior is established.
- Semantic/ABI compatibility does not require reproducing the original compiler's instruction bytes, section layout, or exact CRT import list. Verify the consumer-facing ABI and audit all runtime dependencies. A modern-runtime import difference must be explicit, not concealed by transcript normalization.

The August umbrella and phase plans remain useful references. This completion plan updates their execution order and stale remaining-work assumptions. In particular, classified data is not unfinished code, and a historical `pass` with explicit deferrals is not full subsystem completion.

## 2. Measured starting point

The inventory validator passes and reports **6,338 function rows**, **5,138 data objects**, and **zero unexplained executable bytes**. These are census facts, not proof that the executable behavior has been reconstructed.

| Historical owner | Code rows | Discovered | Reconstructed | Dynamically gated | Statically reviewed |
|---|---:|---:|---:|---:|---:|
| Phase 2: SDK/core | 143 | 9 | 73 | 55 | 6 |
| Phase 3: geometry/collision | 392 | 131 | 188 | 73 | 0 |
| Phase 4: assets | 1,053 | 254 | 768 | 31 | 0 |
| Phase 5: objects | 205 | 0 | 0 | 204 | 1 |
| Phase 6: joints/effectors | 433 | 19 | 263 | 151 | 0 |
| Phase 7: scenes/simulation | 561 | 210 | 294 | 56 | 1 |
| **Total** | **2,787** | **623** | **1,586** | **570** | **8** |

The other 3,551 function rows are currently classified compiler artifacts. All 5,138 data objects are currently classified. The full audit must validate those classifications; three known exceptions below already show why classification is not unquestionable. No code row is yet at the final `closed` state. The phase ledgers currently recognize 518 rows at intermediate proof levels; final closure remains reserved for Phase 8.

Important qualifications and concrete gaps:

- The 629 discovered rows are not 629 functions necessarily needing fresh handwritten implementations. The Phase 4 ledger records 722 vendored qhull/OPCODE rows, including 633 code rows lacking row-specific falsification. Reuse their source and prove correspondence.
- `NpActor.cpp`, `NpScene.cpp`, and `NpJoint.cpp` contain respectively 84, 63, and 26 explicit `(unimplemented)` markers. This is a source triage count, not a substitute for a complete API audit. `NpActorVtable::isDynamic()` currently always returns true; scene `simulate` and `fetchResults` are placeholders.
- `NpPhysicsSDK.cpp` still has unfinished scene release/enumeration, triangle-mesh factory/release, actor-group pair, and core-dump paths.
- The earlier Phase 5 `batch3268 candidate failures=3` gate failure is resolved; the current Phase 5 gate passes at 2,621 assertions. Keep its prior diagnostic record as historical evidence rather than an open blocker.
- `phys_fn_005493` (`Segment::SquareDistance`) is correctly classified as Phase 4 code and its source reconstruction now has a caught row-specific mutation (`mismatches=40000`); DLL-level routing remains open. `phys_fn_005517` (`AABBTreeNode::_BuildHierarchy`) and `phys_fn_005523` (`AABBTree::Build`) are now separately mapped and falsified through the public triangle-mesh/PMap fixture; see `evidence/phase4-aabbtree-005517-005523.md`.
- Existing PMap fixtures leave all cells at `0xffffffff`, so they cannot prove the reconstructed arithmetic. Existing numeric proofs also carry x87 discrepancies and some were measured only outside the simulation floating-point environment.
- Some historical mutations prove only that a row was entered, not that an incorrect result would be detected. Several proofs lack retained raw runs or reproducible edits. These are final-audit debts.
- `program.json`, ledger notes, and old phase plans contain stale prose/counts. For example, the Phase 7 ledger's note says it closes nothing while its entries contain four closures. Derive current progress from entries and checked evidence.

## 3. Approach choice

| Approach | Benefit | Limitation |
|---|---|---|
| **Dependency-ordered slices using the existing census — recommended** | Preserves prior work, resolves cross-phase dependencies, and proves behavior through the actual DLL early | Requires an accurate indirect-call and object-lifetime map |
| Finish historical phases strictly in numerical order | Familiar ownership and reporting | SDK, objects, assets, and scenes depend on each other; partial phase passes have already left substantial debt |
| Bulk-decompile the remaining rows and integrate afterward | Produces a large reference corpus quickly | Does not resolve layouts, calling conventions, product linkage, runtime state, or behavior; creates a large integration backlog |

Use the first approach. Retain historical phase IDs for provenance and reports; use the milestones below for scheduling. A slice may implement prerequisites owned by multiple phases without pretending those phases are complete.

## 4. Evidence and implementation loop

Apply this loop to a bounded family: one ownership/lifetime path, shape-pair dispatch, mesh operation, joint family, or solver kernel. Avoid giant source dumps or adding unrelated blocks to a single test function.

1. Select stable IDs and merge their continuation chunks for analysis. Follow direct calls, tail calls, constructor-installed vtables, callbacks, dispatch tables, and receiver-specific virtual targets. Mark unresolved indirect edges explicitly. Group mutually dependent functions so scheduling does not deadlock on cycles.
2. Recover the contract from disassembly and decompilation: arguments, calling convention, stack cleanup, return convention, `this` adjustments, allocation sizes/alignment, fields, aliasing, callbacks, failures, and floating-point state. Update `object_model.json` only from evidence.
3. Capture deterministic oracle cases that distinguish competing hypotheses. Prefer real public SDK-created objects. Use synthetic fixtures only when their layouts, dispatch, and ownership preconditions have independent proof.
4. Transcribe maintainable private C++ with named fields/helpers where established, preserving observed sequencing and ownership. Record stable IDs beside implementations. Do not guess missing semantics or convert an unsupported path into a silent successful default.
5. Build via CMake. Compare the oracle with both focused internal implementations and the **actual rebuilt DLL** through its public interfaces. An executable that compiles a second copy of the source is useful local evidence, but cannot prove the DLL routes to that implementation.
6. Perform a behavior-changing mutation within the row under test and require a registered check to detect it. Attribute the failure to that row's observable result, not a callee, added print, loader failure, or harness crash. For genuinely static properties, use the appropriate independently checked structural proof.
7. Retain the exact input seed/data, oracle/candidate hashes, source revision, mutation patch, commands, stdout/stderr, exits, and non-mutated controls. Force a rebuild of mutation variants; stale objects from archive timestamps invalidate measurements.
8. Re-read the successful transcription against the listing for signed zero, NaN payloads, store precision, unusual cleanup, and branches absent from the fixtures. Update inventory and proof records, then commit the bounded change.

Use the existing project `D:\FlamingEnt__\novodex-analysis\novodex-physics\PhysicsOracle.gpr` and the checked-in normalized manifests. Prefer Ghidra's typed decompiler with independent Capstone instruction checks. Use IDA for an unresolved boundary/type disagreement or angr to construct inputs for a bounded hard-to-reach branch; neither output alone closes a row. Record tool versions and address conventions. Avoid restarting the entire census unless pin verification or a reproducibility check requires it.

## 5. Completion milestones

### M0 — Establish a reproducible backlog and correct known accounting errors

**Files:** existing `docs/reconstruction/novodex-physics/{inventory.json,program.json,object_model.json}`, `gates/phase*-closure.json`, `tools/validate_inventory.py`; proposed `completion-backlog.json` and `tools/report_completion.py` in the same evidence tree.

- [x] Preserve unrelated working-tree changes. Recorded execution-start commit `735ea43a` and used the attached isolated worktree `aabbtree-proof`; only the scoped AABB-tree proof packet was committed and fast-forwarded. Existing main-checkout edits, scene dumps, candidate/oracle folders, and local draft were preserved.
- [x] Ensure every required Foundation evidence manifest and pinned third-party input has a reproducible location; clean-checkout verification must not silently depend on untracked files or incidental build caches. Vendored-source checks now discover the committed `External/{qhull,opcode}/upstream` trees and verify every file against the committed, hash-pinned manifests; inventory validation uses those same tracked roots. The clean-archive proof is recorded in `docs/reconstruction/novodex-physics/evidence/completion-backlog-m0-2026-10-07.md`.
- [x] Reproduce the current gates, preserving failures and skips. Fresh Win32 `completed` (Phases 1–5), standalone Phase 5, Phase 6, and Phase 7 runs passed from main; the all-scenes Viewer sweep preserves its five known asset skips. Evidence: `docs/reconstruction/novodex-physics/evidence/completion-backlog-m0-2026-10-07.md` and `docs/reconstruction/novodex-physics/evidence/viewer-all-scenes-effector-order-2026-10-09.md`.
- [x] Build a machine-generated backlog for every code row: implementation/linkage, public reachability, unresolved dependencies, applicable contexts, evidence strength, and next packet. Count functions and bytes separately. Current snapshot: `docs/reconstruction/novodex-physics/completion-backlog.json`.
- [x] Re-evaluate the three named artifact misclassifications against their recorded callers and source correspondence. Correct kind/ownership/proofs consistently; do not bulk-reclassify other artifacts by analogy. `phys_fn_005493` now also has a row-specific mutation proof; `phys_fn_005517` and `phys_fn_005523` now have separate public-DLL mutation proofs through triangle-mesh/PMap creation; the registered Phase 4 gate requires the creation marker and passes at 530/530.
- [x] Audit referenced data types, tables, and constants. All 5,138 data rows are classified under 11 recognized types with structurally validated proofs; no inconsistency was found. Reopen only with concrete contrary evidence. See `docs/reconstruction/novodex-physics/evidence/completion-backlog-m0-2026-10-07.md`.
- [x] Make reporting separate intermediate gate coverage, full code closure, artifact classification, and data classification. The completion reporter publishes these as separate counters and the inventory/report suites reject hidden deferrals and unsupported promotion; refreshed-ledger snapshot tests pass. See `docs/reconstruction/novodex-physics/evidence/completion-backlog-m0-2026-10-07.md`.

**Exit:** every remaining code row has an actionable packet; totals reconcile; the known classification errors are resolved from evidence; no historical partial pass is reported as completion.

### M1 — Make the verification harness reliable and prove real DLL coverage

**Files:** `tests/PhysicsObjectLayoutTests.cpp`, `tests/PhysicsAssetTests.cpp`, `tests/PhysicsPairLoader.h`, `CMakeLists.txt`, `tools/{gate_targets.ps1,run_phase_gate.ps1,run_differential.ps1}`. Proposed focused tests: `tests/PhysicsObjectLifecycleTests.cpp` and `tests/PhysicsSceneLifecycleTests.cpp`.

- [x] Re-evaluate the historical Phase 5 failure from a fresh build. The old synthetic vtable failure no longer reproduces; standalone Phase 5 and `completed` pass on a fresh Win32 build. The aggregate run exposed and fixed duplicated shared-target coverage accounting. Continue the ABI/call-adapter and full lifecycle audits below. Evidence: `docs/reconstruction/novodex-physics/evidence/completed-gate-coverage-floor-2026-10-09.md`.
- [ ] Separate actual candidate discrepancies from harness defects. Split new lifecycle fixtures into small functions/translation units with explicit ownership. Do not move the entire historical test frame blindly or weaken coverage to turn it green.
- [ ] Validate internal call adapters against the oracle calling convention, stack balance, nonvolatile registers, and structure-return ABI. **Static cleanup audit complete:** corrected fixture typedef/arity mismatches; the pinned-byte scanner follows reachable in-row control flow, uses exact inventory function bounds, resolves bounded switch tables, three exact pinned indirect-tail targets, and one byte-pinned direct-tail destination row. It now verifies 278/278 cast sites with zero unresolved rows or cleanup mismatches and is a fail-closed gate on every phase run. A raw-call probe checks stack cleanup and EBX/ESI/EDI/EBP preservation for oracle and candidate `phys_fn_001329`; deliberate register-clobber and wrong-cleanup controls are both detected. The three tail adapters now verify `StaticCollisionDestroy(void*)`, the Observable destructor, and `Observable::event(unsigned, Observable&)`, including forwarded values. Structure-return ABI is exercised for all 19 `NxActor` returns and all six `NxShape` pose returns; the Phase 6 joint-family differential probes both `NxJoint` returns for all ten families. The two actor point-velocity methods are checked with a visible stack argument. Evidence: `docs/reconstruction/novodex-physics/evidence/abi-call-cleanup-2026-10-09.md`, `docs/reconstruction/novodex-physics/evidence/abi-indirect-tail-signatures-2026-10-09.md`, `docs/reconstruction/novodex-physics/evidence/abi-call-adapter-probe-2026-10-09.md`, `docs/reconstruction/novodex-physics/evidence/abi-structure-return-probe-2026-10-09.md`, `docs/reconstruction/novodex-physics/evidence/phase6-joint-structure-return-abi.md`, and `docs/reconstruction/novodex-physics/evidence/abi-direct-tail-cleanup-000649-2026-10-15.md`. The six fluid-emitter aggregate rows are now exercised through an object constructed by the candidate DLL's map-resolved constructor and dispatched through its DLL-owned `NxFluidEmitter` vtable. Raw calls match the oracle's hidden result pointer and stack cleanup, and a DLL-only local-position offset mutation is detected by raw and normal interface calls. This verifies those six getter implementations and ABI only; the disabled factory and remaining emitter API stay open. M1 remains open for broader runtime adapter/lifecycle coverage and other registered harness/proof gaps. Restore stack-protection checks for repaired fixture code; do not broaden `/GS-` as a workaround.
- [ ] Add actual-DLL SDK/scene/actor construction and destruction tests, then route each subsequent public feature through them. Audit that candidate symbol resolution uses candidate symbols/maps, never oracle RVAs applied to the candidate.
- [x] Extend the runners to accept repository, build, oracle, and output roots so isolated worktrees run their own binaries. Preserve defaults for existing usage. Missing cases/targets, skipped phases, timeouts, and crashes remain failures or explicit skips; separate worktree and main-checkout runs verified these paths. Evidence: `docs/reconstruction/novodex-physics/evidence/completed-gate-coverage-floor-2026-10-09.md`.
- [x] Pin oracle-only input/output digests and case counts independently of candidate output. Persist machine-readable proof artifacts with hashes rather than relying on matching numbers in prose. `NxPhysicsJointSupportTests`, `NxPhysicsJointDescTests`, the 122-case ten-family `NxPhysicsJointTests` matrix, the 84-case BOX-sweep `NxPhysicsShapeVtableTests`, `NxPhysicsObjectLayoutTests`, and `NxPhysicsFluidEmitterAbiTests` all have retained proofs bound to the pinned oracle DLL and normalized fixture sources. `NxPhysicsAssetTests`, `NxPhysicsThirdPartyTests`, and `NxPhysicsCollisionTests` now have the remaining target-level proofs: Asset pins 32 cases and its oracle digest; ThirdParty pins 116 driven cases, 27 coverage markers, and digest `e2a266c1`; Collision pins 108 input-digest rows, 125 coverage labels, and 136 oracle result summaries while filtering candidate-only fields. Fresh Phase 3 and Phase 4 gates verify Collision at 561/561 and ThirdParty at 519/519. Evidence: `docs/reconstruction/novodex-physics/evidence/oracle-only-baselines.json`, `docs/reconstruction/novodex-physics/evidence/oracle-only-proofs/`, and the dated raw proof archives.

**Exit:** the reproduced layout failures are explained and fixed without suppressing cases; small lifecycle tests are stable; mutations to the built DLL's dispatch or behavior fail the public tests.

### M2 — Complete SDK, scene ownership, actors, bodies, shapes, and materials

**Files:** `Physics/src/{PhysicsSDK.cpp,NpPhysicsSDK.cpp,Scene.cpp,NpScene.cpp,ObjectModel.cpp,NpActor.cpp}`, their headers under `Physics/src/include/`, `object_model.json`, and lifecycle tests introduced in M1.

- [ ] Resolve the actor secondary base at `+8` and remaining shape-tail/BoxHullFacade questions listed in `object_model.json`. **Resolved within this item:** `phys_fn_000038` and `phys_fn_000040` are `NxActor::getPointVelocityVal` and `getLocalPointVelocityVal`; the shape pose at `+0x6c` is its owner-relative local pose, loaded from and saved to `NxShapeDesc::localPose`; `phys_fn_000973` builds six box-face records, with nearby integer arrays classified as data rather than descriptor vtables; the actor's `+8` member is the shared 12-byte secondary base represented privately as `EmbeddedHookBase` (the original source identifier is not recoverable from the stripped image). The exact consumer of the facade's second index family remains open. See the actor-vtable and Phase 5 object-model evidence.
- [ ] Complete SDK/scene create, enumerate, release, ownership unwind, locks, allocation-failure behavior, and Foundation reference handling, including repeated create/destroy cycles and partially initialized objects.
- [ ] Replace actor placeholders with recovered static/dynamic behavior, poses, velocities, forces/impulses, mass/inertia, flags, sleep controls, shape ownership, and descriptor round trips.
- [ ] Complete each shipped shape family and its final virtual table. Keep constructor/destructor transition tables distinct from final dispatch; test base adjustments and deleting destructors.
- [ ] Complete materials, group/pair registries, and notifications that object creation and collision require. Verify error codes, callback arguments/order, allocation accounting, and invalid-descriptor behavior.
- [ ] Use named private layout types only when offsets/alignment are established. Split `ObjectModel.cpp` by recovered responsibility when needed for this work, retaining stable-ID mappings and existing proof coverage.

**Exit:** public lifecycle and object-state tests cover all shipped actor/shape kinds, including static actors, nonidentity transforms, failed construction, and complete release. No fixture-only constant behavior substitutes for the oracle.

### M3 — Finish mesh/convex assets, PMaps, streams, and embedded libraries

**Files:** `Physics/src/{TriangleMesh.cpp,PMap.cpp,MemoryStream.cpp,ThirdPartyHost.cpp}`, `Physics/src/opcode/`, `External/{opcode,qhull}/novodex/`, asset/third-party tests, and source correspondence evidence.

- [ ] Complete SDK-facing mesh factories/releases and connect private mesh code to the public API; recover load/save formats, stream ownership, endian markers, validation, and lifetime behavior.
- [ ] Reconstruct NovodeX-specific wrappers and changes around qhull/OPCODE. Prove source correspondence for vendored families; do not treat being linked as equivalence.
- [ ] Correct the recorded `Segment::SquareDistance` behavior and prove hierarchy-building modifications after reclassifying their rows in M0. `phys_fn_005517` and `phys_fn_005523` are now individually falsified; see `docs/reconstruction/novodex-physics/evidence/phase4-aabbtree-005517-005523.md`.
- [ ] Build PMap inputs with both occupied and empty cells and observable arithmetic outcomes. Close arithmetic only when a mutation affects an asserted result; an all-sentinel grid is insufficient.
- [ ] Exercise acceleration-tree queries, degenerate and large meshes, convex construction, malformed/truncated streams, repeated ownership transfer, and allocation failures. Run numeric helpers under every context from which simulation reaches them.

**Exit:** real public mesh objects support serialization and runtime queries; all asset/library behavior has source correspondence and appropriate static/dynamic evidence, including the 633 currently unproven vendored code rows after accounting corrections.

### M4 — Complete collision, spatial queries, filtering, and contact generation

**Files:** `Physics/src/{Geometry.cpp,NarrowPhase.cpp,ContactGeneration.cpp,SmoothNormals.cpp}`, shape wrappers, collision tests; proposed private `BroadPhase.cpp` and `Filtering.cpp` only where recovered ownership supports those boundaries.

- [ ] Recover the remaining broad-phase update/removal, pair persistence, collision-group/filter, and continuous-collision dependencies.
- [ ] Complete mesh and compound dispatch paths in both recovered matrices and every remaining primitive path. Resolve targets per concrete receiver type and include continuation chunks in the dependency walk.
- [ ] Implement public ray, overlap, sweep, and callback routing through actual shapes/scenes. Verify result ordering, hit flags, buffers, and early termination.
- [ ] Complete manifold/contact creation and persistence with correct layouts, material pairing, normals, depth, friction/restitution inputs, and cleanup.
- [ ] Re-run kernels reachable from stepping under both `0x027f` and `0x0f7f` and any additional observed contexts. Close recorded x87 discrepancies by examining generated assembly and spill lifetimes; a pinned mismatch is still a mismatch.

**Exit:** all shipped shape-pair/query paths are mapped, implemented, and driven; contact outputs and lifetime behavior agree through the DLL. Unknown indirect edges remain open work.

### M5 — Complete joint and effector contracts and constraint preparation

**Files:** `Physics/src/{JointDesc.cpp,NpJoint.cpp,Scene.cpp}`, private joint headers, `tests/PhysicsJointTests.cpp`; introduce separate private joint-family/constraint files as their oracle boundaries become known.

- [ ] Recover each joint family present in the image, including public subtype casts, descriptors, limits, motors, springs, breakability, collision flags, actor rebinding, serialization, and release.
- [ ] Replace the current generic joint vtable's descriptor echoes and constant state with the recovered object behavior. Test world-space results with translated/rotated actors, not only identity fixtures.
- [ ] Complete effectors and shared constraint-row preparation consumed by the solver. Establish row layout, Jacobians, warm-start storage, and ownership before integrating stepping.
- [ ] Extend descriptor tests to distinguish malformed input behavior and degenerate axes where the oracle defines an observable result. Keep unavailable shipped features faithful to their observed refusal behavior.

**Exit:** every shipped joint/effector family has real construction, state, mutation, and cleanup tests, plus validated constraint preparation. Dynamic joint trajectories are required by M6 before final closure.

### M6 — Complete scene scheduling and the simulation pipeline

**Files:** `Physics/src/{Scene.cpp,NpScene.cpp}` and private scene headers; proposed `Islands.cpp`, `Solver.cpp`, `Integration.cpp`, `Sleeping.cpp`, `SceneQueries.cpp` as justified by recovered units; proposed `tests/PhysicsSimulationTests.cpp`.

- [ ] Start from `phys_fn_000659` at RVA `0x00013c40` and all of its direct/indirect dependencies. Recover step state transitions, timing/substep policy, gravity, queues, fences, callbacks, locks, and floating-point save/restore.
- [ ] Implement `simulate`, `checkResults`, `fetchResults`, `wait`, and writable-state behavior, including repeated calls, blocking/nonblocking behavior, invalid transitions, and release during supported states.
- [ ] Complete island formation, constraint assembly, contact/joint solve, iteration order, velocity/pose integration, kinematic targets, sleeping/waking, and cleanup.
- [ ] Compare every frame of deterministic scenarios: free fall, contact/resting, stacking, friction/restitution, each joint/motor/limit family, sleeping/waking, actor deletion, mesh contact, and CCD cases.
- [ ] Assert callbacks, errors, flags, contact streams, object counts, and allocations as well as trajectories. Compare ordered output where the oracle defines order. Measure repeat-oracle variation before considering any tolerance.

**Exit:** the actual rebuilt DLL simulates all representative scenarios with correct state transitions, callbacks, resource lifetime, and numerics. Passing a creation-only joint test does not establish simulation completion.

### M7 — Exhaust the remaining tree and finalize every public interface

**Files:** all remaining implementation packets from `completion-backlog.json`, including `FluidSupport.cpp`, SDK diagnostics/debug/performance/core-dump paths, and any newly identified reachable code.

- [ ] Generate the residual list from the inventory, not from Unreal smoke-test coverage. Complete rarely used APIs, callbacks, error/unwind paths, optional compile-time surfaces actually present in the oracle, and independently reachable internal functions.
- [ ] Audit every public virtual slot and export against its concrete receiver types. Remove reconstruction placeholders; retain actual oracle stubs only with named proof.
- [ ] Discharge earlier-phase deferrals through the gates that actually execute their proofs. Preserve the original ownership/provenance; do not move work into a completed bucket to improve totals.
- [ ] Audit compiler artifacts, data classifications, and source-to-oracle mappings, including product behavior concealed by mistaken artifact labels. Account for inlining and shared/compiler-generated helpers without inventing one-to-one correspondence.
- [ ] Resolve recorded numeric and Foundation-boundary escalations. Document shipped public-header defects as compatibility behavior without editing the frozen headers.

**Exit:** no unimplemented product path, unknown required dispatch, unowned code row, or unresolved behavioral discrepancy remains. Every code row has sufficient evidence for final audit.

### M8 — Perform the full semantic, ABI, and consumer audit

**Files:** `tools/validate_inventory.py`; proposed `tools/{verify_static_proofs.py,run_final_verification.ps1,run_consumer_smoke.ps1}`, `gates/{phase8-closure.json,final.json}`, candidate/source-map manifests, and `closeout.md` under the Physics evidence tree.

- [ ] Add the Phase 8 ledger and final validation that promotes code to `closed` only with valid source linkage, actual-DLL coverage or justified static proof, reproducible behavioral mutations, resolved dependencies, and evidence for its execution contexts. Keep justified data/artifact rows `classified`.
- [ ] Regenerate normalized oracle/source evidence as needed from pinned inputs and verify reproducibility. Reject missing raw proof artifacts, stale hashes, skipped suites, placeholder APIs, and mutation claims based only on entry prints.
- [ ] Build from clean recorded source with CMake. Verify all 41 export names/ordinals, calling conventions, structure returns, public layouts/packing, concrete vtables, and Foundation linkage. Record and review any extra symbols and runtime imports explicitly.
- [ ] Pin one retained Foundation/Physics pair and run the complete Foundation and Physics suites against those exact bytes, including multi-frame tests. Do not rebuild between certification and consumer testing.
- [ ] Exercise a minimal public-SDK consumer and an Unreal scenario with actual physics activity. The approved viewer smoke sweep has run against all 39 available `ViewerScenes/**/*.pds.ods` entrypoints; five existing asset-backed skips remain explicit in CTest, as recorded above. Stage an isolated consumer copy where practical; any live replacement must use exact backup/hash checks and guaranteed restoration. Load-only viewer runs cover scene startup/render/shutdown but do not replace active-physics or Unreal testing.
- [ ] Verify the retained artifacts again without rebuilding or writing evidence; record final counts, commands, binary hashes, source revision, consumer results, and restoration state.

**Exit:** every code row is closed, every artifact/data classification is justified, all required suites pass, public headers are unchanged, and the retained CMake-built pair passes representative consumer execution.

## 6. Ordering, packet size, and progress

Primary order: **M0 → M1 → M2 → M3 → M4 → M5 → M6 → M7 → M8**. Implement scene/object prerequisites in M2 without waiting for the full solver. M5 validates joint construction and constraint preparation; M6 supplies the dynamic evidence needed for final joint closure. Later discoveries may reopen an earlier slice's proof without erasing its historical results.

Each execution packet should fit one cohesive contract and identify stable IDs, files, prerequisites, exact test invocation, expected oracle observations, and closure artifacts. Derive packet-level implementation details from recovered contracts; this plan does not invent C++ bodies for functions whose semantics remain unknown. The historical subsystem plans supply background, while the live backlog supplies scope.

The first execution packet is M0's baseline/reclassification report, followed by the smallest reproducible Phase 5 failure in M1. Establish measured throughput on these and one complete public lifecycle slice before estimating the rest. A row count alone cannot predict effort: a tiny ABI thunk, vendored tree builder, and solver kernel have very different costs.

Report newly mapped, implemented, DLL-driven, and fully closed rows separately; include open indirect edges, failing cases, and unresolved numeric contexts. Prioritize work that removes a named dependency for another slice. Avoid repeated narrative-only updates that leave the source and measured proof unchanged.

## 7. Baseline commands and verification recorded for this plan

Run from `D:\github\Novodex`. These commands are existing interfaces and were run during planning:

```powershell
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --root Physics/include --manifest docs/reconstruction/novodex-physics/public_header_hashes.json
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --root D:\FlamingEnt__\Unreal_3\Development\External\Novodex\Physics\include --manifest docs/reconstruction/novodex-physics/public_header_hashes.json
python docs/reconstruction/novodex-physics/tools/verify_toolchain.py --ghidra-home D:\ghidra --pin docs/reconstruction/novodex-physics/analysis_toolchain.json
cmake -S D:\github\Novodex -B D:\github\Novodex\build\completion-plan-baseline -G "Visual Studio 18 2026" -A Win32
cmake --build D:\github\Novodex\build\completion-plan-baseline --config Release --target NxPhysics --parallel 8
```

Results: inventory validation passed; both public-header checks passed for 80 files; toolchain verification passed; a fresh MSVC Win32 Release configure/build succeeded. Build log: `D:\github\Novodex\build\completion-plan-baseline\build-NxPhysics.log`.

The fresh build proves the current source builds, not semantic completion. Full dynamic suites were **not** re-run during planning; the Phase 5 failures and numeric discrepancies above are attributed to checked-in evidence and must be reproduced in M0/M1. No public header or production source was changed and no consumer binary was replaced.

For execution, the existing regression entry points are:

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase completed
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5
```

The current runners hard-code the primary repository/build roots; M1 must parameterize them before they can verify another worktree. `completed` currently covers passing historical phases through Phase 4 and is not a whole-DLL gate. Record Phase 5's actual result rather than expecting historical failure text to remain identical after a rebuild. Phase 8's final runner and the newly proposed test targets do not yet exist.

## 8. Completion checklist

- [ ] Complete shipped function/data tree accounted for, including corrected classifications and rarely used paths.
- [ ] All public headers byte-identical to their original manifests.
- [ ] Maintainable private C++ builds as `NxPhysics.dll` through CMake and links the reconstructed Foundation.
- [ ] No oracle forwarding, reconstruction placeholders, or unproven vendored behavior.
- [ ] Full export and public virtual ABI verified on Win32.
- [ ] All code rows terminally closed; data and artifacts structurally justified.
- [ ] Reproducible proofs test behavior, real DLL integration, floating-point contexts, failures, and lifetime.
- [ ] Multi-frame simulation and actual consumer physics verified on the exact retained pair.
- [ ] Source revision, hashes, evidence, and build/test instructions sufficient to reproduce the result.


**M2/M4 follow-up — BOX slot-5 raycast closure, 2026-10-08:** Closed `phys_fn_000949` (`BoxShape::nxBoxRaycast`) with the public-SDK `NxPhysicsSceneRaycastTests` differential. Reversing the hit-normal hint branch produced `stdout_delta=508` with equal zero exits and exact stderr; the restored run returned to `stdout_delta=0`. Phase 5 now requires the raycast test's full 249-line pinned coverage corpus. A fresh Win32 Release Phase 5 gate passes all 19 staged targets, its static/oracle checks, and 2,560/2,560 coverage assertions. Inventory is 6,338 functions / 5,138 data objects / zero unexplained; Phase 5 is 162 closed / 43 reconstructed-but-unfalsified with 122 data objects still undispositioned. Public Physics headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-box-raycast-000949.md`.


**M2 follow-up — box-hull shared-hook accessor, 2026-10-08:** Closed `phys_fn_000985` (`BoxHullFacade::sharedHook`) through the pinned-oracle `NxPhysicsObjectLayoutTests` differential. Returning null instead of the stable three-word block produced `mismatches=1`; restoring the block returned to `mismatches=0`. Phase 5 now records 163 closed / 42 reconstructed-but-unfalsified rows, remains pending, and still has 122 data objects to disposition. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-box-sharedhook-000985.md`.


**M2 follow-up — capsule save-to-state row, 2026-10-08:** Closed `phys_fn_000991` (`CapsuleShape::nxCapsuleSaveState`) using the pinned-oracle `NxPhysicsObjectLayoutTests` differential. Replacing the row's base-save return with false is caught (`morerows candidate okCap=0`, `mismatches=1`); restoring the tail call returns to `okCap=1`, `mismatches=0`. Its existing record fixture uses the default zero capsule fields, so non-default field values remain unproven. Phase 5 advances to 164 closed / 41 reconstructed-but-unfalsified rows; it remains pending. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-capsule-save-000991.md`.


**M2 follow-up — box-family collision-object constructor, 2026-10-08:** Closed `phys_fn_001075` by mutating its owner-pointer store in the registered pinned-oracle object-layout differential. Replacing `+0x18 = argument` with null makes the BoxShape child-object assertion fail and produces `mismatches=6`; restoring the store returns to `mismatches=0`. The generic constructor `phys_fn_001193` remains separately owned by Phase 3. Phase 5 advances to 165 closed / 40 reconstructed-but-unfalsified rows and remains pending. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-box-collision-object-ctor-001075.md`.

**M2 follow-up — base shape save-to-descriptor, 2026-10-08:** Closed `phys_fn_001277` with a mutation against the existing pinned-oracle `NxPhysicsObjectLayoutTests` fixture. Replacing the +0xde halfword load with zero changes the descriptor digest from `bc9dc964` to `6e8a0c5c` and yields `mismatches=6`; restoring it returns to the exact digest and zero mismatches. Phase 5 advances to 166 closed / 39 reconstructed-but-unfalsified rows and remains pending. The fixture currently uses the default saved word value 8; other flag combinations remain useful future coverage. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-base-save-001277.md`.

**M2 follow-up — ShapeBase constructor, 2026-10-08:** Closed `phys_fn_001273` with the registered pinned-oracle `NxPhysicsObjectLayoutTests` poisoned-object differential. Changing the +0xde default from 8 to 9 changes the candidate digest from `de5f5000` to `0461ca69` and yields 15 mismatches; restoring it returns the exact digest and zero mismatches. Phase 5 advances to 167 closed / 38 reconstructed-but-unfalsified function rows and remains pending. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-shapebase-ctor-001273.md`.

**M2 follow-up — shape apply-group mask, 2026-10-08:** Closed `phys_fn_001329` by mutating the prunable group-mask assignment under the existing pinned-oracle `NxPhysicsObjectLayoutTests` report-and-state fixture. The zero-mask mutant changes the digest from `b6f879ec` to `b17097cc` and yields one mismatch; restoring the shift returns to the exact digest and zero mismatches. Phase 5 advances to 168 closed / 37 reconstructed-but-unfalsified function rows and remains pending. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-apply-group-001329.md`.

**M2 follow-up — base descriptor-load row, 2026-10-08:** Added a row-targeted pinned-oracle `NxPhysicsObjectLayoutTests` fixture for `phys_fn_001347` using non-default pose, flags, group, and material values. A zero flags-copy mutant changes the row digest from `11e5e856` to `b866a954` and yields one mismatch; the restored differential is exact. The Phase 5 gate pins both oracle and candidate outputs and now passes 2,562 assertions against a 2,561 floor. Phase 5 advances to 169 closed / 36 reconstructed-but-unfalsified rows and remains pending. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-apply-descriptor-001347.md`.

**M2 follow-up — actor interface vtable shape, 2026-10-08:** Removed the private candidate-only `setGlobalPose(const NxVec3&, const NxMat33&)` virtual and empty body from `NpActorVtable`. The immutable `NxActor` interface and shipped primary actor table have only the `NxMat34` overload; the extra candidate slot displaced the adjacent member-table boundary. The rebuilt Release map contains the `NxMat34` symbol and no `NxVec3`/`NxMat33` overload. The staged actor lifecycle differential is exact (`stdout_delta=0`, `stderr_exact=True`), and the full Phase 5 gate passes 2,563/2,563 coverage assertions, including its object-layout oracle check. This corrects candidate ABI shape but does not close the full actor vtable or Phase 5. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-actor-interface-vtable-shape.md`.

**M2 follow-up — actor collision-object helper, 2026-10-08:** Closed `phys_fn_000019` (`nxBodyCollisionObject`) with the pinned-oracle `NxPhysicsObjectLayoutTests` differential. Changing the kind-5 mesh branch to return `shape+0x9c` instead of the mesh collision object at `shape+0xf0` produces `actorsm candidate ok=0` and `mismatches=1`; restoring it returns the exact `1bdc2fa8` actor digest and zero mismatches. The Phase 5 ledger now records 170 closed / 35 deferred functions; the full gate passed all 2,563/2,563 assertions. This is direct helper falsification in the object-layout harness; the separate Phase 8 audit still must prove public-DLL routing for every reachable row. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-actor-collision-object-000019.md`.

**M5 D6 breakability setter follow-up, 2026-10-09:** Added a public D6 setBreakable/getBreakable check. A forced-zero maxForce mutation changes the exact readback and is caught by the registered joint staged-pair differential. Evidence: docs/reconstruction/novodex-physics/evidence/phase6-joint-row004445-d6-set-breakable.md.

**M5 D6 limit-point setter follow-up, 2026-10-09:** Added a public D6 setLimitPoint/getLimitPoint readback. Forwarding a zero point is caught by the staged-pair differential (stdout_delta=2); the restored control is exact. Evidence: docs/reconstruction/novodex-physics/evidence/phase6-joint-row004447-d6-set-limit-point.md.

**M5 D6 limit-plane follow-up, 2026-10-09:** The D6 public fixture observes the pinned SDK's ddLimitPlane refusal and empty iterator. A forced-true return mutation is caught; evidence records that this closes the refusal result for the exercised case, not successful D6 insertion. Evidence: docs/reconstruction/novodex-physics/evidence/phase6-joint-row004449-d6-limit-plane-refusal.md.

**M5 D6 name setter follow-up, 2026-10-09:** Added a public D6 setName/getName check. A no-op setter mutation leaves a null name and is caught by the staged-pair differential (stdout_delta=2); the restored control is exact. Evidence: docs/reconstruction/novodex-physics/evidence/phase6-joint-row004453-d6-set-name.md.


### M5.8 — D6 descriptor save row

Closed `phys_fn_004459` (`NpD6Joint::saveToDesc`) using the existing D6 staged-pair save/readback fixture. The no-op mutation was detected at `stdout_delta=92`; the restored differential is exact. This adds no new fixture line or gate assertion. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004459-d6-save-to-desc.md`.


### M5.9 — D6 drive-position write-lock path

Extended the existing staged-pair fixture to mark the scene lock as owned by another thread and observe `NpD6Joint::setDrivePosition` reporting `NXE_INVALID_OPERATION` at line 40. Suppressing the row's report was caught at `stdout_delta=24`; the restored differential is exact. Registered one additional coverage line in each of the Phase 6 and Phase 7 gates. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004461-d6-drive-position-lock.md`.


### M5.10 — D6 drive setter write-lock paths

Expanded the staged-pair contention case to cover D6 orientation, linear-velocity, and angular-velocity wrappers. Mutating each callback report is caught at `stdout_delta=24`; the restored differential is exact. Three Phase 6/7 coverage lines were registered. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004463-004467-d6-drive-locks.md`.


### M5.11 — D6 wrapper constructor

Closed `phys_fn_004469` using the existing staged-pair lifecycle fixture. A null internal-pointer mutation causes a candidate access violation (`stdout_delta=2916`); the restored differential is exact. No new coverage line was needed. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004469-d6-constructor.md`.


**M5.12 — Fixed-joint break support flags (`phys_fn_004091`):** The registered four-step public break simulation now falsifies the support-record flag loop with an immediate-return mutation (`stdout_delta=8`, both exits zero, exact stderr); its restored control is exact. Phase 6 advances to 84 closed / 349 deferred function rows. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004091-break-support-flags.md`, and `docs/reconstruction/novodex-physics/evidence/phase6-d6-row004204-descriptor-load.md`.


**M5.13 — D6 descriptor loader (`phys_fn_004204`):** The existing public staged-pair D6 cases provide non-default descriptors. An immediate-return mutation leaves private fields uncopied and is detected across the cases (`stdout_delta=180`, both exits zero, exact stderr); the restored control is exact. Phase 6 advances to 85 closed / 348 deferred function rows. See `docs/reconstruction/novodex-physics/evidence/phase6-d6-row004204-descriptor-load.md`.


**M5.14 — Shared joint global-anchor setter (`phys_fn_004099`):** Added a public D6 `setGlobalAnchor`/`getGlobalAnchor` readback to the registered staged-pair fixture. Adding `1.0f` to the localized X anchor is caught with `stdout_delta=4`; the restored differential is exact. Updated the pinned matrix proof to 2,874 oracle-output lines. Phase 6 advances to 86 closed / 347 deferred. Phase 6 and Phase 7 coverage floors advance to 1,239 and 1,430. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004099-global-anchor.md`.


**M5.15 — Shared joint global-axis setter (`phys_fn_004101`):** The existing rotated public joint matrix reaches this shared transform during joint construction and D6 `setGlobalAxis` readback. Adding `1.0f` to the normalized X axis before tangent construction is caught with `stdout_delta=6`; the restored differential is exact. Phase 6 advances to 87 closed / 346 deferred. No additional coverage line was needed. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004101-global-axis.md`.


**M5.16 — Joint actor attachment (`phys_fn_004107`):** The ten-family public joint matrix reaches the shared actor-body binding path. An immediate-return mutation is caught as a candidate access violation (`candidate_exit=-1073741819`, `stdout_delta=3160`); the restored differential is exact. Phase 6 advances to 88 closed / 345 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004107-actor-attachment.md`.


**M5.17 — Joint limit-point setter (`phys_fn_004109`):** The public D6 fixture observes the stored point returned by the setter even when its feature-availability result is false. A +1.0 local-X mutation is caught with `stdout_delta=2`; the restored differential is exact. Phase 6 advances to 89 closed / 344 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004109-limit-point.md`.


**M5.18 — Joint break-event slot (`phys_fn_004111`):** The registered joint-slot fixture invokes the break path on a live support record and observes state/flag changes. An immediate-return mutation is caught with `stdout_delta=4`; the restored differential is exact. Phase 6 advances to 90 closed / 343 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004111-break-event.md`.


**M5.19 — Shared joint base descriptor loader (`phys_fn_004121`):** An immediate-return mutation of the base loader changes descriptor-derived state across the ten-family public joint matrix and is caught with `stdout_delta=2806`; the restored differential is exact. Phase 6 advances to 91 closed / 342 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004121-load-desc.md`.


**M5.20 — Shared joint global-anchor getter (`phys_fn_004125`):** Adding 1.0 to the computed X result is detected across the ten-family public joint matrix (`stdout_delta=246`); the restored staged differential is exact. Phase 6 advances to 92 closed / 341 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004125-global-anchor-getter.md`.


**M5.21 — Joint limit-plane distance helper (`phys_fn_004131`):** A zero-return mutation changes the serialized limit-plane transcript and is caught with `stdout_delta=4448`; the restored staged differential is exact. Phase 6 advances to 93 closed / 340 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004131-limit-plane-distance.md`.


**M5.22 — Shared joint global-axis getter (`phys_fn_004129`):** Adding 1.0 to the computed X result changes the ten-family joint transcript (`stdout_delta=246`); the restored staged differential is exact. Phase 6 advances to 94 closed / 339 deferred. See `docs/reconstruction/novodex-physics/evidence/phase6-joint-row004129-global-axis-getter.md`.

**M5.23 — ActorPairEffector deleting destructor (`phys_fn_003932`):** Added direct oracle/candidate scratch-object probes for flags 0 and 1, covering observer removal, body-pointer clearing, base-vtable restoration, stack balance, return identity, and conditional freeing. Removing the second pointer clear is caught (`stdout_delta=2`); the restored differential is exact. Phase 6 advances to 95 closed / 338 deferred, and its full gate passes. See `docs/reconstruction/novodex-physics/evidence/phase6-effector-row003932-deleting-destructor.md`.


**Phase 7 Scene effector-count getter, 2026-10-10:** Closed `phys_fn_000561` (`NxSceneInternal::getNbEffectors`) with the registered Phase 7 `NxPhysicsEffectorTests` differential. Reading the adjacent joint count at `+0x6c8` instead of the effector count at `+0x6c4` changes lifecycle counts to zero and is rejected with `stdout_delta=14`, both exits zero, and exact stderr. The source was restored byte-for-byte; the restored control is exact. Phase 7 now records 57 closed / 504 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-count-000561.md`. Full-DLL reconstruction remains open.


**Phase 7 Scene effector-iterator reset, 2026-10-10:** Closed `phys_fn_000565` (`NxSceneInternal::resetEffectorIterator`) with the registered Phase 7 `NxPhysicsEffectorTests` differential. Clearing the cursor instead of copying the list head at `+0x5a4` changes the lifecycle iterator labels to `none` and is rejected with `stdout_delta=14`, both exits zero, and exact stderr. The source was restored byte-for-byte; the restored control is exact. Phase 7 now records 58 closed / 503 remaining rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-iterator-000565.md`. Full-DLL reconstruction remains open.


**Phase 7 Scene get-next-effector, 2026-10-10:** Closed `phys_fn_000569` (`NxSceneInternal::getNextEffector`) with the registered Phase 7 `NxPhysicsEffectorTests` differential. Returning null for a non-null cursor changes the lifecycle iterator labels to `none` and is rejected with `stdout_delta=14`, both exits zero, and exact stderr. The source was restored byte-for-byte; the restored control is exact. Phase 7 now records 59 closed / 502 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; focused completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-effector-next-000569.md`. Full-DLL reconstruction remains open.


**Phase 7 Scene remove-effector, 2026-10-10:** Closed `phys_fn_000573` (`NxSceneInternal::removeEffector`) with the registered Phase 7 `NxPhysicsEffectorTests` differential. A no-op leaves the released effector linked from the Scene head and crashes the candidate (`candidate_exit=-1073741819`, `stdout_delta=48`, exact stderr) while the oracle exits normally. Restoring the unlink path returns an exact differential. Phase 7 now records 60 closed / 501 remaining rows. The fresh full Phase 7 gate passes at 1,491/1,457 assertions; inventory passes with 6,338 functions, 5,138 data objects, and zero unexplained entries; completion-report, work-unit, and inventory tests pass 11/11, 12/12, and 251/251. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-remove-effector-000573.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene release-effectors, 2026-10-10:** Closed `phys_fn_000575` (`NxSceneInternal::releaseEffectors`) with the existing `NxPhysicsEffectorTests` scene teardown. An immediate-return mutant skips every effector destructor; the allocator transcript records four fewer frees and the differential rejects it with `stdout_delta=2` (both processes exit zero, exact stderr). Restoring the destructor walk returns an exact differential. Phase 7 now records 61 closed / 500 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries; focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-effectors-000575.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene joint-break event processing, 2026-10-10:** Closed `phys_fn_000577` (`NxSceneInternal::processJointBreakEvents`) with the registered `NxPhysicsSimulationTests` callback fixture. An immediate-return mutant skips both retain and release notifications and fails the fixture's callback-count assertion (`candidate_exit=1`, `stdout_delta=801`); restoring the event walk returns an exact differential. Phase 7 now records 62 closed / 499 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries; focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-joint-break-events-000577.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene debug-renderable getter, 2026-10-10:** Closed `phys_fn_000579` (`NxSceneInternal::getDebugRenderable`) with the registered `NxPhysicsSceneVisualizeTests` fixture. A null-return mutant crashes while visualization consumes the Scene's lazy renderable (`candidate_exit=-1073741819`, `stdout_delta=869`, exact stderr); the restored control matches exactly. Phase 7 now records 63 closed / 498 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries; focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-debug-renderable-000579.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene release-effector, 2026-10-10:** Closed `phys_fn_000594` (`NxSceneInternal::releaseEffector`) with the registered `NxPhysicsEffectorTests` lifecycle fixture. An immediate-return mutant leaves the candidate effector count at 1, iterator at `np`, and performs no frees; the oracle reaches zero/`none` and frees the object. The differential rejects the mutant with `stdout_delta=48` (both exits zero, exact stderr); restoring the release sequence returns an exact control. Phase 7 now records 64 closed / 497 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries; focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-effector-000594.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene create-effector, 2026-10-10:** Closed `phys_fn_000587` (`NxSceneInternal::createSpringAndDamperEffector`) with the registered `NxPhysicsEffectorTests` creation and lifecycle fixture. An immediate null-return mutant changes `created=yes` to `created=no` and is rejected with `stdout_delta=85` (both exits zero, exact stderr); the restored control matches exactly. Phase 7 now records 65 closed / 496 remaining rows. The fresh Phase 7 gate passes at 1,491/1,457 assertions; inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries; focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-create-effector-000587.md`. Full-DLL reconstruction remains open.

**Phase 7 Scene collision-shape pruner visualization, 2026-10-10:** Reconstructed the static and selected dynamic pruner walks for `phys_fn_000581` and `phys_fn_000583`, dispatching each `Prunable` owner through shape virtual slot 3. The `NxPhysicsSceneVisualizeTests` collision-shapes stage records static and dynamic box output. Mutating each loop independently is caught (`stdout_delta=1382` for static, `1322` for selected dynamic; both exits zero, exact stderr); the restored differential is exact. Phase 7 now records 67 closed / 494 remaining rows. The full Phase 7 gate passes at 1,496/1,467 coverage assertions, inventory reports 6,338 functions, 5,138 data objects, and zero unexplained entries, and focused completion-report, work-unit, and inventory tests pass 274/274. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-collision-shape-pruners-000581-000583.md`. Full-DLL reconstruction remains open; separate sphere/capsule visualization rounding remains unclosed.


**Phase 7 joint iterator advancement, 2026-10-10:** Closed `phys_fn_000567` (`NxSceneInternal::getNextJoint`). Clearing the cursor rather than advancing through `Joint + 0x10` is caught by `NxPhysicsJointStagedPairTests` (`stdout_delta=90`, both exits zero, exact stderr); the restored differential is exact. Phase 7 now records 82 closed / 479 remaining rows. The full gate passes at 1,639/1,475 coverage assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-get-next-joint-000567.md`.


**Phase 7 joint-break event producer, 2026-10-10:** Closed `phys_fn_000571` (`NxSceneInternal::addJointBreakEvent`). Clearing the Scene event-list head is caught by the broken-joint callback assertion in `NxPhysicsSimulationTests` (`candidate_exit=1`, `stdout_delta=801`); the restored full Phase 7 gate passes at 1,639/1,475 coverage assertions. Phase 7 now records 82 closed / 479 remaining rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-add-joint-break-event-000571.md`.


**Phase 6 joint-support solver wrapper, 2026-10-10:** Recovered the existing `nxSolveJointSupportRecords` implementation as `phys_fn_004174`. A wrapper no-op and a targeted omission of iterative solver passes are both caught by `NxPhysicsSimulationTests` (`stdout_delta=835`, candidate exit 1); the restored Phase 6 gate passes at 1,597/1,267 coverage assertions. Phase 6 now records 152 closed / 281 deferred rows. The adjacent custom-row callback row remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-joint-support-solver-wrapper-004174.md`.


**Phase 7 Scene result-status mask, 2026-10-11:** Added a public staged-pair assertion for NxScene::checkResults(static_cast<NxSimulationStatus>(0), false) under phys_fn_000396. The shipped DLL returns true when the rigid-body-finished bit is absent; changing that row branch to false is rejected by NxPhysicsSimulationTests (oracle exit 0, candidate exit 1 at the new assertion, stdout_delta=3846). The restored Win32 candidate passes the full simulation target exactly (stdout_delta=0, exact stderr). Phase 7 advances to 84 closed / 477 remaining; Phase 8 remains open. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-checkresults-status-mask-000396.md.


**Phase 7 Scene isWritable success path, 2026-10-11:** The registered public simulation differential observes phys_fn_000392 through simulation writable=initial. Changing the successful return after acquiring and releasing the write lock is rejected (both exits zero, exact stderr, stdout_delta=2); the restored control is exact. Phase 7 advances to 85 closed / 476 remaining. The failure-on-contention arm remains untested. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-iswritable-000392.md.


**Phase 7 Scene fluid release, 2026-10-11:** Closed `phys_fn_000622` (`NxSceneInternal::releaseFluid`) using the registered simulation differential. An immediate-return mutant leaves the disabled manager installed and skips seeded fluid-array swap-removal/deleting dispatch; both processes exit zero with exact stderr and `stdout_delta=4`. Restoring the implementation returns an exact transcript. The row proof covers tested disabled-backend paths; callback reentry and extension-backed live-fluid notification remain open. Phase 7 advances to 86 closed / 475 remaining. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-release-fluid-000622.md`.


**Phase 7 FluidManager surface-mesh traversal, 2026-10-11:** Reconstructed and mutation-falsified `phys_fn_003632` / `phys_fn_003634` through `NxPhysicsFluidEmitterAbiTests`. A two-fluid fixture observes matching `ImplicitFluidMeshRun` callback tuples; an increment-only loop mutant is caught (`oracle_callbacks=2`, `candidate_callbacks=0`, `mismatches=3`). The restored full Phase 7 gate passes at 1,659/1,659 coverage assertions. Phase 7 now records 97 closed / 464 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-manager-surface-mesh-003632-003634.md`. Full-DLL reconstruction remains open.

**Phase 7 fluid substep nonempty-emitter path, 2026-10-11:** Added a direct oracle/candidate `phys_fn_003495` differential with a one-element bodyless emitter range. It checks the `003624` pose callback, FluidRun arguments and vector/timestep payload, timestep/state writes, bodyless `003601` early return, result, and x86 stack. Removing the pre-FluidRun emitter update call is rejected (`pose_callback=0`, `mismatches=1`); restored output is exact. Fresh Phase 5/7 gates pass at 2,638/2,638 and 1,665/1,665; all 810 tooling tests pass. Candidate SHA-256 `8180eeee170d23d07139445d6e91edb888a454524cdf61d3d5cc6a9d643445b9`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-nonempty-emitter-substep-003495.md`. The dirty static-collision builder (`003505`) and full manager dirty-flag path (`003630`) remain open; no complete-row closure is claimed.

**Phase 7 fluid collision bucket-prefix leaf, 2026-10-11:** Reconstructed `phys_fn_003514`, a leaf prerequisite of dirty static-collision rebuild `003505`, with direct four-record and empty-range oracle differentials. Adding one to every row contribution is rejected (`oracle_total=9`, `candidate_total=13`, `mismatches=1`); restored prefixes, total, return, preserved empty-range records, and x86 stack match. Fresh Phase 5/7 gates pass at 2,640/2,640 and 1,667/1,667 assertions; all 810 tooling tests pass; inventory and both 80-file public-header checks pass. Candidate SHA-256 `a794af9c11567222f7baf9a1a544463990eb481cc092273eeca1294183437e0c`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-bucket-prefix-003514.md`. Dirty builder `003505` and manager path `003630` remain open.

**Phase 7 fluid collision cell-prefix leaf, 2026-10-11:** Reconstructed `phys_fn_003516`, which assigns flattened index prefixes to occupied cell records and clears their pending counts. Three bucket rows and three cells match the oracle (prefixes `0,3,4`, total `6`, return `3`, balanced stack); a per-cell `+1` prefix mutation is rejected (`prefixes=0`, `mismatches=1`), and the empty bucket range preserves all state. Fresh Phase 5/7 gates pass at 2,642/2,642 and 1,669/1,669; all 810 tooling tests pass; inventory and both 80-file public-header manifests pass. Candidate SHA-256 `0021acdd295e407fa51ec11c7ac64a3c70fbeddb4c83a2b2f9519b05e55f4c1b`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. This closes only the helper; builder `003505` and manager `003630` remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-collision-cell-prefix-003516.md`.

**Phase 7 fluid collision plane-array appender, 2026-10-11:** Reconstructed `phys_fn_003551` (`0x87940`) as the type-0, 96-byte typed-array append path, including its shape vtable slot-13 thiscall and defined four-word record prefix. The direct pinned-oracle fixture passes with `oracle_contract=1`, balanced stack, and zero mismatches; a one-word scratch-window mutation is rejected (`mismatches=1`). Fresh Phase 5/7 gates pass at 2,657/2,657 and 1,684/1,684; all 810 tooling tests pass; inventory validation and the 80-file public Physics header manifest pass. The Phase 7 ledger now records 119 closed / 442 remaining rows. The intervening `phys_fn_003552` row is compiler padding. Next backlog slice: reconstruct mesh appender `phys_fn_003541` at RVA `0x876d0`, a prerequisite to shape-to-array router `phys_fn_003553`.

**Phase 7 fluid collision mesh-array appender, 2026-10-12:** Reconstructed `phys_fn_003541` (`0x876d0`) with its `-8` mesh-data sentinel, geometry word order `+12,+20,+8,+16`, and shared shape packer. The direct pinned-oracle fixture passes with `oracle_contract=1`, balanced stack, and zero mismatches; mutating the first source word from `+12` to `+8` is rejected (`mismatches=1`). The Phase 7 ledger now records 120 closed / 441 remaining rows. Fresh Phase 5/7 gates pass at 2,658/2,658 and 1,685/1,685; all 810 tooling tests pass; inventory validation and the 80-file public Physics header manifest pass. Next backlog slice: shape-to-array router `phys_fn_003553` at RVA `0x87a40`.

**Phase 7 fluid collision shape-array router, 2026-10-12:** Reconstructed `phys_fn_003553` (`0x87a40`) with its excluded/owner-managed early returns and typed descriptor routing. The 13-case direct oracle fixture passes with `oracle_contract=1`, balanced stack, and zero mismatches; changing the ordinary box descriptor offset from `+0x3c` to `+0x30` is rejected (`mismatches=1`). The Phase 7 ledger now records 121 closed / 440 remaining rows. Next backlog slice: scene shape collector `phys_fn_003555`.

**Mainline verification refresh, 2026-10-12:** The shape-array router is committed on `main`. Fresh Phase 5 and Phase 7 gates pass at 2,659/2,659 and 1,686/1,686 assertions; all 810 reconstruction-tooling tests pass; inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained entries; and the 80-file public Physics header manifest passes. The five-family collision-object destructor gate remains green, with all five scratch/heap secondary-vtable calls and all five primary deleting-destructor calls matching the oracle and the member-only negative control reporting five mismatches. Candidate SHA-256: `20cf41661aa185fbe24ed8478d260a00921b75a85b8daea5026d90a14cdfa2cc`. Next backlog slice: scene shape collector `phys_fn_003555`.
# Continuation — bodyless joint-list insertion (2026-10-12)

Closed Phase 7 row `phys_fn_000557` (`NxSceneInternal::pushJointWithoutBodies`)
with a registered standalone staged-pair fixture and a no-op mutation that the
oracle differential rejects. The full Phase 7 gate passes at 1,710/1,710
assertions. This is a single row closure; Phase 7 and full-DLL reconstruction
remain open. See `docs/reconstruction/novodex-physics/evidence/phase7-joint-bodyless-list-000557.md`.

**Phase 7 indirect callback destroy thunk, 2026-10-12:** Closed `phys_fn_003509` (`0x863f0`) by making the `NxPhysicsObjectLayoutTests` marker data-driven. A no-forward mutation returns zero without invoking the callback, changes the row marker to `candidate=0 arg=mismatch return=0`, and is rejected by the registered Phase 7 coverage check (zero required-marker occurrences). Restored output matches `oracle=1 candidate=1 arg=match return=3509`. Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,678/2,678, 1,607/1,607, and 1,710/1,710. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-indirect-tail-003509.md`.

### Continuation — public `NxFluid::setStiffness` (`phys_fn_003718`, 2026-10-13)

The primary NxFluid table now dispatches `setStiffness` through the FluidModel callback under the Scene write lock and shared reentry guard. The licensed fixture checks the exact scalar and manager/fluid callback arguments. Suppressing the callback in a temporary build is rejected by the staged-pair differential (`candidate_exit=1`, `stdout_delta=52`); restored control matches exactly. Fresh Phase 5, 6, and 7 gates pass at 2,686, 1,620, and 1,728 assertions. Phase 7 records 226 closed / 334 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-set-stiffness-003718.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid` viscosity accessors (`phys_fn_003720`, `003722`, 2026-10-13)

`NxFluid::getViscosity` row 003720 now has a registered differential closure: suppressing its unavailable-feature diagnostic changes the transcript by two bytes, and the restored run is exact. Row 003722 now routes `setViscosity` through `FluidSetViscosity` under the Scene write lock and shared reentry guard. The licensed public fixture checks scalar bits and callback arguments; a no-forward mutation is rejected (`candidate_exit=1`, `stdout_delta=52`). Fresh Phase 5/6/7 gates pass at 2,687/1,621/1,729 assertions. Phase 7 records 228 closed / 332 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-set-viscosity-003720-003722.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid::setDamping` (`phys_fn_003726`, 2026-10-13)

The setter now acquires the Scene write lock, applies the shared reentry guard, and forwards the backend manager, fluid, and value to `FluidSetDamping`. A licensed public fixture checks the exact scalar bits and callback arguments; a no-forward mutation is rejected (`candidate_exit=1`, `stdout_delta=52`) and the restored differential is exact. Fresh Phase 5/6/7 gates pass at 2,688/1,622/1,730 assertions. Phase 7 records 229 closed / 331 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-set-damping-003726.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid::setStaticCollisionRestitution` (`phys_fn_003730`, 2026-10-13)

The setter now takes the Scene write lock, applies the shared reentry guard, and forwards manager, fluid, and scalar to `FluidSetStaticCollisionRestitution`. The public fixture checks exact value bits, callback arguments, and zero diagnostics. A no-forward mutation is rejected (`candidate_exit=1`, `stdout_delta=52`), and the restored differential is exact. Fresh Phase 5/6/7 gates pass at 2,689/1,623/1,731 assertions; Phase 7 records 230 closed / 330 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-static-rest-restitution-003730.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid::setStaticCollisionAdhesion` (`phys_fn_003734`, 2026-10-13)

The setter now acquires the Scene write lock, checks the shared callback reentry guard, and forwards manager, fluid, and scalar through `FluidSetStaticCollisionAdhesion`. The public fixture checks the scalar and callback arguments with zero diagnostics. A no-forward mutant is rejected (`candidate_exit=1`, `stdout_delta=52`), and the restored differential is exact. Fresh Phase 5/6/7 gates pass at 2,690/1,624/1,732 assertions; Phase 7 records 231 closed / 329 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-static-adhesion-003734.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid` dynamic-collision setters (`phys_fn_003738`, `003742`, 2026-10-13)

The dynamic-collision restitution and adhesion setters preserve the oracle's exact unavailable-feature warnings while forwarding their values through the matching FluidModel callbacks. The public fixture checks warning metadata, scalar bits, call count, and manager/fluid arguments. Separate no-forward mutants are rejected (`candidate_exit=1`, `stdout_delta=52`), and restored output matches exactly. Phase 5/6/7 pass at 2,692/1,626/1,734 assertions, and Phase 7 records 233 closed / 327 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-dynamic-setters-003738-003742.md`. Full-DLL reconstruction remains open.

### Continuation — public `NxFluid` surface-mesh accessors (`phys_fn_003706`, `003708`, 2026-10-13)

The public getter and release slots now match the oracle's license gates, Scene locks, pointer layout, reentry guard, deleting dispatch, and slot clearing. The registered simulation fixture checks both product-code-465 unavailable-feature warnings and the product-code-804 licensed pointer/destructor path. Independent no-op getter and release mutants are each rejected (`stdout_delta=60`); the restored staged differential is exact. Phase 5/6/7 gates pass at 2,695/1,629/1,737, and Phase 7 records 237 closed / 323 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-surface-mesh-003706-003708.md`.

### Continuation — public `NxFluid::setName` and `getName` (`phys_fn_003786`, 2026-10-13)

The public solver vtable now stores and retrieves caller-owned fluid name pointers through the shared SDK pointer-binding registry under the wrapper write/read locks. The registered simulation fixture verifies set, get, and null-clear; a no-op setter mutant is rejected (`candidate_exit=1`, `stdout_delta=60`), while the restored staged differential is exact. Phase 5/6/7 gates pass at 2,694/1,628/1,736; Phase 7 now records 235 closed / 325 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-name-003786.md`.

### Continuation — public `NxFluid::getWorldBounds` (`phys_fn_003784`, 2026-10-13)

The public vtable now routes `getWorldBounds` through the Scene read lock and copies six words from the internal fluid bounds at `+0x5c`. The fixture seeds six distinct values and asserts the public output. A wrong-offset mutation is rejected (`candidate_exit=1`, `stdout_delta=60`), and the restored differential is exact. Phase 5/6/7 pass at 2,693/1,627/1,735 assertions; Phase 7 records 234 closed / 326 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-world-bounds-003784.md`. Full-DLL reconstruction remains open.


### Continuation — public `NxFluid::setFlag` and `getFlag` (`phys_fn_003744`, `003746`, 2026-10-13)

The public solver vtable now toggles and reads the requested bits at internal-fluid+0x58 under the Scene write/read locks, with the setter honoring the shared callback reentry guard. The registered simulation fixture verifies initial, cleared, and set states while restoring its seeded private word. Separate no-op setter and zero-return getter mutants are rejected (`candidate_exit=1`, `stdout_delta=61` each); the restored differential is exact. Current closure accounting is Phase 7 239 closed / 321 deferred, with Phase 5/6/7 coverage floors advanced to 2,696/1,630/1,738. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-flags-003744-003746.md`. Full-DLL reconstruction remains open.


### Continuation — public `NxFluid::setParticlesWriteData` and `getParticlesWriteData` (`phys_fn_003712`, `003714`, 2026-10-13)

The solver vtable now reads and writes the eleven-dword `NxParticleData` record at internal-fluid+0x28, using the Scene read/write locks and the setter's shared reentry guard. The registered simulation fixture verifies seeded readback, setter storage, aggregate return ABI, and round-trip equality, then restores its private state. Independent no-op setter and zero-return getter mutations are rejected (`candidate_exit=1`, `stdout_delta=61` each); restored output is exact. Phase 7 now records 241 closed / 319 deferred, and Phase 5/6/7 floors advance to 2,697/1,631/1,739. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npfluid-particle-write-data-003712-003714.md`. Full-DLL reconstruction remains open.

### Continuation — friction-patch assignment (`phys_fn_000877`, 2026-10-13)

Added a registered direct ABI differential for `NxFrictionPatch::operator=` using three patterned scratch pairs. The no-op mutation is rejected (`mismatches=3`); restored control matches the oracle payload and preserves the vptr, tail padding, return pointer, and x86 stack. Updated the oracle-only fluid transcript pin for the new oracle-return line. Fresh Phase 5 and Phase 7 gates pass at 2,699/2,699 and 1,741/1,741. Phase 7 now records 245 closed / 315 deferred; the completion census still has 2,786 code rows open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-friction-patch-assignment-000877.md`.

### Continuation — contact friction solver (`phys_fn_000879`, 2026-10-13)

The existing public sliding-sphere simulation now explicitly pins its final 60-step friction state in the Phase 5/6/7 gate. Replacing `NxActorPair::row000879` with an immediate return changes the staged transcript by 132 lines and leaves the sphere at its initial horizontal speed; restored code exactly matches the oracle's final position and velocity. Phase 7 records 246 closed / 314 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-friction-row-000879.md`.
### Continuation — contact-pair constructor (`phys_fn_000893`, 2026-10-13)

A no-op mutation of `NxActorPair::row000893` makes the registered contact/friction simulation candidate access-violate (`candidate_exit=-1073741819`, `stdout_delta=3983`); the restored control matches exactly (`stdout_delta=0`, exact stderr). Phase 7 now records 251 closed / 309 deferred rows and passes at 1,744/1,744 assertions. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows; all 313 focused inventory/completion/work-unit/gate-target tests pass, and both 80-file public-header manifests pass. The unresolved `phys_fn_002356` helper and remaining contact-manager paths stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-constructor-000893.md`. Full-DLL reconstruction remains open.
### Continuation — contact-pair patch lookup (`phys_fn_000895`, 2026-10-13)

An immediate-null mutation of `NxActorPair::row000895` makes the registered sliding-contact candidate access-violate (`candidate_exit=-1073741819`, `stdout_delta=3887`); the restored differential matches exactly (`stdout_delta=0`, exact stderr). Phase 7 now records 252 closed / 308 deferred rows. The post-merge mainline Phase 7 gate passes at 1,744/1,744, both 80-file public-header manifests pass, inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained, and all 313 focused inventory/completion/work-unit/gate-target tests pass. The fixture proves the first embedded-patch path; multi-patch selection, allocation failure, and vector growth remain open for the full-image audit. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-patch-lookup-000895.md`. Full-DLL reconstruction remains open.
### Continuation — contact-pair constraint generator (`phys_fn_000897`, `phys_fn_000899`, 2026-10-13)

The two oracle ranges map to the single candidate body `NxActorPair::row000897`. Replacing that body with an immediate return changes the registered contact/friction transcript by 6,644 bytes (both exits zero, exact stderr); the restored control is exact. Phase 7 now records 254 closed / 306 deferred rows. The post-merge mainline Phase 7 gate passes at 1,744/1,744, both public-header manifests pass, inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained, and all 313 focused inventory/completion/work-unit/gate-target tests pass. The registered scenes cover ordinary, sliding, spring, sphere-pair, and mesh contact; rare stream flags and full precision/branch audit remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-contact-pair-constraints-000897-000899.md`. Full-DLL reconstruction remains open.
### Continuation — broadphase pair-node constructor (`phys_fn_000901`, 2026-10-13)

A return-this mutation of `NxPairNode::row000901` omits `NxActorPair` initialization and list linking and makes the registered candidate access-violate (`candidate_exit=-1073741819`, `stdout_delta=3984`); the restored control matches exactly. Phase 7 now records 255 closed / 305 deferred rows. The post-merge mainline Phase 7 gate passes at 1,744/1,744, both public-header manifests pass, inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained, and all 313 focused inventory/completion/work-unit/gate-target tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-pair-node-constructor-000901.md`. Full-DLL reconstruction remains open.
### Continuation — pair-node per-step refresh (`phys_fn_000905`, 2026-10-13)

A no-op mutation of `NxPairNode::row000905` suppresses contact-report refresh and fails the registered fixture (`candidate_exit=1`, `stdout_delta=3985`); restoring it returns an exact differential. Phase 7 now records 256 closed / 304 deferred rows. The post-merge mainline Phase 7 gate passes at 1,744/1,744, both public-header manifests pass, inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained, and all 313 focused inventory/completion/work-unit/gate-target tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-pair-node-refresh-000905.md`. Full-DLL reconstruction remains open.
### Continuation — pair-list refresh/insertion (`phys_fn_000909`, `phys_fn_000911`, 2026-10-13)

Independent mutations of `NxPairList::row000909` (no-op) and `row000911` (null return) each fail the registered contact simulation (`candidate_exit=1`, `stdout_delta=3985`); both restored controls are exact. Phase 7 now records 258 closed / 302 deferred rows. The post-merge mainline Phase 7 gate passes at 1,744/1,744, both public-header manifests pass, inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained, and all 313 focused inventory/completion/work-unit/gate-target tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-pair-list-refresh-insert-000909-000911.md`. Full-DLL reconstruction remains open.

### Continuation — fluid-manager parallel-array growth failures (`phys_fn_003649`, 2026-10-13)

Added paired allocator-failure probes for the first and second 24-byte array-growth allocations during enabled fluid creation. The first failure faults before either array publishes the new fluid (counts 2/2); the second publishes the internal fluid but faults before its public wrapper (counts 3/2). A pre-fix differential caught the candidate's incorrect 2/3 state on first-allocation failure; removing its allocation-null early return restores oracle ordering. Both staged differentials now match exactly. Phase 7 passes at 1,772/1,772 and all 39 gate-registry unit tests pass; public header hashes remain unchanged. Later constructor allocations and partial-construction teardown remain open, so `phys_fn_003649` and the full DLL reconstruction stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-manager-growth-failure-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-manager-growth-003649-20261013/`.

### Continuation — initial-particle array allocation failure (`phys_fn_003503`, 2026-10-13)

Added an enabled-fluid callback fixture that makes `ParticleArrayAlloc` fail for a valid two-particle descriptor. IDA shows the oracle immediately reads the returned array while writing the first particle record; its access violation occurs before either manager list is appended. The candidate's prior null guard instead returned a fluid with both lists populated, so the staged red differential captured the mismatch. Removing that guard reproduces the oracle fault; the caught oracle/candidate marker now matches exactly (`c0000005`, `internal=0`, `public=0`). The normal strided position, velocity, and lifetime fixture still passes. Phase 7 passes at 1,773/1,773; all 39 gate-target tests pass. ParticleArray growth failures and broader callback ordering remain open, so `phys_fn_003503` and the full DLL reconstruction stay open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-internal-create-003497-003503.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-particle-array-003503-20261013/`.


**M1 continuation, 2026-10-14:** Added the retained oracle-only baseline for `NxPhysicsThirdPartyTests`. Its proof binds the pinned oracle DLL and normalized fixture source, fixes the xorshift32 differential generator mode, 27 coverage lines, 116 driven cases, and final oracle digest `e2a266c1`; candidate-only fields do not enter the digest. A fresh Phase 4 gate passes all 519/519 coverage assertions and invokes the proof verifier. The tooling suite passes all 815 tests. The remaining target-level proof gap is `NxPhysicsCollisionTests`; `NxPhysicsAssetTests` already has a retained proof. The completion backlog remains at 2,786 open executable rows; this harness milestone does not close DLL reconstruction. Raw differential and gate transcripts are compressed under `docs/reconstruction/novodex-physics/evidence/raw/phase4-thirdparty-oracle-proof-20261014/`.


**M1 continuation, 2026-10-14 — Collision oracle proof:** Added an oracle-only summary proof for `NxPhysicsCollisionTests`. It binds the pinned oracle DLL and normalized harness source, validates the fixed generator seeds, all 108 input-digest rows, all 125 case-coverage labels, and 136 oracle result rows. Candidate digests, mismatch counters, and other candidate-only fields are excluded from the retained summary hash. The Phase 3 gate verifies the proof and passes all 561/561 assertions. The proof parser has mutation tests for candidate-only changes, oracle output changes, and missing coverage. The full reconstruction-tooling suite passes all 816 tests. Archived oracle and gate transcripts are under `docs/reconstruction/novodex-physics/evidence/raw/phase3-collision-oracle-proof-20261014/`. All three M1 oracle-only baseline targets now have machine-readable proofs; M1 remains open for lifecycle coverage and other harness gaps. The 2,786 executable DLL rows remain open.

### Continuation — Scene raycast empty-collector guard (`phys_fn_000700`, `2026-10-14`)

The `nxRaycastClosestShapeLoop` count guard was falsified through `NxPhysicsSceneRaycastTests`: reversing the empty-collector condition in a CMake-rebuilt candidate produces an access violation and `stdout_delta=2707`; the restored candidate matches the pinned oracle exactly. The fresh Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 276 functions and defers 284. The loop body `phys_fn_000702` remains separately tracked. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-raycast-000700-empty-collector.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Scene raycast closest-hit body (`phys_fn_000702`, `2026-10-14`)

The closest-hit condition in `nxRaycastClosestShapeLoop` was mutation-falsified through `NxPhysicsSceneRaycastTests`: reversing the comparison produces `stdout_delta=288` (both exits zero, exact stderr), and the restored candidate matches the oracle exactly. The fresh Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 277 functions and defers 283. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-raycast-000702-closest-hit.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Scene any-shape raycast wrapper (`phys_fn_000704`, `2026-10-14`)

Suppressing the `NxSceneInternal::raycastAnyShape` loop forward is rejected through `NxPhysicsSceneRaycastTests` (`stdout_delta=288`, both exits zero, exact stderr); the restored control is exact. Fresh Phase 7 passes at 1,779/1,779 coverage assertions. Phase 7 closes 278 functions and defers 282. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-raycast-000704-any-shape.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Scene closest-shape distance finalization (`phys_fn_000706`, `2026-10-14`)

Replacing the closest-shape wrapper’s final square-root distance store with zero is rejected by `NxPhysicsSceneRaycastTests` (`stdout_delta=288`, both exits zero, exact stderr); the restored differential is exact. The fresh full Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 279 functions and defers 281. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-raycast-000706-distance-finalization.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Island support-body copy-back (`phys_fn_000708`, `2026-10-14`)

Returning before `Row000708Fixture::row000708` copies the completed support vectors and saved velocities back to the island body record is rejected by `NxPhysicsSimulationTests` (`stdout_delta=6986`, both exits zero, exact stderr); restored output is exact. Fresh Phase 7 passes at 1,779/1,779 coverage assertions. Phase 7 closes 280 functions and defers 280. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-support-copy-back-000708.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Body-step gravity initialization (`phys_fn_000710`, `2026-10-14`)

Returning before the body-step helper stores gravity and clears auxiliary acceleration words is rejected by `NxPhysicsSimulationTests` (`stdout_delta=6870`, candidate exit 1); the restored candidate is exact. Fresh Phase 7 passes at 1,779/1,779 coverage assertions. Phase 7 closes 281 functions and defers 279. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-body-gravity-initialization-000710.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Phase 2 shared island root lookup (`phys_fn_000712`, `2026-10-14`)

The Phase 2-owned root/path-compression helper is discharged by Phase 5. Returning null instead of the compressed root is rejected by `NxPhysicsSimulationTests` (`stdout_delta=771`, candidate access violation); the restored differential is exact. The full Phase 5 gate passes at 2,708/2,708 coverage assertions. Phase 2 closes 64 functions and defers 79; Phase 7 counts are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-island-root-return-000712.md` and archived transcripts.

### Continuation — Island sleep/wake update (`phys_fn_000714`, `2026-10-14`)

Returning before the sleep-group version, velocity thresholds, sleep timer, and awake-flag update is rejected by `NxPhysicsSimulationTests` (`stdout_delta=5918`, both exits zero, exact stderr); the restored differential is exact. The fresh full Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 282 functions and defers 278. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-sleep-wake-update-000714.md` and archived transcripts. Full DLL reconstruction remains active.

### Continuation — Island-root union (`phys_fn_000716`, `2026-10-14`)

Returning before the island-root and member-list merge is rejected by `NxPhysicsSimulationTests` (`stdout_delta=782`, candidate access violation); restored simulation and joint controls are exact. The fresh full Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 283 functions and defers 277. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-root-union-000716.md` and archived transcripts.

### Continuation — Sleep-group union (`phys_fn_000718`, `2026-10-14`)

Returning before the sleep-group root and member-list merge is rejected by `NxPhysicsSimulationTests` (`stdout_delta=4158`, both exits zero, exact stderr); the restored control is exact. The fresh full Phase 7 gate passes at 1,779/1,779 coverage assertions. Phase 7 closes 284 functions and defers 276. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sleep-group-union-000718.md` and archived transcripts.


### M1 continuation — Scene/actor allocator ownership (`2026-10-14`)

Registered the existing `NxPhysicsSceneAllocatorTests` target in Phases 5 and 7 with 12 coverage markers. Its first staged-pair run found six cross-allocator frees: the Scene condition object, its worker-state block, the Scene auxiliary object, and three public actor wrappers were routed inconsistently between the Foundation and SDK allocators. The oracle’s distinct-allocator transcript established the Foundation allocator for each block. Corrected the four allocation/free routes; restored output now matches exactly (`stdout_delta=0`, `stderr_exact=True`). A targeted actor-wrapper free mutation is rejected (`stdout_delta=20`, `cross_frees=3`). The full Phase 5 and Phase 7 gates pass at 2,720/2,720 and 1,791/1,791 coverage assertions. The full-DLL backlog remains 2,786 open code rows. Evidence and raw transcripts: `docs/reconstruction/novodex-physics/evidence/phase5-scene-allocator-ownership-20261014.md` and `evidence/raw/phase5-scene-allocator-ownership-20261014/`.

### M2 continuation — triangle-mesh release registry miss (`phys_fn_000470`, `2026-10-14`)

`PhysicsSDK::releaseTriangleMesh` now reproduces the pinned `NXE_INVALID_OPERATION` report (`PhysicsSDK.cpp:569`) for an empty mesh registry and for a foreign mesh when a registered mesh exists. A public NxTriangleMeshApi fixture catches a no-op diagnostic mutant (`stdout_delta=27`); the restored staged pair is exact. Phase 4 passes at 529/529 coverage assertions. Phase 2 advances to 66 closed / 77 deferred rows; full-DLL semantic and ABI closure remains open at 2,786 code rows. Public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-000470.md`.

### M2 continuation — public triangle-mesh creation wrapper (phys_fn_000242, 2026-10-14)

The public NpPhysicsSDK::createTriangleMesh wrapper now has a row-specific mutation proof: forcing its successful path to return null makes the API differential fail (stdout_delta=26); the restored candidate matches exactly. The existing simulation fixture still covers Scene-lock contention and reverse unlock. Phase 2 advances to 67 closed / 76 deferred; the public headers and internal mesh-factory boundary are unchanged. Phase 4 remains green at 529/529. Evidence: docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-create-000242.md.

### M2 continuation — triangle-mesh release wrapper lock refusal (phys_fn_000244, 2026-10-14)

The registered Phase 2 lock fixture confirms that a release call returns promptly while another thread holds the Scene write lock, emits the pinned NXE_INVALID_OPERATION diagnostic, and can release successfully after unlock. A no-op diagnostic mutant is caught (stdout_delta=5); restored output is exact. Phase 2 advances to 68 closed / 75 deferred. Public headers and internal mesh release remain unchanged/open, respectively. Evidence: docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-lock-000244.md.

### M2 continuation — triangle-mesh release reverse unlock (`phys_fn_000246`, 2026-10-14)

Expanded the lock fixture to two live Scenes. Contending on Scene 2 forces the release wrapper to acquire Scene 1 and enter the outlined reverse-unlock path. A no-op loop-body mutant leaks Scene 1's lock and is caught by an external probe (`stdout_delta=5`, oracle `unwind_first=1`, candidate `unwind_first=0`); restored oracle/candidate control is exact. The independent second-Scene and full lock/unwind assertions pass under the Phase 2 coverage floor of 8. Phase 2 records 69 closed / 74 deferred; full-DLL backlog remains 2,786 open code rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-triangle-mesh-release-unwind-000246.md` and archived raw transcripts.

### M2 continuation — SDK material lock wrapper (`phys_fn_000254`, `phys_fn_000256`, 2026-10-14)

Reconstructed `NpPhysicsSDK::addMaterial`'s per-Scene write-lock protocol and outlined reverse-unlock continuation from the pinned oracle. With Scene 2 contended, the public API refuses with line 173, returns index 0, preserves material count, and releases Scene 1's acquired lock; unlocked insertion succeeds at index 1. Independent no-forward and reverse-unlock mutants are rejected (`stdout_delta=18` and `19`); restored output is exact. Phase 2/3/6 gates pass at 10/10, 563/563, and 1,642/1,642. Phase 2 records 70 closed / 73 deferred; full DLL remains open at 2,786 code rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-sdk-add-material-lock-000254-000256.md`.


### Continuation — SDK purge and performance-inspector Scene locks (`phys_fn_000263`, `000265`, `000277`, `000279`, 2026-10-14)

Reconstructed the all-Scene write-lock, reverse-unwind, diagnostic and successful-forwarding paths for `purgeMaterials` and `setPerformanceInspector`. Two-live-Scene public fixtures verify refusal at lines 211 and 277, release of Scene A after Scene B contention, unchanged material count on refused purge, and successful unlocked purge/inspector forwarding. Four independent mutations are caught (`stdout_delta=20/21` for purge and `18/19` for inspector); restored output is exact. Fresh Phase 2, 3 and 6 gates pass at 14/14, 567/567 and 1,646/1,646. The 39 gate-target unit tests pass; inventory and all 80 immutable public-header checks pass. Phase 2 now records 72 closed / 71 deferred rows. Evidence and compressed transcripts: `docs/reconstruction/novodex-physics/evidence/phase2-sdk-purge-inspector-lock-000263-000265-000277-000279.md` and its `evidence/raw/` directory. Full-DLL reconstruction remains active.

### M1 continuation — repeated public Scene lifecycle (`2026-10-15`)

Added `NxPhysicsSceneLifecycleTests` to the Win32 Release CMake build and Phase 5. Each of eight cycles creates three Scenes, releases a middle element, appends a Scene, and removes the first and last elements while checking the SDK's replace-with-last array behavior, wrapper identity, and count. Oracle/candidate output matches exactly (`creates=32 releases=32 stable=1 mismatches=0`, `stdout_delta=0`, exact stderr). A candidate-only no-op mutation of `PhysicsSDK::releaseScene` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2`, 47 mismatches). The fresh Phase 5 gate passes at 2,741/2,741 assertions; all 816 reconstruction tooling tests pass. The packet adds lifecycle verification and refreshes stale gate-registry, work-unit, and oracle-proof snapshots; it does not close a production code row. M1 and the 2,786-row DLL backlog remain open. Evidence and compressed raw logs: `docs/reconstruction/novodex-physics/evidence/phase5-scene-lifecycle-cycles-20261015.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase5-scene-lifecycle-cycles-20261015/`.


### Continuation — box-hull AABB center/extents helper (`phys_fn_000923`, 2026-10-14)

The row's implementation is the inline `IceMaths::AABB::SetCenterExtents` in `External/opcode/upstream/Opcode/Ice/IceAABB.h`, called by the reconstructed `BoxShape::nxBoxRebuildHull`. Changing only its minimum calculation from `mMin = c - e` to `mMin = c` makes the registered `NxPhysicsSceneRaycastTests` staged-pair differential fail (`stdout_delta=126`, both exits zero). Restoring the header and rebuilding returns an exact control. This row is discharged by Phase 4, which already owns the driven box-hull caller; Phase 2 now records 73 closed / 70 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-aabb-center-extents-000923.md` and its `evidence/raw/` directory. Full-DLL reconstruction remains open.


### Continuation — SDK core-dump Scene-lock wrapper (`phys_fn_000267`, 2026-10-14)

The existing `NxPhysicsCoreDumpTests` target exercises the wrapper's two-Scene refusal path: Scene B is marked as held by another thread, the oracle reports `NpPhysicsSDK.cpp:225`, creates no dump file, and releases Scene A. Replacing `NpPhysicsSDK::coreDump` with an immediate `false` return changes the staged transcript (`stdout_delta=2266`, both processes exit 0); restoring the function returns an exact control. Registered the target in Phase 3 so this Phase 2 row is discharged by a passed phase rather than a pending Phase 6/7 gate. Phase 3's coverage floor is now 945; Phase 2 records 74 closed / 69 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-core-dump-lock-000267.md` and its `evidence/raw/` directory. Full-DLL reconstruction remains open.


### Continuation — Actor-group pair insertion and lookup (`phys_fn_000433`, `phys_fn_000435`, 2026-10-14)

Suppressing `cpmSetActorGroupPairFlags` is rejected by `NxPhysicsSDKTests` (`stdout_delta=6`); forcing `PhysicsSDK::getActorGroupPairFlags` to return zero is rejected at `stdout_delta=2`. The restored staged differential is exact. Fresh Phase 2 and Phase 3 gates pass at 14/14 and 945/945 coverage assertions; the later Phase 3 gate discharges both Phase 2 rows. Phase 2 now records 76 closed / 67 deferred rows. Public headers and CMake are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-actor-group-pair-core-000433-000435.md` and its raw transcripts. Full-DLL reconstruction remains open.


## M6.19 — Island sleep/wake body-state update (`phys_fn_000732`)

The existing public simulation sleep fixture distinguishes this row: an immediate-return mutant changes automatic sleeping from 1 to 0 and the registered differential rejects it (`candidate_exit=1`, `stdout_delta=6720`). The restored source is byte-identical to the pinned mainline source, the rebuilt DLL hash matches the baseline, and the restored differential is exact. Phase 7 now records 307 closed / 253 deferred rows; this packet does not close caller `000714` or the rest of the simulation pipeline. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-body-sleep-wake-000732.md`.


### M6.20 investigation — Dirty island rebuild dispatch (`phys_fn_000720`)

An immediate-return mutation produced an exact simulation transcript (`stdout_delta=0`), so the existing fixture does not exercise its dirty-island rebuild branch. The source was restored byte-for-byte and no closure was claimed; row 000720 remains open for a fixture that marks the island dirty.

### M6.21 — Island joint/contact solver dispatch (`phys_fn_000728`)

Suppressing the row’s joint and contact-pair solve loops is rejected by `NxPhysicsSimulationTests` (`candidate_exit=1`, `stdout_delta=6874`); restoring the source returns an exact differential. Phase 7 now records 308 closed / 252 deferred rows; row 000720 and the rest of the simulation step remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-constraint-contact-solve-000728.md`.


### M6.22 — Island body-chain solver traversal (`phys_fn_000730`)

An immediate-return mutation suppressing the sleep-group chain traversal is rejected by `NxPhysicsSimulationTests` (`candidate_exit=1`, `stdout_delta=6874`); restoring the source returns exact output. Phase 7 now records 309 closed / 251 deferred rows. Row 000720 remains open because the current fixture does not expose its dirty-island rebuild branch. The post-merge full Phase 7 gate passes at 1,793/1,793; its compressed transcript is recorded in the row raw-evidence folder. Mainline commit: `7f5a0d6b`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-body-chain-solver-000730.md`.

### M6.23 — Dirty island rebuild dispatch (`phys_fn_000720`)

The public articulated transition fixture is insensitive to an immediate-return mutation, so use a direct DLL probe for this private row. Resolve the oracle at pinned RVA `0x16100` and the candidate from the staged link map; seed a scratch root with its dirty bit set and check that the row clears it, marks traversal, and installs a rebuilt island. The no-op mutation is rejected (`candidate_exit=1`, `stdout_delta=4000`); the restored differential is exact. Full Phase 7 passes at 1,798/1,798 assertions. The public Scene fixture remains supplemental because a later body update can mask this dispatch. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-dirty-rebuild-000720.md`.

### M6.24 — Sleep-group island pair linking (`phys_fn_000724`)

The existing registered simulation fixture exercises the pair-link path: an immediate-return mutation changes the staged transcript by 6,606 bytes (both exits zero, exact stderr), including contact response and body trajectories. Restoring and rebuilding returns an exact differential. The full Phase 7 gate remains at 1,798/1,798; this task changes no public headers or production source. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-pair-link-000724.md`.

### M6.25 — CCD swept bounds (`phys_fn_000740`)

The public CCD scene is insensitive to an immediate-return mutation, so the registered CCD test now directly checks the row's validity flag and all six swept bounds using a controlled shape-sphere result. The mutation is rejected (`candidate_exit=1`, `stdout_delta=6`) and the restored differential is exact. The target and direct assertion are registered in Phase 7; the floor is 1,801. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-ccd-swept-bounds-000740.md`.


### M6.26 — Dirty island record reset (`phys_fn_000764`)

The registered simulation exercises the per-body post-joint island reset. An immediate-return mutation causes the candidate to access-violate while the oracle exits cleanly (`stdout_delta=3898`, exact stderr); restoring the source gives an exact staged-pair control. The row is closed through the existing Phase 7 `NxPhysicsSimulationTests` target; no public headers or production code changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-island-record-reset-000764.md`.


### M6.27 — Joint island linking (`phys_fn_000762`)

The registered joint-break simulation exercises the row's joint-island link: an immediate-return mutation suppresses the expected notification and is rejected (`candidate_exit=1`, `stdout_delta=876`). Restoring `Island.cpp` byte-for-byte returns an exact differential. Phase 7 now records 314 closed / 246 deferred rows. No public headers or production source changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-island-link-000762.md`.


### M6.28 — Joint contact suppression scans (`phys_fn_000750`, `000752`)

The public simulation did not expose each body-list scan independently, so `NxPhysicsSimulationTests` now probes the pinned oracle and staged candidate directly using scratch records. Independent scan-only mutants flip `owner_scan` and `fallback_scan` respectively and are rejected; restored output is exact. The shared target is registered for Phases 5, 6 and 7, whose coverage floors increase by one to 2,728, 1,653 and 1,802. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-contact-filter-000750.md` and `000752.md`.


### M6.29 — Body CCD step update (`phys_fn_000770`)

A row-only no-op is rejected by both the registered public simulation (`stdout_delta=7134`) and CCD (`stdout_delta=5`, response velocity outside range); restored controls are exact. Phase 7 moves to 317 closed / 243 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-body-ccd-update-000770.md`.


### M6.30 — Joint island dissolve (`phys_fn_000778`)

The registered public simulation rejects a row-only no-op (`stdout_delta=4`, both exits 0, exact stderr); joint staged-pair and allocator targets are exact under the mutation and are not the detector. Restored controls match across all targets. Phase 7 moves to 318 closed / 242 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-dissolve-000778.md`.


### M6.31 — Joint island continuation array push (`phys_fn_000780`)

The simulation test now directly probes the row's continuation using a seeded joint chain and spare-capacity joint array. Suppressing only the array push changes `queued=1` to `queued=0` while preserving link clearing and body reset; the staged differential rejects it (`stdout_delta=4000`). Restoring the source is exact. Phase 7 moves to 319 closed / 241 deferred. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-array-push-000780.md`.

### M6.32 — Material combine selector (`phys_fn_000855`)

A direct `NxPhysicsSimulationTests` DLL probe checks average, minimum, multiply, maximum, and unknown-mode maximum. Replacing only the row with an immediate return of its first argument is rejected (`candidate_exit=1`, `stdout_delta=4000`); restored output is exact. Phase 7 now records 320 closed / 240 deferred. Phase 5/6/7 coverage floors advance to 2,730/1,655/1,804. See `docs/reconstruction/novodex-physics/evidence/phase7-material-combine-000855.md`.


### M6.33 — Body-base kinematic destructor (`phys_fn_000799`)

An empty-body mutation of `DynamicBodyBase::destruct` drops the kinematic-release free from six to five and is rejected by `NxPhysicsBodyCreationTests` (`stdout_delta=8`, both exits zero). Restored output is exact. The fixture is registered in Phase 7, whose gate passes at 1,975/1,975 coverage assertions. Phase 7 records 321 closed / 239 deferred. See `docs/reconstruction/novodex-physics/evidence/phase7-body-base-dtor-000799.md`.


### M6.34 — Contact friction parameters (`phys_fn_000861`)

Replacing only the friction-parameter calculation with an empty body preserves normal force but changes friction force from `3d5ae33a` to zero. The registered simulation differential rejects the mutation (`stdout_delta=266`, both exits zero), and the restored control is exact. Phase 7 records 322 closed / 238 deferred; its full gate passes at 1,975/1,975. See `docs/reconstruction/novodex-physics/evidence/phase7-friction-params-000861.md`.


### M6.35 — Scene joint removal (`phys_fn_000633`)

A no-op `NxSceneInternal::removeJoint` mutation is rejected by the registered joint staged-pair differential (`candidate_exit=-1073741819`, `stdout_delta=3520`, exact stderr); forcing a fresh CMake rebuild after restoring the source returns exact output. Phase 7 records 328 closed / 232 deferred, then passes after merge at 1,975/1,975. Commits `c0ee0045` and `463e979d` are on local main. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-remove-joint-000633.md`.


### M6.36 — Scene collision-bounds visualization (`phys_fn_000638`)

IDA decompilation confirms that the existing helper walks sections 1 and 2 of the static and selected dynamic pruners, filters shape visualization bit 3, applies the compound type-5 filter, resolves each current world AABB, and submits it with the requested color. A row-only no-op changes `NxPhysicsSceneVisualizeTests` by 2,744 bytes; the restored control is exact. Phase 7 records 329 closed / 231 deferred and passes after merge at 1,975/1,975. Commits `3f4bbdfc` and `1bcd8f35` are on local main. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-visualize-bounds-000638.md`.


### M6.38 — Anisotropic friction helper (`phys_fn_000859`)

Added a direct oracle/candidate fixture for the rotated shape axis, unequal U/V coefficients, and one versus two anisotropic materials. The row-only immediate-return mutation is caught (`candidate_exit=1`, `stdout_delta=4002`); restoring `ContactPairManager.cpp` byte-for-byte returns an exact control. Phases 5, 6, and 7 pass at 2731/2731, 1656/1656, and 1978/1978. The row is dynamically gated and closed in the Phase 7 ledger. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-anisotropic-friction-000859.md`.


### M6.37 — Null-pruner collision-bounds visualization

Added an empty-scene public visualization stage with collision AABBs enabled. The oracle returns an empty renderable for null static and selected dynamic pruners; before the guard, the candidate access-violates (`stdout_delta=1703`, exact stderr). Adding the null-pruner early return restores exact output. The two new coverage assertions raise the Phase 7 floor to 1,977, and the formal gate passes at that count both before and after merge commit fa169984. This refines row `phys_fn_000638`; it does not add another closed function row.


### M6.39 — Kind-selected accumulator (`phys_fn_000867`)

The previous object-layout cases compared the oracle function with a test-local formula and did not reach the built candidate DLL. Added a 15-case direct staged-pair fixture for the candidate map symbol and oracle RVA. A row-only immediate-return mutation changes the full-object digest (`337db078` to `3695bd32`, `stdout_delta=138`); restored control is exact. This row is now dynamically gated and closed in the Phase 7 ledger. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-accumulator-000867.md`.


### M6.40 — Scene dump setting serialization (`phys_fn_003997`)

A row-only early-false mutation in `SceneDumpSetting::store` is detected by the registered `NxPhysicsCoreDumpTests` staged-pair differential (`stdout_delta=3776`, exact stderr). Restoring the source byte-for-byte yields an exact control. The row is now dynamically gated and closed in the Phase 6 ledger. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-scene-dump-setting-003997.md`.


### M6.41 — Scene dump setting print (`phys_fn_003999`)

A row-only no-op at `SceneDumpSetting::print` is detected by the registered `NxPhysicsCoreDumpTests` staged-pair differential (`stdout_delta=280`, exact stderr). Restoring the source and forcing recompilation yields an exact control. The row is dynamically gated and closed in Phase 6. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-scene-dump-print-003999.md`.


### M6.42 — Scene dump date/time format (`phys_fn_003992`)

`PhysicsCoreDumpTests` now validates the generated date/time fields before normalizing the wall-clock line. The existing registered date-line marker fails on an entry-return mutation (`stdout_delta=40`); restored source and forced rebuild pass exactly. The row is dynamically gated and closed in Phase 6. Evidence: `docs/reconstruction/novodex-physics/evidence/phase6-scene-dump-datetime-003992.md`.


### Continuation — actor owner-pair cleanup (`phys_fn_000521`, 2026-10-14)

Reconstructed the Scene member helper that removes owner-pair records for an actor's root shape before deleting it. The direct staged fixture covers both root-present removal and no-root preservation. A no-op root branch is rejected (`candidate_exit=1`, `stdout_delta=4007`); byte-exact restoration matches the oracle. Phase 7 now records 332 closed / 228 deferred functions, with a fresh full gate passing at 1,980/1,980. The shared simulation fixture also advances the Phase 5/6 coverage floors. No public headers changed. See `docs/reconstruction/novodex-physics/evidence/phase7-actor-owner-pair-cleanup-000521.md`.


### Continuation — SDK cache release (`phys_fn_000492`, 2026-10-14)

Closed the seven-block teardown discrepancy by freeing Scene shape-pair hashes and trigger-pair vectors, releasing cached BoxPruning radix sorters, and validating the existing OPCODE pool cleanup. Each path is mutation-tested; the restored populated-scene differential is exact and reports zero outstanding allocator blocks. Fresh Phase 7 passes at 1,980/1,980 with the pinned oracle pair. `phys_fn_000492` and thunk `phys_fn_000494` remain open pending full destructor-row closure. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-cache-release-000492.md`.


### Continuation — SDK Scene-before-mesh teardown (`phys_fn_000492`, 2026-10-14)

A staged fixture with a live triangle-mesh actor falsified the candidate's mesh-before-Scene order. The oracle frees the registered Scene first; moving mesh destruction after the Scene loop restores an exact differential. Fresh Phase 7 passes at 1,980/1,980. The SDK destructor row and scalar deleting thunk remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-scene-before-mesh-000492.md`.


### Continuation — SDK process-wide cache release order (`phys_fn_000492`, 2026-10-14)

A staged actor-group-pair cache fixture caught that the candidate freed this global cache before its registered Scenes and meshes. Moving the reset after Scene, mesh, OPCODE-pool, and BoxPruning-cache teardown restores an exact differential. Fresh Phase 7 passes at 1,980/1,980. The destructor row and deleting thunk remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-cache-order-000492.md`.


### Continuation — Scene pair refresh (`phys_fn_001976`, 2026-10-14)

The registered simulation differential already exercises the reconstructed `nxSceneRefreshPairs` implementation across all three public broadphase selectors, separated and overlapping actors, dense and bounded pairs, plane contact, pair ordering, and coherent-pruner separation/rejoin. An entry-return mutation is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4000`); restoring and rebuilding returns an exact differential. The Phase 7 row is now dynamically gated. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-pair-refresh-001976.md`.


### M6.43 — PhysicsSDK scalar deleting destructor flag 1 (`phys_fn_000494`)

Extended the populated-scene teardown fixture to assert that public `release()` frees the internal 56-byte PhysicsSDK allocation through the generated scalar deleting destructor with flag 1. The direct vtable flag-0 check remains and verifies preserved storage and return identity. A candidate-only mutation suppressing the flag-1 free is rejected (`candidate_exit=1`, `stdout_delta=7`, `freed=0`); restored oracle/candidate teardown output is exact. This exposed a stale pinned source hash for `NxPhysicsObjectLayoutTests`: commit `f7f9c839` expanded the committed fixture without refreshing its oracle-proof manifest. Updating the manifest to the current committed fixture hash preserved the oracle digest (`774d629d`) and restored green clean-checkout gates: Phase 3 `974/974`, Phase 5 `2737/2737`, and Phase 7 `2867/2867`. Raw gate logs are in `evidence/raw/phase7-sdk-scalar-delete-flag1-000494-20261014/`. The user's separate uncommitted fixture edit remains untouched. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-scalar-delete-flag1-000494.md`.


### 2026-10-15 continuation — Pruner pool insertion and approved date/time assertion

`NxPrunerProcessPool::add` (`phys_fn_005442`) is closed through the registered `NxPhysicsSceneRaycastTests` actual-DLL differential. Returning handle 0 at function entry changes the dynamic registration word and fails the candidate assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2794`); the restored candidate is exact (`stdout_delta=0`, exact stderr). Phase 4 now has 39 closed / 1,014 deferred rows, and the full-census audit backlog remains 2,786 executable rows; Phase 4 closure does not terminally close a full-image row. The date/time assertion approved by the user was also rerun through `NxPhysicsCoreDumpTests`: actual date fields validate before normalization and the oracle/restored pair is exact. Details and compressed transcripts are in `docs/reconstruction/novodex-physics/evidence/phase4-pruner-add-005442.md`. Pool growth, allocation failure, and full-image audit work remain open; the full-DLL objective is not complete.


### 2026-10-15 continuation — Pruner pool replacement

`NxPrunerProcessPool::replace` (`phys_fn_005440`) is now mutation-falsified through the registered Scene raycast staged pair. An entry-return mutant preserves the initial registration and generation-reuse markers, then crashes during final pool cleanup (`candidate_exit=-1073741819`, `stdout_delta=13`, exact stderr); the oracle exits 0. The restored pair is exact. Phase 4 now records 40 closed / 1,013 deferred rows; the full-image audit backlog remains 2,786 rows. See `docs/reconstruction/novodex-physics/evidence/phase4-pruner-replace-005440.md`. Further pool size and allocation-failure cases remain open.


### 2026-10-15 continuation — Pruner pool removal

`NxPrunerProcessPool::remove` (`phys_fn_005444`) is mutation-falsified through generation-tagged handle reuse in the registered Scene raycast pair. An entry-return mutant preserves the old handles, causing the next Scene to receive indices 4/5 rather than 2/3 with generation 1 (`stdout_delta=2`); the restored pair is exact. Phase 4 now records 41 closed / 1,012 deferred. The full-image audit backlog remains 2,786 rows. See `docs/reconstruction/novodex-physics/evidence/phase4-pruner-remove-005444.md`. Generation wraparound and OOM branches remain open.


### 2026-10-15 continuation — Pruner process-pool constructor

`NxPrunerProcessPool::NxPrunerProcessPool` (`phys_fn_005438`) is mutation-falsified through the actual-DLL Scene raycast pair. An entry-return mutation crashes the candidate during SDK teardown (`candidate_exit=-1073741819`, `stdout_delta=2804`, exact stderr); the restored staged pair is exact. Phase 4 now records 42 closed / 1,011 deferred. Pool allocation failure and large-capacity boundaries remain open. See `docs/reconstruction/novodex-physics/evidence/phase4-pruner-pool-constructor-005438.md`.


### 2026-10-15 continuation — SDK pruner-pool release shim

`nxOpcodeReleasePool` (`phys_fn_005446`) is mutation-falsified through the registered SDK allocator fixture. A no-op removes all five pool frees at SDK release and changes the staged transcript (`stdout_delta=6`, both exits 0, exact stderr); the restored transcript is exact. The Phase 4 closure is discharged by Phase 5, where the fixture is registered. Phase 4 now records 43 closed / 1,010 deferred. See `docs/reconstruction/novodex-physics/evidence/phase4-pruner-release-pool-005446.md`.

### 2026-10-15 continuation — Pruner process-pool growth

Extended `NxPhysicsSceneLifecycleTests` to instantiate static and dynamic Pruners in three simultaneously live Scenes. The oracle and candidate report six active registration handles (`0` through `5`), proving growth beyond the constructor's initial capacity of two; after releasing the middle Scene, the appended Scene reuses the two freed slots at generation 1 (static handle `0x00010003`, dynamic handle `0x00010002`). A candidate-only mutation disabling `NxPrunerProcessPool::add`'s capacity-growth branch crashes during the third registration (`oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=5`, exact stderr); the restored DLL differential is exact. Phase 5 passes at 2,743/2,743 and the tooling suite passes all 816 tests. This closes the recorded growth/reuse test gap but changes no production code row; allocation failure and the complete 2,786-row DLL backlog remain open. Evidence and compressed transcripts: `docs/reconstruction/novodex-physics/evidence/phase5-pruner-process-pool-growth-20261015.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase5-pruner-process-pool-growth-20261015/`.


### 2026-10-15 continuation — NpScene internal Scene accessor (`phys_fn_000287`)

Made the +0x24 NpScene pointer getter an explicit noinline method and routed public `releaseScene` through it. The registered two-Scene staged-pair test now pins the row marker and survivor lookup. Returning `this` is rejected (oracle exit 0, candidate exit 1, `stdout_delta=7`); restored control is exact. Phase 2 records 84 closed / 59 deferred functions. No public header changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-row-000287.md`.

### 2026-10-15 continuation — NpScene gravity source mapping (`phys_fn_000289`)

Completed the source-to-candidate census entry for the already reconstructed gravity setter. IDA's oracle body and the existing mutex-wide mutation differential identify `NpScene::setGravity`; the current linker map resolves the candidate symbol in `NpScene.obj`. Added the exact write-lock/forward/error-return contract and candidate map address to the inventory. This mapping does not change phase closure counts or the full-image backlog.

### 2026-10-15 continuation — Prunable owner-query callback (`phys_fn_000965`)

Added a direct oracle-RVA/candidate-map probe for the three-byte callback that always returns zero. The existing scene raycast differential did not observe this row: a return-one mutation survived. The new direct call with null and opaque owners rejects that mutation; the restored pair is exact. Phase 4 passes at `542/542`, discharging the row's Phase 2 homeless-shared-code deferral through its later-phase caller. Phase 2 now records 85 closed / 58 deferred rows. Evidence: `docs/reconstruction/novodex-physics/evidence/phase4-prunable-owner-query-000965.md`.

### 2026-10-15 continuation — NpScene gravity getter source mapping (`phys_fn_000291`)

Recorded the existing getter implementation and its Win32 Release map symbol, `?getGravity@NpScene@@UAEXAAVNxVec3@@@Z` at `0x10042660` in `NpScene.obj`. The registered simulation differential already checks both the descriptor gravity and the changed value, and the row's zero-vector mutation is rejected. This source-to-candidate census entry does not change phase closure counts or the full-image backlog. The user-approved Phase 6 date/time assertion remains covered by its fresh `1,662/1,662` gate. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-getgravity-000291.md` and `docs/reconstruction/novodex-physics/evidence/phase6-scene-dump-datetime-003992.md`.

### 2026-10-15 continuation — NpScene pair-query source mappings (`phys_fn_000317`, `phys_fn_000319`)

Recorded candidate map references for the already reconstructed pair-count and pair-flag-array wrappers in `NpScene.obj`. Their registered Phase 7 differential already rejects a zero count and a missing array copy, and the restored control is exact. This census update does not change phase closure counts or the full-image backlog. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-pair-query-wrappers-000317-000319.md`.

### 2026-10-15 continuation — NpScene legacy-run wrapper mappings (`phys_fn_000333`, `phys_fn_000335`, `phys_fn_000336`)

Recorded the Win32 Release map symbols for `flushStream`, `startRun`, and `finishRun`. The first is the oracle's no-op public slot; the legacy run wrappers already have Phase 7 mutation-falsified simulation coverage with exact restored output. These map links complete source-to-candidate census for the three rows without changing the DLL or phase closure totals. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-npscene-flush-stream-000333.md` and `docs/reconstruction/novodex-physics/evidence/phase7-npscene-legacy-run-wrappers-000335-000336-000342.md`.

### 2026-10-15 continuation — SDK pointer-binding lookup audit correction (`phys_fn_000454`)

Reconciled an invalid Phase 8 audit caveat: oracle RVA `0x0000df90` is a complete 58-byte cdecl key/value lookup, implemented as `nxGetSdkPointerBinding` in `PhysicsInternal.cpp`; the cited VA `0x10007460` lies inside an unrelated actor velocity setter. IDA confirms the null guards, linear scan, first-match return, and null-on-miss behavior, and the Win32 Release map resolves the candidate implementation in `PhysicsInternal.obj`. The internal test suite exercises misses, rebinding, removal, and table cleanup; a return-null mutation is caught at “both read back,” and restored CMake builds pass. This is not an oracle DLL differential, so the full-census row remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-sdk-pointer-binding-get-000454.md`.

Mapped the companion setter `phys_fn_000480` to `nxSetSdkPointerBinding` and its Win32 Release symbol in `PhysicsInternal.obj`. Its existing internal mutation rejects failure to destroy and clear the final-entry table; the restored test suite passes. Both rows remain part of the open full-census backlog because these checks are candidate assertions rather than oracle DLL differentials. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-sdk-pointer-binding-set-000480.md`.

### Continuation — indexed-manager growth continuation (`phys_fn_002419`, 2026-10-15)

IDA shows the 89-byte RVA `0x0005c100` range is an internal continuation of the shared indexed-manager insertion body at `0x0005bc90`. The Phase 3 `NxPhysicsBodyCreationTests` target already drives its record-array growth through 257 live bodies. Omitting only the record active-list append changes that marker (`stdout_delta=6`) while the shape path remains exact; restored oracle/candidate output matches. Phase 2 now records 86 closed / 57 deferred functions, discharged through Phase 3. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-indexed-manager-continuation-002419.md`.

### Continuation — Scene visited-buffer growth (`phys_fn_000503`, 2026-10-15)

Added direct oracle-RVA/candidate-map coverage for the Scene's cached visited-buffer growth, including capacity rounding, allocation/free order, zeroing, stamp and embedded container/pruner rebinding. The Pruner rebind mutation is caught and restored output is exact. This row's Phase 2 deferral remains open because fresh Phase 5/7 runs stopped at the pinned-source hash check for the separate local `tests/PhysicsObjectLayoutTests.cpp` edit; the fixture edit remains untouched. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-visited-buffer-growth-000503.md`.

### Continuation — scratch-stamp wrap (`phys_fn_000505`, 2026-10-15)

The Phase 4 polygon-interface oracle differential drives 65 scratch-stamp wraps. Changing the candidate's wrap restart from the scratch capacity to zero yields 714 mismatches; the restored linked implementation returns to zero mismatches. Phase 2 now records 87 closed / 56 deferred functions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase2-scratch-stamp-wrap-000505.md`.


### M7 continuation — `phys_fn_003611` global-to-local emitter pose

IDA maps RVA `0x89410` to the shared emitter global-pose conversion. The direct oracle differential covers unattached, matrix-frame, and dynamic-body quaternion cases; a local-X translation mutation is rejected in both attached cases. The helper lives in `Physics/src/ZzFluidEmitterPose.cpp`; its CMake source order preserves `NxCreatePhysicsSDK` at RVA `0x00055e50`, avoiding the link-layout-sensitive allocator crash. Phase 5 passes at 2,768/2,768; Phase 7 passes at 2,882/2,882 and the tooling suite passes with 831 tests and 743 subtests. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-global-to-local-003611.md`.

### Phase 6 continuation — Effector base destructor (`phys_fn_003936`, 2026-10-15)

The registered `NxPhysicsEffectorTests` staged-pair release path reaches the base `Effector` destructor through the derived deleting-destructor chain. A candidate naked-return mutant suppressing the compiler-generated `Observable` base teardown is rejected (oracle exit 0, candidate access violation, `stdout_delta=75`, exact stderr); the restored control is exact. `phys_fn_003936` is now mutation-falsified in the Phase 6 ledger. Fresh Win32 Release Phase 6 passes at 1,666/1,666, including the approved core-dump date/time assertion. Full DLL reconstruction remains open. See `docs/reconstruction/novodex-physics/evidence/phase6-effector-base-destructor-003936.md`.

### Phase 6 continuation — Effector scalar deleting destructor (`phys_fn_003938`, 2026-10-15)

Added direct oracle-RVA/candidate-map calls for the abstract `Effector` scalar deleting destructor with flags 0 and 1. The probe checks return identity, stack balance, and the flag-controlled 0x24-byte deallocation. A candidate-only no-op allocator-delete mutation is caught (`mismatches=1`, candidate exit 1, `stdout_delta=12`); the restored differential is exact. `phys_fn_003938` is now mutation-falsified, with Phase 6/7 coverage floors advanced to 1,668/2,884. See `docs/reconstruction/novodex-physics/evidence/phase6-effector-deleting-destructor-003938.md`.

### Phase 7 continuation — controller position getter (`phys_fn_002334`, 2026-10-15)

IDA confirms the four-byte getter is `lea eax,[ecx+0x28]; ret`, implemented by `NxControllerCore::getPosition` in `Physics/src/Scene.cpp`. The registered `NxPhysicsSimulationTests` differential covers two initialized controller positions and the post-move position. A candidate-only `+0x2c` offset mutation is rejected after explicitly rebuilding `NxPhysics` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=62`, exact stderr); the restored getter rebuild matches exactly. Phase 7 records 353 closed / 207 deferred; its coverage floor is unchanged. See `docs/reconstruction/novodex-physics/evidence/phase7-controller-getposition-002334.md`.

### Phase 7 continuation — Scene trigger-pair vector release (`phys_fn_002342`, 2026-10-15)

The populated Scene teardown differential retains both embedded trigger-pair allocations and checks them after `releaseScene`. Skipping only the +0x18 vector free in the candidate makes `previous_freed=0`, fails the teardown assertion, and is rejected (oracle exit 0, candidate exit 1, `stdout_delta=15`). Restoring the free and rebuilding returns the staged transcript to exact equality. Phase 7 now records 354 closed / 206 deferred; the existing coverage floor is unchanged. See `docs/reconstruction/novodex-physics/evidence/phase7-scene-trigger-pair-release-002342.md`.
### Phase 7 continuation — Scene trigger-pair list initialization (`phys_fn_002346`, 2026-10-15)

The populated-scene teardown and trigger-simulation staged-pair differentials exercise the reconstructed two-lane initializer. Changing only the second header pointer from `self+0x18` to `self+0x14` crashes both candidate trigger paths (oracle exit 0; candidate exit `-1073741819`; `stdout_delta=38` and `32`). The constructor-only control remains exact. Restoring the pointer and rebuilding returns all three target controls to exact equality. The full Phase 7 gate passes at 2,884/2,884 assertions; Phase 7 now records 355 closed / 205 deferred. See `docs/reconstruction/novodex-physics/evidence/phase7-scene-trigger-pair-init-002346.md`.


### Continuation — lock-API push-self wrappers (`phys_fn_002369`, `002373`, 2026-10-15)

The existing `nxLockApiPushSelf` source comparison is now required coverage for both lock-API callback wrappers. A candidate-source mutation from `fn(v)` to `fn(v ^ 1u)` is rejected at both row markers (`mismatches=1` each; candidate failures=2); restored source is rebuilt byte-for-byte and both controls match. Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,771/2,771, 1,671/1,671, and 2,887/2,887 coverage assertions. Phase 7 advances to 358 closed / 202 deferred. Public headers and production behavior are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-push-self-002369-002373.md`.


### Continuation — lock-API call and result wrappers (`phys_fn_002371`, `002375`, 2026-10-15)

The existing `nxLockApiCall2371` and `nxLockApiTest2375` source comparisons are now required gate rows. Changing the forwarded 002371 word is caught (`mismatches=1`); inverting 002375's boolean return is caught in both callback-result cases (`mismatches=2`). Restored controls match. The Phase 7 ledger advances to 360 closed / 200 deferred. Public headers and production behavior are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-call-and-test-002371-002375.md`.


### Continuation — lock-API worker record destructor (`phys_fn_002392`, 2026-10-15)

Row 002392 now reconstructs the complete worker-record teardown: vptr reset, conditional termination/state transition, unconditional handle close, Foundation allocator release, and owner-pointer clear. Its oracle/source differential covers states 0, 1, and 2, observes the record state at free time and the freed pointer, and checks handle closure and owner bytes. A no-op mutation reports four mismatches and exits 1; restored control reports row mismatches=0 and `layout result=differential-pass`. Public SDK headers are unchanged. Phase 7 advances to 366 closed / 194 deferred; fresh Phase 5/6/7 gates pass at 2,787/2,787, 1,688/1,688, and 2,904/2,904 assertions, and all 816 reconstruction-tool tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-destroy-002392.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-worker-destroy-002392-20261015/`.

### Continuation — lock-API worker scalar deleting thunk (`phys_fn_002394`, 2026-10-15)

The adjacent MSVC scalar deleting thunk now completes the worker record's deleting-destructor path. The direct flags 0/1 oracle differential checks object lifetime and destructor side effects; an inverted flag mutant is rejected with heap corruption, and the restored implementation matches. Phase 7 advances to 367 closed / 193 deferred. The row is registered in all shared Phase 5/6/7 layout gates. See `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-scalar-delete-002394.md`.

### Phase 7 continuation — lock-API worker base constructor (`phys_fn_002377`, 2026-10-15)

IDA evidence identifies a compact 55-byte constructor at RVA `0x5b820`, called by the newly closed scalar deleting thunk's corresponding owner constructor. It installs the base vtable, allocates a 0x14-byte record through `nxFoundationSDKAllocator`, clears bytes 0–8 of that record, and returns the owner pointer. A direct oracle/source fixture with distinct patterned allocator shims will verify allocation size/ownership, initialized bytes, untouched tail bytes, and return identity. The public SDK header set remains immutable.

### Phase 7 continuation — lock-API worker base constructor (`phys_fn_002377`, 2026-10-15)

IDA confirms the base constructor installs vtable `0x1010886c`, allocates a 0x14-byte worker record through the Foundation allocator, and clears the first nine bytes. The direct oracle/source fixture uses distinct patterned allocator shims and verifies allocation size, zeroed fields, untouched tail bytes, and return identity. A byte-8-to-1 mutant is rejected (`mismatches=1`); the restored control matches. Phase 7 advances to 368 closed / 192 deferred. The row adds one assertion to each shared Phase 5/6/7 gate. See `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-base-construct-002377.md`.
Fresh Release Phase 5/6/7 gates pass at 2,781/2,781, 1,681/1,681, and 2,897/2,897 assertions, including the direct constructor marker on all three phases. The 816-test tool suite and inventory validator pass. See the raw evidence packet at `docs/reconstruction/novodex-physics/evidence/raw/phase7-lock-api-worker-base-construct-002377-20261015/`.

### Phase 7 continuation — derived lock-API worker constructor (`phys_fn_002396`, 2026-10-15)

The constructor delegates base record initialization to phys_fn_002377, stores its three arguments at +8/+0xc/+0x14, installs vtable `0x10108894`, and preserves +0x10. The registered allocator-shim oracle/source fixture checks the full object and record layout. A one-bit third-argument mutation is rejected (`mismatches=1`); restored control matches. Phase 7 advances to 369 closed / 191 deferred. The row adds one assertion to each shared Phase 5/6/7 gate. See `docs/reconstruction/novodex-physics/evidence/phase7-lock-api-worker-construct-002396.md`.

### Phase 7 continuation — Scene callback-dispatch continuations (`phys_fn_000642`, `phys_fn_000644`, 2026-10-15)

The synthetic fetch callback fixture now verifies trigger/contact listener identity, event values, exactly-once delivery, and queue-end reset. Isolated candidate mutations suppressing the trigger callback, joint-break drain, trigger queue reset, contact callback, and contact queue reset are rejected with `stdout_delta=4072`, `842`, `4074`, `610`, and `587`; the restored `NxPhysicsSimulationTests` differential is exact. The Phase 5, 6, and 7 gates pass at 2,786/2,786, 1,687/1,687, and 2,903/2,903. All 816 reconstruction-tool tests pass; inventory validation reports Phase 7 at 371 closed / 189 deferred, with 6,338 function rows, 5,138 data objects, and zero unexplained records. Public Physics headers and production callback behavior are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-callback-dispatch-000642-000644.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-scene-callback-dispatch-000642-000644-20261015/`.


### Phase 7 continuation — Scene fluid creation (`phys_fn_000645`, 2026-10-15)

Mapped oracle RVA `0x12b80` to `NxSceneInternal::createFluid` in `Physics/src/Scene.cpp` and its candidate map symbol. The registered simulation differential now has a row-specific assertion spanning the shipped disabled-backend warning and the enabled callback-backed create, descriptor validation, list visibility, callback lifecycle, and allocation failures. An enabled-path early-return mutation is rejected (`candidate_exit=1`, `stdout_delta=94`); after restoring the source and forcing a real `Scene.cpp` rebuild, both sides report the assertion and match exactly (`stdout_delta=0`, `stderr_exact=True`). Phase 7 advances to 372 closed / 188 deferred. The separate FluidManager create row `phys_fn_003649` remains partial and open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-scene-fluid-create-000645.md`.

Fresh Release Phase 5/6/7 gates pass at 2,787/2,787, 1,688/1,688, and 2,904/2,904; the reconstruction-tool suite passes all 816 tests. Both 80-file public-header trees match the pinned manifest, inventory validation reports unexplained=0, and the regenerated full-image backlog remains 2,786 open code rows. See `docs/reconstruction/novodex-physics/evidence/raw/phase7-scene-fluid-create-000645-20261015/`.


### Phase 7 continuation — null FluidCreate backend result in `phys_fn_003649`

The enabled simulation fixture now forces `FluidCreate` to return null for one valid descriptor and confirms the oracle still publishes the local fluid wrapper into both manager arrays, then tears down the manager without calling `FluidDestroy`. A candidate early-return mutant is rejected (`stdout_delta=60`); the restored differential is exact. The new row-specific marker raises the Phase 5/6/7 floors to 2,788 / 1,689 / 2,905. `phys_fn_003649` remains partial and open; this branch proof does not substitute for its full 2,452-byte audit. See `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md`.

Fresh Phase 5/6/7 gates pass at the updated floors and all 816 reconstruction-tool tests pass. Immutable public headers pass, inventory validation reports `unexplained=0`, and the raw rerun logs are preserved alongside the transient Phase 5 timeout diagnostic at `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-create-null-backend-003649-20261015/`.

### Phase 7 continuation — FluidManager descriptor-validation corpus

Expanded the `phys_fn_003649` oracle fixture to 48 invalid descriptors, covering scalar/action constraints, particle output strides, and implicit-mesh data invariants. The registered marker requires each invalid descriptor to report the oracle's line-140 diagnostic. The earlier 35-case `desc.isValid()` bypass mutant is rejected (`stdout_delta=73`); the restored differential matches. The previously uncommitted object-layout fixture edit is now committed with its updated source hash; all 14 retained oracle proofs are rebound to the new baseline manifest. Phase 5/6/7 pass at 2,788/2,788, 1,689/1,689, and 2,905/2,905 assertions; 831 tooling tests and 743 subtests pass. Public headers are unchanged. The row remains partial/open pending the rest of the creation/failure-order audit. See `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md`.

After integrating the object-layout edit and updating its pinned source hash, Phase 5 passes at 2,788/2,788; Phase 6 passes at 1,689/1,689 after a transient allocator mismatch cleared on direct and full reruns; Phase 7 passes at 2,905/2,905. The tooling suite passes 831 tests and 743 subtests. The 48-case validation corpus is committed on main; full `phys_fn_003649` reconstruction remains open.

### Continuation — fluid particle cell quantization (`phys_fn_003423`, 2026-10-15)

IDA confirms the shared particle initializer rounds each byte stride down to a multiple of four, stores a rounded float reciprocal of cell size, multiplies coordinates, and floors the resulting values. `NxPhysicsFluidStrideAlignmentTests` distinguishes rounded strides from the candidate's prior raw-byte stepping; removing the three stride masks is mutation-rejected. The negative-coordinate oracle differential caught truncation toward zero; a `truncf` mutation is rejected. A precision-boundary differential caught direct division (`71`) where reciprocal multiplication yields the oracle's `70`; a direct-division mutation is rejected. `NxPhysicsFluidNullParticleCountTests` confirms the null-count early return; removing its guard is mutation-rejected. A poisoned-cell fixture confirms the oracle writes zero coordinates when cell size is zero; removing those stores is mutation-rejected. A continuation audit clears x87 status in the cell-size callback and confirms oracle/candidate agreement (`fpu-status=0001`, `divzero=0`); reciprocal computation is guarded by the clipped count as in the oracle. The shared candidate implementation is `nxFluidInitializeParticles` in `Physics/src/Scene.cpp` at map RVA `0x10067cb0`. The row remains partial/open for the full instruction-level audit. Phase 5/6/7 pass at 2,791/2,791, 1,692/1,692, and 2,913/2,913; 835 reconstruction-tool tests and 743 subtests pass. Next: continue the remaining `phys_fn_003423` audit, then continue the fluid initialization dependency rows without changing public headers. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-rounding-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-reciprocal-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-stride-alignment-003423.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-null-count-003423.md`, and `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-cell-size-zero-003423.md`.

### Phase 7 continuation — fluid-emitter record constructor (`phys_fn_003557`, 2026-10-16)

Reconstructed the private 0x78-byte emitter record constructor and wired it into the real fluid-emitter factory. The map-resolved helper installs the internal vtable and wrapper, writes the backend sentinel, initializes both local and world identity poses, and preserves the flags and frame-actor words that the oracle leaves untouched. The registered ABI fixture rejects a zeroed world-pose mutation (`mismatches=7`, exit 1) and the restored oracle/candidate comparison passes. The adjacent scalar deleting destructor `phys_fn_003603` is now reconstructed and mutation-falsified: omitting SDK pointer-binding cleanup is detected (`mismatches=1`), while restored control clears the binding in both delete-flag paths with zero mismatches. Phase 7 advances to 381 closed / 179 deferred. Fresh Win32 Release Phase 5 and 7 gates pass at 2,795/2,795 and 2,917/2,917; all 816 reconstruction-tool tests pass after refreshing the shared oracle-proof hashes and generated work-unit map. Inventory validation reports `unexplained=0`, and public headers remain unchanged. The next adjacent open row is the large emitter visualization routine `phys_fn_003607`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-record-constructor-003557.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-record-destructor-003603.md`, and the corresponding dated raw evidence directories.


### Continuation — fluid emitter visualization (`phys_fn_003607`, 2026-10-16)

The 3,026-byte `phys_fn_003607` routine at RVA `0x886b0` is reconstructed in `Physics/src/SceneVisualize.cpp`. The registered `NxPhysicsFluidEmitterAbiTests` fixture compares unattached, matrix-frame, and quaternion-frame oracle and candidate primitive streams; all three match with balanced stack, including the unattached digest `c284be3c`. An immediate-return mutation is rejected in all three cases (`mismatches=3`). Fresh Phase 5 and Phase 7 gates pass at 2,798/2,798 and 2,920/2,920; all 816 reconstruction-tool tests pass; inventory validates with `unexplained=0`. The approved Phase 5 object-layout baseline matches its normalized fixture hash, and the oracle-only fluid proof is refreshed. Public headers remain unchanged, and CMake builds the candidate. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-visualize-003607.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-emitter-visualize-003607-20261016/`. The next adjacent open row was `phys_fn_003626` at RVA `0x89a00`.

### Phase 7 continuation — FluidKey feature resolver (`phys_fn_003626`, 2026-10-16)

Mapped `phys_fn_003626` at RVA `0x89a00` to `nxFluidResolveLicensedFeatureFlags` in `Physics/src/Scene.cpp`. The direct oracle/candidate differential covers eleven isolated-registry inputs: all three licensed product codes, unknown code, bad token, unequal token lengths, numeric suffix parsing, ignored fourth token, wrong registry type, missing value, and missing product key. Restored output matches (`input_digest=04893d50`, oracle/candidate digest `8d75d855`, mismatches=0). A zero-return candidate mutation is rejected on five cases (`mismatches=5`). Phase 7 advances to 383 closed / 177 deferred. Fresh Phase 5 and Phase 7 gates pass at 2,799/2,799 and 2,921/2,921; all 816 reconstruction-tool tests pass; inventory validates with `unexplained=0`. Public header trees remain unchanged and the candidate builds through CMake. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-license-resolver-003626.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-license-resolver-003626-20261016/`.

### Continuation — FluidCreate callback argument map (`phys_fn_003649`, 2026-10-16)

The registered simulation differential now supplies distinct values for the descriptor fields forwarded by `phys_fn_003649` to `FluidCreate` and verifies all fourteen non-pointer callback arguments. Oracle and restored candidate match (`arguments=14`, digest `1d2e206d`). A word-19-to-word-18 candidate mutation is rejected (`candidate_exit=1`, `stdout_delta=64`). Phase 5, 6, and 7 pass at 2,800/2,800, 1,693/1,693, and 2,922/2,922 assertions. This closes the argument-map slice only; `phys_fn_003649` and full DLL reconstruction remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-create-argument-map-003649-20261016/`.


**Phase 7 FluidManager combined initialization/teardown, 2026-10-16:** Added a registered licensed fixture combining implicit mesh creation, descriptor emitter creation, and initial-particle allocation. It pins `CMEP` callback order and the oracle's fluid-release callback counts (`1.0.0`). The candidate's extra implicit-mesh destroy callback during fluid teardown was removed to match the shipped DLL. Omitting mesh initialization is caught (`candidate_exit=1`, `stdout_delta=72`); restored differential is exact. Fresh Phase 5/6/7 gates pass at 2,801/2,801, 1,694/1,694, and 2,923/2,923 assertions. `phys_fn_003649` remains partial and open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-combined-init-teardown-003649-20261016/`.


**Phase 7 FluidManager implicit-mesh allocator route, 2026-10-16:** Oracle decompilation of `phys_fn_003649`'s mesh helper shows a 0x3c-byte SDK allocation for the manager and a separate 0x18-byte process-heap allocation for its public `NpImplicitMesh` wrapper. The candidate incorrectly used `NxUserAllocator` for that wrapper. A 0x18-byte SDK allocator failure probe caught the candidate (`public-mesh=0`, one SDK allocation attempt) while the oracle retained the mesh (`public-mesh=1`, zero SDK attempts; `stdout_delta=64`). Switching the wrapper allocation/deallocation to the process heap restores exact output. Fresh Phase 5/6/7 gates pass at 2,802/2,802, 1,695/1,695, and 2,924/2,924. `phys_fn_003649` remains partial/open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-mesh-allocation-route-003649-20261016/`.

The phys_fn_003649 first-fluid allocation failure is now covered while the newly created FluidManager's initial static-collision refresh is still dirty. The oracle faults loading fluid + 0x80 after its 0x84-byte allocation fails; the candidate previously returned null. The staged differential caught that mismatch, and the candidate now reproduces the fault only while the dirty refresh and callback path are active. Both manager-growth failure targets match exactly. Fresh Phase 5/6/7 pass at 2,806/2,806, 1,697/1,697, and 2,930/2,930; all 835 tooling tests and 743 subtests pass. The full 2,452-byte FluidManager row and the overall DLL backlog remain open. Evidence: docs/reconstruction/novodex-physics/evidence/phase7-fluid-enabled-create-003649.md and docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-first-create-dirty-refresh-003649-20261016/.

### Phase 7 continuation — Fluid solver process-heap allocation failure (`phys_fn_003649`, 2026-10-16)

The solver's 0x18-byte process-heap failure is now injected in both manager-growth failure targets. IDA shows the constructor skips solver construction on null, then its separate initialization unconditionally writes descriptor `userData` through solver+4. The pre-fix differential was red (`oracle=EXCEPTION_ACCESS_VIOLATION`, candidate=`e06d7363` C++ allocation exception). The candidate now uses nothrow allocation and preserves the unconditional write. Restored runs match exactly for both array-growth ordinals; guarding that write is mutation-rejected (`candidate caught=0`, internal/public counts `2/2`). Phase 7's registered-assertion floor rises from 2,930 to 2,932. `phys_fn_003649` and full DLL reconstruction remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-solver-allocation-failure-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-solver-allocation-oom-003649-20261016/`.

### Phase 7 continuation — FluidManager dirty-refresh null collision (`phys_fn_003649`, 2026-10-16)

The first-fluid dirty-refresh path now has a null-result differential for `FluidManagerGetStaticCollision`. IDA confirms the oracle passes that pointer directly to `phys_fn_003505`, which calls `StaticCollisionCreate` without a null check. The previous candidate returned the fluid and cleared the dirty flag; it now faults at the same point after publishing one entry into each manager array. A guard-restoration mutation is rejected. Both allocation-ordinal target variants pass exactly. Phase 7's required coverage floor rises to 2,934. This closes only the null-collision failure branch; the full FluidManager instruction and teardown audit remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-dirty-refresh-null-collision-003649.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-dirty-refresh-null-collision-003649-20261016/`.

### Phase 7 continuation — Public fluid surface-mesh creation (`phys_fn_003704`, 2026-10-16)

The licensed public `NxFluid::createSurfaceMesh` wrapper is now row-linked to the existing licensed simulation fixture. The fixture checks the explicit returned mesh's `userData`, descriptor values passed to the backend, and the resulting surface properties. Replacing `nxFluidCreateSurfaceMeshPublic` with a null-return mutant is rejected (`candidate_exit=1`, `stdout_delta=77`, row marker `matched=0`); the restored oracle/candidate differential is exact. Phase 7 now records 391 closed / 169 deferred rows, with registered floors at Phase 5/6/7 2,815/1,704/2,945. The full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-create-surface-mesh-003704.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-create-surface-003704-20261016/`.

### Phase 7 continuation — Implicit-mesh surface-radius setter (`phys_fn_003878`, 2026-10-16)

IDA identifies `phys_fn_003878` as the public `NxImplicitMesh::setSurfaceRadius` path. The licensed simulation fixture now has a row-specific assertion for the setter value, callback count, and lock/re-entry behavior. Removing the backend setter is mutation-rejected (`candidate_exit=1`, `stdout_delta=66`); the restored oracle/candidate run is exact. Phase 7 now records 392 closed / 168 deferred rows, with registered floors at Phase 5/6/7 2,816/1,705/2,946. The full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-surface-radius-003878.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-set-radius-003878-20261016/`.

### Phase 7 continuation — Implicit-mesh single-radius getter (`phys_fn_003880`, 2026-10-16)

IDA identifies `phys_fn_003880` as the read-locked `NxImplicitMesh::getSingleRadius` path. The licensed surface-mesh fixture now pins both initial and updated float bits plus two backend getter calls. A zero-return mutation is rejected (`candidate_exit=1`, `stdout_delta=68`); the restored oracle/candidate run is exact. Phase 7 now records 393 closed / 167 deferred rows, with registered floors at Phase 5/6/7 2,817/1,706/2,947; all three gates and all 816 reconstruction-tool tests pass. The full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-get-single-radius-003880.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-get-single-radius-003880-20261016/`.

### Phase 7 continuation — Optional FluidModel loader (`phys_fn_003900`, 2026-10-16)

IDA identifies the x87-preserving `LoadLibraryA("FluidModel.DLL")` helper. The staged fluid lifecycle has a row-specific module/callback marker, and a fresh child-process `DllMain` mutation verifies that the loader restores the x87 environment; replacing the candidate `fldenv` with `fninit` is rejected. The missing-load mutant is also rejected (`candidate_exit=1`, `stdout_delta=118`), while restored differentials are exact. Phase 7 records 400 closed / 160 deferred rows, with registered floors at Phase 5/6/7 2,825/1,714/2,955. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-model-load-003900.md`.

### Phase 7 continuation — Implicit-mesh name setter (`phys_fn_003896`, 2026-10-16)

The public write-locked `NxImplicitMesh::setName` wrapper now has a row-specific licensed-fixture assertion for both the initial and updated name. Removing the backend name update is mutation-rejected (`candidate_exit=1`, `stdout_delta=68`); the restored oracle/candidate differential is exact. Phase 7 now records 399 closed / 161 deferred rows, with registered floors at Phase 5/6/7 2,823/1,712/2,953. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-name-003896.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-set-name-003896-20261016/`.

### Phase 7 continuation — Implicit-mesh smoothing-iteration accessors (`phys_fn_003892` / `phys_fn_003894`, 2026-10-16)

The public read-locked getter and write-locked setter now have separate row-specific assertions over the licensed surface-mesh fixture. The getter checks initial=6, updated=8 and two backend calls; the setter checks updated=8, one backend call and zero callback-argument errors. Independent zero-result and no-op-setter mutations are both rejected (`candidate_exit=1`, `stdout_delta=70` each); the restored differential is exact. Phase 7 now records 398 closed / 162 deferred rows, with registered floors at Phase 5/6/7 2,822/1,711/2,952. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-smoothing-iterations-003892-003894.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-smoothing-iterations-003892-003894-20261016/`.

### Phase 7 continuation — Implicit-mesh triangle-size setter (`phys_fn_003890`, 2026-10-16)

IDA identifies `phys_fn_003890` as the public `NxImplicitMesh::setTriangleSize` write-lock wrapper. The licensed surface-mesh fixture checks the initial and updated float bits, one backend setter callback, and zero callback-argument errors. Removing the setter dispatch is mutation-rejected (`candidate_exit=1`, `stdout_delta=68`); the restored oracle/candidate differential is exact. Phase 7 now records 396 closed / 164 deferred rows, with registered floors at Phase 5/6/7 2,820/1,709/2,950. All three gates and all 816 reconstruction-tool tests pass; the full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-triangle-size-003890.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-set-triangle-size-003890-20261016/`.

### Phase 7 continuation — Implicit-mesh blend setter (`phys_fn_003886`, 2026-10-16)

IDA identifies `phys_fn_003886` as the public `NxImplicitMesh::setBlend` wrapper. The licensed surface-mesh fixture now verifies its unavailable-feature warning (code 206, line 65), the zero blend getter value, and exactly one backend setter callback. Removing the backend dispatch is mutation-rejected (`candidate_exit=1`, `stdout_delta=68`); the restored oracle/candidate differential is exact. Phase 7 now records 395 closed / 165 deferred rows, with registered floors at Phase 5/6/7 2,819/1,708/2,949; all three gates and all 816 reconstruction-tool tests pass. The full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-blend-003886.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-set-blend-003886-20261016/`.

### Phase 7 continuation — Implicit-mesh single-radius setter (`phys_fn_003882`, 2026-10-16)

IDA identifies `phys_fn_003882` as the write-locked `NxImplicitMesh::setSingleRadius` path. The licensed surface-mesh fixture now pins the initial/updated float bits plus exactly one backend setter call. Removing the backend setter is mutation-rejected (`candidate_exit=1`, `stdout_delta=70`); the restored oracle/candidate run is exact. Phase 7 now records 394 closed / 166 deferred rows, with registered floors at Phase 5/6/7 2,818/1,707/2,948; all three fresh phase gates and all 816 reconstruction-tool tests pass. The full DLL remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-set-single-radius-003882.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-set-single-radius-003882-20261016/`.

### Phase 7 continuation — Fluid solver deleting destructors (`phys_fn_003694` / `phys_fn_003788`, 2026-10-16)

IDA confirms `phys_fn_003694` is the primary scalar deleting destructor for the 0x18-byte fluid solver wrapper and `phys_fn_003788` is its secondary-vfptr adjustor thunk. The candidate now reproduces the embedded read-lock base destructor and primary base-vfptr transition; the secondary entry adjusts `this` by eight and forwards to the primary destructor. The registered scratch-object oracle differential checks both vfptr transitions, payload preservation, and the returned pointer for both entries. Independent mutations prove the primary store and secondary adjustment are each required. Phase 5/6/7 gates pass at 2,814/1,703/2,944; Phase 7 records 390 closed / 170 deferred. Full DLL reconstruction remains open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-solver-destructors-003694-003788.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-solver-dtor-003694-003788-20261016/`.

### Phase 7 continuation — initial-fluid null count and optional streams (`phys_fn_003503`, 2026-10-16)

Two additional branches in the initial-particle helper now have exact oracle differentials. A null initial `numParticlesPtr` skips particle callbacks and allocation while the fluid is still published; absent velocity/lifetime buffers leave the internal record fields zero, verified from `0xa5`-poisoned callback storage. Removing the count guard or velocity zeroing store is mutation-rejected. The Phase 7 gate passes at 2,957/2,957, gate-target tests pass 39/39, and the 80-file public Physics header manifest passes. The full `phys_fn_003503` row and entire DLL remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-null-initial-particle-count-003503-20261016.md`, `docs/reconstruction/novodex-physics/evidence/phase7-fluid-null-initial-optional-buffers-003503-20261016.md`, and `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-null-initial-optional-buffers-003503-20261016/`.

### Phase 7 continuation — flags-only initial-particle failure (`phys_fn_003503`, 2026-10-16)

`NxParticleData::isValid()` accepts a positive particle count with only `bufferFlags`, but the oracle initializer still reads `bufferPos` while populating its allocated ParticleArray. The new caught differential verifies the resulting access violation, one `FluidGetParticles` callback, one array allocation, and no manager publication. A mutation that returns early for a null position buffer is rejected because it publishes the fluid instead. The registered Phase 7 floor is now 2,958 assertions; the full gate and focused tooling checks are recorded in `evidence/phase7-fluid-flags-only-initial-failure-003503-20261016.md` and `evidence/raw/phase7-fluid-flags-only-initial-failure-003503-20261016/`.


### Phase 7 continuation — SDK pointer-binding thunk (`phys_fn_003683`, 2026-10-17)

Reconstructed the thiscall bridge to the existing SDK pointer map and routed existing internal binding call sites through its private MSVC member entry. The registered simulation probe verifies map insertion, replacement, removal, and clearing. A return-false mutation is rejected (`mismatches=1`, `stdout_delta=4106`); cleanly rebuilt restored output matches the oracle exactly. The row is now dynamically gated and mutation-falsified; Phase 7 closes one additional function. Full Phase 5/6/7 gates pass at 2,849/2,849, 1,723/1,723, and 2,982/2,982 assertions, and all 817 reconstruction-tool tests pass. Public headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-sdk-pointer-binding-thunk-003683.md`. Continue with the next open row selected by the completion backlog; this slice does not complete the full DLL reconstruction.


### Phase 7 continuation — fluid surface-mesh manager constructor (`phys_fn_003685`, 2026-10-17)

Reconstructed the 0x3c-byte manager constructor and wired it into surface-mesh initialization, including its default type word and 0x18-byte public mesh allocation. The registered simulation probe checks poisoned storage against the oracle; changing the type from 4 to 5 is mutation-rejected (`mismatches=1`, `stdout_delta=4106`), while restored output is exact. The existing licensed mesh creation and allocation-failure route remains oracle-exact. Fresh Phase 5/6/7 gates pass at 2,850/2,850, 1,724/1,724, and 2,983/2,983 assertions. All 817 reconstruction-tool tests pass after regenerating the row-level work-unit map.

### Phase 7 continuation — free-pruner world-box visualization (`phys_fn_001937`, 2026-10-17)

Made the existing free-pruner blue-AABB loop explicit as `nxVisualizeFreePrunerWorldBoxes` and tied it to IDA's 0x4b9f0 contract. `NxPhysicsSceneVisualizeTests` now requires 60 blue lines in its populated `collision_free` scene. Suppressing the candidate's `addAABB` call is caught by the staged-pair runner (`stdout_delta=1478`, candidate exit 1); restored oracle/candidate output matches exactly. Fresh Phase 7 passed at 2,983/2,983, inventory validation reports zero unexplained rows, and all 836 reconstruction-tool tests pass. Public Physics headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-free-pruner-visualize-001937.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-free-pruner-visualize-001937-20261017/`.

The earlier backlog pointer to `phys_fn_001939` is stale: its Phase 7 ledger entry is already dynamically closed in `gates/phase7-closure.json`. Task 15 completed the candidate mapping and row-specific falsification for `phys_fn_000668` (RVA `0x145c0`). Task 16 records the `NpScene` constructor mapping for `phys_fn_000285`, Task 17 records six `NpScene` run/timing mappings, and Task 18 records pair-query links `000317`/`000319`. The regenerated backlog next selects callback rows `000352`, `000356`, `000360`, `000362`, and `000364`. Full DLL reconstruction remains open.

### Task 15 — Scene scalar-deleting thunk (`phys_fn_000668`)

IDA identifies the 34-byte `__thiscall` thunk at RVA `0x145c0`: it calls Scene cleanup row `000663`, tests deleting-destructor flag bit 0, conditionally releases `this` through `nxFoundationSDKAllocator` slot `+0x14`, returns `this`, and uses `ret 4`. Its vtable reference is at `0x101066f4`. The current candidate's corresponding path is `NxSceneInternal::scalarDeletingDestructor` in `Physics/src/Scene.cpp`, with shared cleanup in `nxSceneDelete`; the candidate map must be read from the fresh CMake build.

Add a dedicated `NxPhysicsSceneScalarDeleteTests` process that creates two SDK-owned Scenes and calls the oracle thunk at RVA `0x145c0` and the candidate thunk by fresh linker-map symbol. Inspect the internal 1,808-byte Scene object at wrapper offset `+0x24`, not the 40-byte public wrapper. Flag 0 must preserve the allocation and return `this`; flag 1 must free it and return `this`; both calls must balance the x86 stack. Register both markers in Phase 7 and the oracle differential registry. Mutation-test the candidate's flag-controlled free and require only the flag-1 marker to diverge. Retain exact mutation/restored transcripts, image hashes, IDA vtable/source/map linkage, and ABI evidence. Public headers stay unchanged.

Refresh the inventory and Phase 7 row ledger for `000668`, regenerate the completion backlog, and run the focused target, Phase 5/7 gates, inventory validation, and both public-header manifests. Close the row only after the vtable mapping and flag behavior are independently established.

Status: complete. The vtable linkage is established; the restored staged-pair differential matches both flag cases, and the flag-1 free mutation is rejected. Phase 5 passes 2,860/2,860 and Phase 7 passes 2,996/2,996. Inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows; all 817 reconstruction-tool tests pass. Both public Physics header trees match their pinned manifest. The full DLL reconstruction remains open.

### Task 16 — candidate mapping for the `NpScene` constructor (`phys_fn_000285`)

IDA identifies RVA `0xC310` as the 216-byte `NpScene` `__thiscall` constructor taking an `NxSceneInternal*`, returning `this`, and cleaning the stack with `ret 4`. The direct linker-map symbol is `??0NpScene@@QAE@PAVNxSceneInternal@@@Z` at `0x10041b70` in `NpScene.obj`; candidate source is `Physics/src/NpScene.cpp#NpScene::NpScene`. The constructor is reached by the internal Scene constructor and installs the 0x28-byte public wrapper at `Scene.cpp:791`.

The existing `NxPhysicsSceneConstructorTests` differential verifies the internal Scene back-link at wrapper `+0x24`, and a targeted source mutation of that store was previously caught. This task records the missing source/map link and the IDA allocation/layout contract in the inventory. Full semantic coverage of the nested lock and worker-condition branches remains open.

Status: complete. The candidate source/map link is recorded and inventory validation remains at 6,338 functions, 5,138 data objects, and zero unexplained rows. The full DLL reconstruction remains open.

### Task 17 — NpScene run/timing source/map ledger links (`phys_fn_000333`–`000342`)

IDA confirms the six oracle routines and their call/lock behavior, and the fresh Release linker map resolves each candidate symbol into `NpScene.obj`. Record source/map links and the IDA ranges for `flushStream`, `startRun`, `finishRun`, `setTiming`, `getTiming`, and `runFor`; retain the already registered row-specific dynamic differentials as the behavioral evidence. This is a mapping/provenance task, not new semantic closure. Public headers remain unchanged.

Status: complete. Inventory rows `000333`, `000335`, `000336`, `000338`, `000340`, and `000342` now point to `Physics/src/NpScene.cpp` and their verified `NpScene.obj` map entries. See `docs/reconstruction/novodex-physics/evidence/phase7-npscene-run-timing-map-000333-000342.md`; existing behavior proofs remain linked from the row entries. Full DLL reconstruction remains open.

### Task 18 — NpScene pair-query source/map links (`phys_fn_000317` / `000319`)

The regenerated backlog selects the pair count and pair-flag array wrappers. Their row-specific behavior is already mutation-falsified by `NxPhysicsPairFlagTests`; complete the missing candidate source field and refresh the map addresses from a current CMake Release build. Keep the existing behavioral differential as proof and do not claim new semantic closure. Public headers remain unchanged.

Status: complete. Both rows now identify `Physics/src/NpScene.cpp`; the fresh map resolves `?getNbPairs@NpScene@@UBEIXZ` at `0x10042a90` and `?getPairFlagArray@NpScene@@UBE_NPAUNxPairFlag@@I@Z` at `0x10042bb0` in `NpScene.obj`. Evidence and prior mutation results are in `docs/reconstruction/novodex-physics/evidence/phase7-npscene-pair-query-wrappers-000317-000319.md`. Full DLL reconstruction remains open.

### Task 19 — NpScene callback source/map links (`phys_fn_000352`–`000364`)

The next generated packet consists of the read-locked notify, trigger, and contact callback getters plus the disabled FluidModel callback setter/getter. Existing row-specific mutation proofs cover their behavior; fill the missing candidate implementation paths and refresh the map addresses from a current CMake Release build. Keep behavior status and public headers unchanged.

Status: complete. Rows `000352`, `000356`, `000360`, `000362`, and `000364` now point to `Physics/src/NpScene.cpp` and verified `NpScene.obj` symbols. See `docs/reconstruction/novodex-physics/evidence/phase7-npscene-callback-mappings-000352-000364.md` and existing mutation evidence. Full DLL reconstruction remains open.
