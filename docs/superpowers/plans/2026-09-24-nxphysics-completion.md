# NxPhysics Full Reconstruction Completion Plan

> **For agentic workers:** Use `superpowers:executing-plans` for bounded execution packets. Use `superpowers:subagent-driven-development` when parallel agent execution has been selected. Steps use checkbox syntax for tracking. This document is the program-level completion plan; recover each packet's machine-level contract before writing its implementation.

**Goal:** Finish a source-built, semantically and ABI compatible reconstruction of the entire pinned Win32 `NxPhysics.dll`, including rarely used paths, while preserving every public header and the existing CMake build.

**Architecture:** Continue the existing stable-ID census and Foundation-style C++ implementation. Execute dependency-ordered slices across the historical phases, connecting recovered internals to the real DLL's public interfaces and proving each slice against the shipped binary. Keep phase ownership stable unless evidence corrects it; distinguish passing a partial phase gate from completing the DLL.

**Tech stack:** CMake, MSVC Win32 Release, existing C++ public headers, reconstructed NxFoundation, pinned Ghidra 12.1.2 and Capstone 5.0.6, Python evidence tooling, PowerShell isolated-process runners. Use IDA or angr selectively when they answer a specific unresolved question.

**Status:** Execution started on 2026-09-24 in the isolated `codex/nxphysics-completion` worktree. The user confirmed full-DLL scope. M0 census corrections and the first M3 numeric correction are in progress; no milestone is complete.

**Execution note, 2026-09-24:** The three known OPCODE artifact misclassifications have been corrected in the working inventory. A local `Segment::SquareDistance` overlay now passes both registered oracle families exactly (100,000 compared words); the rebuilt DLL retains that symbol and `qh_pointdist` through explicit link options. The current DLL remains a partial reconstruction. A whole-archive trial failed on stock `SweepAndPrune::Init` because the oracle uses NovodeX helper `phys_fn_004816` at `0x000b4530` instead of the absent `OPC_BoxPruning.cpp` dependency. See `docs/reconstruction/novodex-physics/evidence/phase4-falsification.md` §10. Continue with helper recovery, archive linkage, actual-DLL driving, and the rest of M0/M1 before advancing closure.

**Gate note, 2026-09-24:** Both PowerShell runners now accept explicit repository, build, oracle, and pair-output roots. In the isolated worktree, the Phase 4 gate passes with 100 of 100 coverage assertions and the Phase 2 staged-pair gate passes. A Phase 4-only process previously printed `status=pass` but returned the differential runner's intentional skip code 3; the gate now returns 0 on successful completion.

**Pruning/AABB note, 2026-09-24:** Recovered the oracle's `0x000b4530` NovodeX complete-box-pruning helper and its `0x000b46b0` continuation, then switched `NxOpcode` and its consumers to the oracle's min/max AABB representation. A fresh whole-archive Win32 DLL link now succeeds. The new direct-link helper differential is exact on 730 oracle words and separately falsifies the sorting and pair-output portions; Phase 2, 3 and 4 gates pass, with Phase 4 at 101 of 101 coverage assertions. These are intermediate closures: actual-DLL public routing, lifecycle behavior, and the remaining census remain open. The earlier whole-archive failure recorded above is historical and has been resolved. See `docs/reconstruction/novodex-physics/evidence/phase4-falsification.md` §11.

**Phase 5 reproduction, 2026-09-24:** On the current whole-archive Win32 build, the Phase 5 gate runs to completion and reports `layout candidate mismatches=1`, then exits 1 as designed. The single current failure is an explicit `candidate CANDIDATE-MISSING family=vtables reason=shape finals/actor classes are Tasks 3-4` line in `tests/PhysicsObjectLayoutTests.cpp`; it is not the earlier three-failure crash described by historical notes. All printed candidate provisional families before that line report `failures=0`. The next object-model packet should drive real final shape/actor vtables through the candidate DLL and replace this synthetic missing marker only once the relevant behavior is tested.

**Current gate matrix, 2026-09-24:** Phases 2, 3, 4, 6, and 7 pass from the isolated worktree. Phase 5 remains intentionally red on the vtable marker above. These are intermediate phase gates with substantial deferred census rows, not a claim that the DLL is complete.

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
| Phase 2: SDK/core | 143 | 57 | 30 | 50 | 6 |
| Phase 3: geometry/collision | 392 | 257 | 74 | 61 | 0 |
| Phase 4: assets | 1,050 | 916 | 108 | 26 | 0 |
| Phase 5: objects | 205 | 85 | 120 | 0 | 0 |
| Phase 6: joints/effectors | 433 | 302 | 129 | 2 | 0 |
| Phase 7: scenes/simulation | 561 | 369 | 188 | 4 | 0 |
| **Total** | **2,784** | **1,986** | **649** | **143** | **6** |

The other 3,554 function rows are currently classified compiler artifacts. All 5,138 data objects are currently classified. The full audit must validate those classifications; three known exceptions below already show why classification is not unquestionable. No code row is yet at the final `closed` state. The earlier closure ledgers recognize 149 rows at intermediate proof levels, with final closure reserved for Phase 8.

Important qualifications and concrete gaps:

- The 1,986 discovered rows are not 1,986 functions necessarily needing fresh handwritten implementations. The Phase 4 ledger records 722 vendored qhull/OPCODE rows, including 633 code rows lacking row-specific falsification. Reuse their source and prove correspondence.
- `NpActor.cpp`, `NpScene.cpp`, and `NpJoint.cpp` contain respectively 84, 63, and 26 explicit `(unimplemented)` markers. This is a source triage count, not a substitute for a complete API audit. `NpActorVtable::isDynamic()` currently always returns true; scene `simulate` and `fetchResults` are placeholders.
- `NpPhysicsSDK.cpp` still has unfinished scene release/enumeration, triangle-mesh factory/release, actor-group pair, and core-dump paths.
- The latest Phase 5 narrative records a stable failing layout gate with `batch3268 candidate failures=3`. It also records failed attempts to move the large test frame to other storage. Reproduce and diagnose this; do not declare a new cause from historical guesses.
- `phys_fn_005493` (`Segment::SquareDistance`), `phys_fn_005517` (`AABBTreeNode::_BuildHierarchy`), and `phys_fn_005523` (`AABBTree::Build`) are still classified as artifacts despite the Phase 4 evidence showing product relevance. The distance function has recorded differential discrepancies.
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

- [ ] Preserve unrelated working-tree changes. Record the execution-start commit and use an isolated implementation worktree when work begins. The current untracked `docs/novodex-foundation/` is user-owned input, not permission to stage the whole directory.
- [ ] Ensure every required Foundation evidence manifest and pinned third-party input has a reproducible location; clean-checkout verification must not silently depend on untracked files or incidental build caches.
- [ ] Reproduce the current gates, preserving failures and skips. Record them independently of historical `pass` fields.
- [ ] Build a machine-generated backlog for every code row: implementation/linkage, public reachability, unresolved dependencies, applicable contexts, evidence strength, and next packet. Count functions and bytes separately.
- [ ] Re-evaluate the three named artifact misclassifications against their recorded callers and source correspondence. Correct kind/ownership/proofs consistently; do not bulk-reclassify other artifacts by analogy.
- [ ] Audit referenced data types, tables, and constants. Keep justified `classified` states; reopen a classification only with a concrete inconsistency.
- [ ] Make reporting separate intermediate gate coverage, full code closure, artifact classification, and data classification. Add validator cases rejecting hidden deferrals and unsupported final promotion.

**Exit:** every remaining code row has an actionable packet; totals reconcile; the known classification errors are resolved from evidence; no historical partial pass is reported as completion.

### M1 — Make the verification harness reliable and prove real DLL coverage

**Files:** `tests/PhysicsObjectLayoutTests.cpp`, `tests/PhysicsAssetTests.cpp`, `tests/PhysicsPairLoader.h`, `CMakeLists.txt`, `tools/{gate_targets.ps1,run_phase_gate.ps1,run_differential.ps1}`. Proposed focused tests: `tests/PhysicsObjectLifecycleTests.cpp` and `tests/PhysicsSceneLifecycleTests.cpp`.

- [ ] Reproduce the Phase 5 failure from a fresh build; capture the first failing case, instruction, call target, register/stack state, and the fixture initialization that reaches it.
- [ ] Separate actual candidate discrepancies from harness defects. Split new lifecycle fixtures into small functions/translation units with explicit ownership. Do not move the entire historical test frame blindly or weaken coverage to turn it green.
- [ ] Validate internal call adapters against the oracle calling convention, stack balance, nonvolatile registers, and structure-return ABI. Restore stack-protection checks for repaired fixture code; do not broaden `/GS-` as a workaround.
- [ ] Add actual-DLL SDK/scene/actor construction and destruction tests, then route each subsequent public feature through them. Audit that candidate symbol resolution uses candidate symbols/maps, never oracle RVAs applied to the candidate.
- [ ] Extend the runners to accept repository, build, oracle, and output roots so isolated worktrees run their own binaries. Preserve defaults for existing usage. Missing cases/targets, skipped phases, timeouts, and crashes must remain failures or explicit skips.
- [ ] Pin oracle-only input/output digests and case counts independently of candidate output. Persist machine-readable proof artifacts with hashes rather than relying on matching numbers in prose.

**Exit:** the reproduced layout failures are explained and fixed without suppressing cases; small lifecycle tests are stable; mutations to the built DLL's dispatch or behavior fail the public tests.

### M2 — Complete SDK, scene ownership, actors, bodies, shapes, and materials

**Files:** `Physics/src/{PhysicsSDK.cpp,NpPhysicsSDK.cpp,Scene.cpp,NpScene.cpp,ObjectModel.cpp,NpActor.cpp}`, their headers under `Physics/src/include/`, `object_model.json`, and lifecycle tests introduced in M1.

- [ ] Resolve the actor secondary base at `+8`, actor slots 63/64, third-pose semantics, descriptor-vtable identities, and shape-tail fields listed as open in `object_model.json`.
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
- [ ] Correct the recorded `Segment::SquareDistance` behavior and prove hierarchy-building modifications after reclassifying their rows in M0.
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
- [ ] Exercise a minimal public-SDK consumer and an Unreal scenario with actual physics activity. Stage an isolated consumer copy where practical; any live replacement must use exact backup/hash checks and guaranteed restoration. A load-only smoke test cannot close this milestone.
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
