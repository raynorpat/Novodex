# NxPhysics completion project plan from main

Date: 2026-09-30
Baseline: `main` at `b942f01` (Merge convex-mesh gap reconstruction into main)
Status: Active execution. The user selected full-DLL reconstruction and standalone simulation tests before Unreal integration. Phases 4 and 5 pass at 249/249 and 2,037/2,037 assertions. The candidate passes the standalone simulation differential and runs `DemoGame.exe Physics.war -windowed -benchmark -seconds=6` to a normal exit. Public triangle-mesh PMap attachment, load/reject behavior, and serialization/accessors match the oracle; the deterministic resolution-32 tetrahedron PMap computation also matches exact size and hash. An isolated resolution-64 tetrahedron probe exposes an open candidate mismatch (74,576 bytes versus 74,563 on the oracle; 178 decoded face labels differ). A raw-grid capture narrows this to equidistant face labels, with matching candidate/oracle face preorder; the oracle's point-distance arithmetic remains to be matched. Broader PMap topology/resolution coverage, alternate creation arms, qhull/mesh output fidelity, full-DLL closure, and broader Unreal coverage remain open.

## Outcome and constraints

Finish reconstructing the complete pinned Win32 NxPhysics.dll, including functionality Unreal rarely uses. Build it and NxFoundation.dll through the existing CMake project. Preserve all public Physics headers and the Foundation ABI. Product code must be independent of the original DLL; the original is an oracle for analysis and separate-process tests only.

The user selected **standalone simulation tests first, then Unreal**. Begin useful testing as soon as the real simulation path works. Early testing is an intermediate milestone; it does not reduce the full-DLL scope or authorize declaring unfinished subsystems complete.

This roadmap supersedes the scheduling assumptions of the September 24 completion plan. Existing contracts, recovered source, oracle listings, and proofs remain inputs. Do not repeat already completed NpActor, joint, scene-raycast, qhull, or convex/mesh reconstruction just because an older checklist is unchecked.

## 1. Verified baseline

The assessment used the current source and inventory on main, not the older `codex/nxphysics-completion` worktree. Untracked user files were left alone.

| Item | Current evidence | Planning consequence |
|---|---|---|
| CMake Win32 Release DLL build | Fresh invocation of `cmake --build build --config Release --target NxPhysics` succeeds; existing build cache uses Visual Studio 18 2026 / Win32 | Preserve the working build; avoid build-system replacement |
| Public Physics headers | Manifest verification passes for all 80 files | Keep the hash check mandatory |
| Phase 5 | The original baseline passed 13 staged-pair targets (2,037/2,037 assertions), while the object-layout target emitted `CANDIDATE-MISSING family=vtables`. That marker has since been replaced with executable checks; the fresh Phase 5 gate now passes at 2,037/2,037. | Keep Phase 5 green while continuing the broader vtable, ownership, and whole-DLL audit. The gate repair is not evidence that all actor virtuals or every object family is complete. |
| Inventory validation | Pass; 6,338 function records, 5,138 data records, zero unexplained rows | This proves ledger consistency, not completion |
| Executable code census | 2,787 code rows / 938,498 bytes: 749 discovered, 1,887 reconstructed, 145 dynamically gated, 6 statically reviewed | Separate missing implementation from verification debt |
| Discovered code | 218,282 bytes across 749 unique IDs | This is an audit queue, not a claim that every byte is unwritten |
| Data | All 5,138 records are classified | Prove candidate ownership and relocation for required tables/globals; classification alone is insufficient |
| Work-unit map | Committed map has 143 records, 31 duplicate names and 2,023 multiply assigned IDs; regeneration in `build/main-planning-work-units.json` produces 109 units, 60 named and 49 gaps | Repair generated scheduling data before assigning work |
| Scene simulation API | The real public simulate/check/fetch path, worker lifecycle, gravity/timing/writability, legacy run wrappers, contacts, all three broadphase selectors, forces, sleep/wake, bounded scenes, and tested joint routes have standalone oracle coverage. The registered corpus includes a 1,000-step soak and exact collision/joint fixtures. | T1 is verified for the current fixture corpus. Extend branch coverage during later work; the tested subset does not prove all scene or solver behavior. |
| Phase 4 | The main-branch baseline had a qhull OBJ tape crash. The harness path has since been repaired; the current worktree's fresh Phase 4 gate passes 249/249 coverage assertions, with both asset and third-party differentials exiting successfully. | Keep the gate green while expanding mesh/PMap coverage; a green Phase 4 gate does not close all of M4. |
| Final gate | Phase 8 has no registered test targets and coverage floor zero | A separate whole-DLL acceptance gate must be built |

The original current-main artifacts are `build/main-phase5.log` and `build/main-phase7-with-simulation.log`; they record the pre-repair state. Subsequent work added the real public scene step/result path, so the old `stdout_delta=30` stationary-candidate failure is superseded by the checkpoints below. The explicit-friction differential was the last Phase 7 simulation failure and now passes on the current worktree revision; the full-DLL matrix remains open. Other phase results must be tied to their current logs and source revision. Older branch reports have different coverage floors and must not be presented as current-main verification.

### Execution checkpoint — 2026-09-30

- The explicit Phase 5 red marker has been removed after the object-layout gate gained executable candidate checks. The fresh gate against this worktree passes with 2,037/2,037 recorded assertions (`build/phase5-worktree-final.log`). This closes the stale red gate, while the roadmap's broader actor/shape vtable and whole-DLL audits remain open.
- The real standalone scene step now runs the existing collision/contact path and settles the dynamic sphere on the static plane. The 1,000-step result matches the oracle exactly at `y=0.45`, zero velocity.
- A two-dynamic-sphere impact fixture now reaches the public simulate/check/fetch path through contact generation and the kind-0 support solver row. After correcting the x87 precision boundary from the oracle listing, the complete fixture output matches (`stdout_delta=0`). Its first impact frame is pinned in Phase 7 coverage.
- A no-gravity conventional-force fixture now reaches the same public step path and matches the oracle exactly after one 0.125-second step (`position.x=3d747645`, `velocity.x=3ef47645`). Its result line is registered in Phase 7 coverage.
- The worker path now covers both nonblocking outcomes: before any submitted work, `checkResults(false)` and `fetchResults(false)` return false; after blocking until an empty step completes, both nonblocking calls return true. Oracle and candidate outputs match exactly, and both result lines are registered in Phase 7 coverage.
- A kinematic body driven with `moveGlobalPosition` reaches its target through the real variable-step path and has zero velocity after the step, matching the oracle. A stationary body also expires its wake counter and sleeps after 30 fixed steps, then wakes through `wakeUp(0.5)`; both states match the oracle.
- The explicit-friction sphere/plane probe now matches the oracle through all eight steps. The first remaining issue was the solver-to-patch callback: candidate pre-divided the impulse and supplied the iteration number where the oracle supplied the timestep, adding an unintended float rounding. After matching that call contract, the normal and friction support rows now scale the signed impulse by inverse mass before projecting along the constraint direction, matching the oracle's x87 operation order.
- A three-body stack regression now runs 120 fixed steps through the public scene/contact/solver path. Its first divergence had been at step 4: the oracle reduced the gravity increment for bodies with multiple contact pairs, while candidate `BodyStep.cpp` read an unrelated hard-coded zero. An oracle debugger write watch showed SDK initialization copies the live parameter defaults into that data region, and the corresponding adaptive-force parameter is `1.0` in the running oracle. `row000726` now reads the live SDK parameter instead of a detached zero; the full simulation fixture matches exactly (`stdout_delta=0`, `stderr_exact=True`) in `build/stack-fixed-differential.log`.
- After the stack fix, the fresh Phase 7 gate passed all eight targets at 1,165/1,165 assertions (`build/phase7-stack-fixed.log`), Phase 5 passed 2,037/2,037 (`build/phase5-after-stack-fix.log`), the CMake Release build succeeded, and the reconstruction tooling suite passed all 770 tests. The current fixed-joint follow-up also passes fresh Phase 5 and Phase 7 runs, and the full standalone simulation target matches. No public Physics headers changed. This closes the current stack mismatch, not the full M1 matrix. Repeated scene/SDK lifecycle cases are still outstanding. Solver kinds 1/2/3/6 now have a provisional 004397 path; kind 5's 004399 and broader per-kind fixture coverage remain open. Full-DLL completion remains unchanged.
- The fixed-joint probe initially exposed a scene lifecycle defect: `Scene::simulate()` omitted the second per-joint `row000720` pass and did not reset the used-joint array end (`Scene+0x590`) to its start (`Scene+0x58c`). On the next fixed frame, `row000762` linked the joint to itself, and a worker spun while traversing that list. Both operations are now restored from oracle row `000635`, and all 12 fixed steps complete. Raw oracle disassembly shows `FixedJoint::row_slot6` does not set `mFlags` bit 2; the speculative write was removed. The trial kind-1/2/3/6 handler also rounded the `004397` impulse too early. Moving the float store to the recovered accumulated-impulse boundary made the complete standalone simulation differential green again (`stdout_delta=0`, `stderr_exact=True`; `build/fixed-joint-retest2.log`). This validates the fixed-joint fixture and its tested kind-3 route, not every solver kind or the full M1 matrix; keep the handler marked provisional until broader fixtures cover kinds 1, 2, and 6.
- The fresh Phase 7 run after this change passes all eight targets (`build/phase7-after-fixed-gate-serial.log`); Phase 5 passes all 13 (`build/phase5-after-fixed-gate-serial.log`). The 770-test tooling suite passes in 212.8 seconds. These results begin T1's requested standalone testing and keep Phase 5 green; repeated lifecycle cases and kinds 1/2/6 coverage remain open.
- The first T2 launcher probe is blocked at baseline, before candidate deployment. In the installed oracle build, hidden `DemoGame.exe Physics.war -windowed -benchmark -seconds=5 -nosound` and `RubeGoldbergExample.war` hang after engine initialization; adding `-unattended -nopause` or running the Physics map in a normal visible window exits with code 3 and no useful error text or clean-shutdown line. The visible run confirmed both pinned oracle modules loaded from `Binaries`. Only probe-launched processes were stopped; no DLL was replaced, and both installed DLL hashes still match the pinned oracle. Identify a reproducible launch path and clean oracle run before staging the candidate pair. Full-DLL completion remains unchanged.
- The default `001976` broadphase/contact refresh is now wired into each real simulation substep. Its static-plane settle, explicit-friction, 120-step three-body stack, and 40-step two-body impact fixtures match the pinned oracle exactly. Fresh checks on this worktree pass the Release build, Phase 3 (2 targets), Phase 5 (13), Phase 6 (6), and Phase 7 (8); the standalone simulation target has zero stdout delta and exact stderr. The full tooling suite passes 770 tests plus 690 subtests; inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained; all 80 public headers match the manifest. Phase 4 currently has no registered test targets, so its runner skips it. These results verify the tested mode-0/default-pruner route only: other pair-refresh selectors, the full joint solver dispatch, repeated lifecycle, callbacks, and mesh contact remain open.
- Mode 3 now uses a persistent `Opcode::SweepAndPrune` coherent cache. Oracle evidence in `ContactPlaneMesh.cpp__to__PenetrationMap.cpp.md` identifies the initialization, changed-box update, and pair-enumeration paths; pruning-engine add/remove and destruction invalidate or release the cached tree. A public simulation fixture moves one actor out of a three-body overlap chain and back in. Oracle and candidate pair counts are one and then two, and the full transcript matches (`stdout_delta=0`, `stderr_exact=True`) in `build/coherent-sap-differential.log`. Both lines are registered in Phase 7; its focused coverage-floor tests pass and fresh Phase 7 passes at 1,192/1,192 (`build/phase7-coherent-sap.log`).
- The coherent-cache result covers only the tested add/remove transitions. Bounded-scene behavior, multi-object update ordering, mode-1 traversal fidelity, pair callback/report lifecycle, and broader invalidation cases remain open. T1 and full-DLL completion remain open.
- The three selectors now also pass a dense four-body pair-order case (`0-1,0-2,0-3,1-2,1-3,2-3`) and bounded-scene contact coverage. Oracle and candidate both report two bounded-scene pairs for each selector. A red differential exposed dynamic actors being registered into hard-coded pool 2 when the bounded descriptor selects pool 1; registration now follows the engine's descriptor-selected pool and creates its 0x40-byte bounded dynamic pruner. The Phase 7 floor is 1,201 and the fresh gate passes. Bounded-pruner tree-backed query slots are still incomplete, so this closes only the exercised simulation registration/contact route.
- The `004397` joint solver callback now matches the oracle vtable dispatch (slot 3 for final accumulated-force updates), and its force accumulator uses the separate ±maxForce clamp and slot-2 break check shown in the listing. The distinct kind-5 `004399` solver has been transcribed and wired for both iterative and final-record passes; inventory state is `reconstructed` on static listing evidence, with dynamic kind-5 coverage still open. The 12-step kind-1 distance-joint fixture exposed a linear impulse rounding-order mismatch; `supportApplyJointImpulse004395` now rounds the projected impulse before inverse-mass multiplication, following the oracle listing. After a fresh CMake rebuild, all 12 distance states match exactly and the fixture is registered in Phase 7. Fresh Phase 5 (13 targets) and Phase 7 (8 targets) pass; the simulation target has `stdout_delta=0`, and the Phase 7 floor is 1,214. Focused coverage registry checks pass. The low-break-force fixed-joint response and broader per-kind matrix remain open, so this does not close M1.
- On 2026-10-01, fresh Phase 5 (13 targets), Phase 6, and Phase 7 (8 targets) pass (`build/phase5-after-probe-removal.log`, `build/phase6-after-probe-removal.log`, `build/phase7-final-current.log`); the registered simulation target is exact (`stdout_delta=0`, `stderr_exact=True`). The four-step low-force fixed-joint probe still produces `stdout_delta=6`; it stays outside the shared gate. Oracle disassembly confirms 004395 is a record-thiscall with impulse and timestep stack arguments (`ret 8`); the candidate uses that ABI and reverse-column inertia accumulation. An attempted change to keep angular matrix sums unrounded increased the mismatch to 28 lines and was reverted. A separate 12-step revolute pendulum probe produced `stdout_delta=20` (`build/revolute-sim-differential.log`). A fresh all-kind trace found 384 matching solver-row entries and matching post-helper body states; all 77 traced slot-3 accumulated-force callback outputs also match, including row accumulators, body linear/angular velocity, and the joint accumulator at +0x154. Those traces were preliminary; the subsequent current-worktree trace isolated a missing solver-wrapper callback.
- The coherent-broadphase moved-apart/rejoin regression then exposed a kind-0 contact row rounding mismatch in body 2's X velocity. Oracle and candidate row inputs matched, but `phys_fn_004403` projects the signed impulse before applying inverse mass for body 2. `JointSupport.cpp` now preserves that operation order, and the regression reads the internal body record so it cannot pass through a public getter that returns stale state. The exact expected velocity bits match after the fix. Fresh Phase 5 and 6 gates pass at 2,037/2,037 and 856/856 (`build/phase5-after-contact-fix.log`, `build/phase6-after-contact-fix.log`); Phase 7 passes all eight targets at 1,218/1,218 with zero differential output (`build/phase7-after-contact-fix.log`). The full tooling suite then passed 770 tests in 214.2 seconds; header verification passed all 80 files, inventory validation passed with 6,338 functions, 5,138 data objects, and zero unexplained records, and `git diff --check` passed. The low-force fixed-joint response (`stdout_delta=6`) remains an unresolved standalone case; the revolute mismatch is resolved below. Public Physics headers remain untouched.
- The 12-step public-API revolute fixture reproduced the former `stdout_delta=20` mismatch. Current paired debugger traces matched through ordinary solver rows and localized the first difference to final-pass kind 6 handling: oracle `004174` clears the row target and dispatches joint vtable slot 1 before the kind 6 solve, while the candidate skipped that reset. `JointSupport.cpp` now performs the slot 1 callback at the corresponding point. The fixture now matches across all 12 steps (`stdout_delta=0`, `stderr_exact=True`); fresh Phase 6 and Phase 7 gates pass at 856/856 and 1,218/1,218 (`build/gate-phase6-kind6-reset.log`, `build/gate-phase7-kind6-reset.log`). Full solver-kind interaction coverage, the low-force fixed-joint case, and full-DLL completion remain open.
- The low-force fixed-joint mismatch is now resolved and registered in the Phase 7 simulation corpus. Oracle tracing identified the missing 000577 break-event queue drain in fetchResults; the candidate now dispatches 004113, applies the 004105 remove-before-clear path, and then completes finishSimulation in oracle order. The focused fixture and all eight Phase 7 targets pass with exact output, and all 13 Phase 5 targets remain green (`build/break-event-exact-detach.log`, `build/phase7-break-event-drain-registered.log`, `build/phase5-after-break-fix.log`). The nine registered break observations raise the Phase 7 floor to 1,227. This closes the tested default-notify break route, not all joint break callback outcomes or the broader solver-kind matrix. The regenerated scheduling map at `build/current-work-units.json` has 109 units (60 named, 49 gaps). Remaining high-priority clusters include OPCODE mesh interface (58 rows/40,204 bytes), Controller-to-Fluid (91/29,058), PlaneMesh-to-PenetrationMap (43/20,213), TriangleMesh-to-Controller (24/16,739), and Fluid-to-FluidManager (46/14,051). Continue M1 with solver-kind interactions and repeated lifecycle, then close the triangle-mesh object/load prerequisites before claiming mesh-contact coverage. Public Physics headers remain untouched.
- Current-worktree checkpoint: after adding solver-wrapper and static-contact coverage, a clean Release rebuild passes. `NxPhysicsSimulationTests` matches the oracle exactly across the registered 1,000-step soak, static ground, force, impact, broadphase-selector, and tested joint/break-event fixtures (`build/simulation-clean-diff.log`). Fresh canonical Phase 2, 3, 5, 6, and 7 gates pass (`build/phase2-final.log`, `build/phase3-final.log`, `build/phase5-full-gate.log`, `build/phase6-full-gate.log`, `build/phase7-full-gate.log`); Phase 3 is 359/359, Phase 5 is 2,037/2,037, Phase 6 is 856/856, and Phase 7 is 1,227/1,227. The all-target CMake Release build passes (`build/all-targets-after-cmake-fix.log`), and direct-source test targets now list their real step/pruner dependencies. No public Physics headers changed.
- Phase 4 follow-up on 2026-10-01: bounded the self-only hull tape loops explicitly, compiled the real `TriangleMesh` implementation into the asset and third-party test targets, and repaired the mesh-writer fixture to initialize the decorated allocator export from the loaded Foundation DLL while detaching its borrowed stack data before destruction. The asset oracle differential now passes with zero mismatches; the third-party oracle differential exits successfully with zero candidate mismatches and zero layout failures. The phase runner still reports red because its recorded coverage assertion requires `qhull_exact_output` (oracle digest `4f7bae0d`), which the test no longer emits; observed output includes `qhull_output` instead (`build/phase4-after-allocator-fix.log`). This coverage-registration mismatch is the remaining Phase 4 gate defect, rather than a crash or differential mismatch. Phase 5 independently passes 2,037/2,037 coverage assertions (`build/phase5-before-commit.log`).
- T1's agreed entry criterion is met for the current registered standalone corpus, so T2 is next. First recover a reproducible clean oracle launch and shutdown in the installed Unreal build. Prior oracle-only probes hung after initialization or exited with code 3 and no diagnostic; those do not provide a usable baseline. Do not stage the candidate pair into the installed game until this baseline is reliable. Full-DLL reconstruction remains open.

Current unique code-row states by owning phase:

| Phase | Discovered | Reconstructed | Dynamically gated / statically reviewed |
|---|---:|---:|---:|
| 2: SDK | 21 | 66 | 50 / 6 |
| 3: collision | 144 | 187 | 61 / 0 |
| 4: meshes/spatial/vendor | 273 | 752 | 28 / 0 |
| 5: objects | 8 | 197 | 0 / 0 |
| 6: joints/effectors | 27 | 404 | 2 / 0 |
| 7: scenes/simulation | 276 | 281 | 4 / 0 |

The eight discovered Phase 5 IDs are `000002`, `000030`, `000032`, `000034`, `002318`, `002324`, `002330`, and `002421`. Some already have candidate bodies, including `000030` and `000032`; audit their full paths and evidence before rewriting them. Phase 5 closure also requires verification of its reconstructed rows and concrete dispatch, not merely promoting these eight.

## 2. Approach selection

1. **Recommended: dependency-driven parallel reconstruction with continuous integration.** Trace the public simulation entries to their real internal callees, partition those dependencies into owned work units, and bring up an end-to-end simulation slice while independent full-DLL work proceeds. This minimizes time to useful testing and exposes integration defects early.
2. **Sequential phase closure.** Finish every Phase 5 proof, then every Phase 6 item, then Phase 7. Easier coordination, but delays testing behind work that does not block the simulation loop.
3. **Source transcription first, testing afterward.** Maximizes the apparent rate of reconstructed rows but postpones ABI, ownership, dispatcher, and solver failures. Existing component successes alongside empty simulation entries show why this is a poor fit for the requested outcome.

Use the existing translation-unit contracts and listing bundles. Ghidra or IDA recovers structure and candidate types; Capstone listings resolve calling convention, control flow, x87 order, and ambiguous decompiles. Use cdb for execution and ownership traces. Use angr only for a specific unresolved branch or reachability problem where it saves time. Do not rerun whole-image decompilation routinely.

## 3. Milestones and dependency order

### M0 — Establish an executable backlog and integration baseline

Deliverables: corrected work-unit map, a current-main baseline report, and one dependency backlog keyed by unique stable IDs.

- Regenerate the map with the existing tool; verify every executable row has exactly one owner and repair duplicate-output detection in the normal validator/test workflow.
- Classify all 749 discovered rows into missing, partial, implemented-but-unverified, proven vendor correspondence, or oracle-native unsupported/no-op. Locate existing candidate implementations and their callers before assigning new code.
- For reconstructed rows, record missing dispatch, unresolved callees, current-source proof, or known differential divergence separately. Avoid equating a reconstructed count with production readiness.
- Trace the public scene stepping entries into scheduling, broadphase, pair generation, contacts, islands, solver, integration, sleeping, and report delivery. Resolve indirect targets or explicitly list them as blockers. Numeric phase labels and adjacent source addresses are insufficient dependency evidence.
- Assign required data tables and globals to the same owners as their users, including vtables, dispatch matrices, allocator state, and FP control state.
- Run a clean CMake build, header check, inventory validator, tooling suite, and gates 2–7 once on a single main SHA. Record candidate hashes and exact failures. Require the Phase 5 gate to pass and retain every Phase 7 differential in the registered target set; the friction mismatch on the planning baseline has now been corrected and verified green.

Exit: every remaining ID and unresolved dependency has one owner and a test route; no duplicate work assignments; the first simulation dependency packet is ready. Timebox the initial scheduling pass to one work session and deepen contracts as each packet starts.

### M1 — Bring up the real standalone simulation path (critical path)

Primary code: `Physics/src/NpScene.cpp`, `Physics/src/Scene.cpp`, their private headers, and the actual solver/scheduler units identified by M0. Proposed test target: `NxPhysicsSimulationTests`.

- Reconstruct scene gravity, timing, writable/running state, lock and result semantics, and error paths from the oracle. Follow the oracle's relationship between old run APIs and newer simulate/fetch APIs rather than imposing a new engine design.
- Gravity and timing reads/writes have been implemented and pinned at bit level; keep this verified slice while completing the remaining scene methods.
- The initial current-main baseline had false result flags and a stationary candidate; the real scene step/result path has since been wired and now settles and collides through contact/support rows. The explicit-friction divergence is resolved. Keep filling the standalone test matrix before declaring T1 complete.
- `NxSceneDesc::broadPhase` maps the public selectors to pruning-engine modes 1/2/3. Mode 2 uses full sweep-and-prune; mode 3 uses the persistent coherent `SweepAndPrune` cache. Differential fixtures pin selector mapping, pair membership, bounded/unbounded registration, dense and chain ordering, and mode-3 move-out/move-back updates. Bounded-pruner tree-backed queries, broader update ordering, and pair/report lifecycle are still required before claiming selector behavior complete.
- Wire the recovered body state, forces, and joint implementations through the real stepping path. Complete missing solver and integration callees as a dependency cluster; do not create a substitute Euler integrator just to pass a falling-box test.
- Build independent oracle and candidate processes from identical serialized fixtures. Compare per-step poses, velocities, forces, wake state, result status, callbacks, and allocation/lifetime events.
- Start with an empty scene and one body under gravity/force, then static contact, two-body collision, kinematic interaction, sleep/wake, a small stack, and a jointed pair. Include variable step sizes and the oracle's FP control-word transitions.
- Cover each `NxSceneDesc::broadPhase` selector with bounded and unbounded scene descriptors, then prove the selected traversal emits the oracle's candidate pairs and contact outcomes.
- Exercise at least 1,000 steps for basic stable fixtures, repeated scene/SDK lifecycle, and both blocking and nonblocking result calls. Expand cases to reach every new branch before claiming its closure.

Exit / **T1: standalone testing begins**: the candidate runs the genuine public step/result path through collision and solver code; the above fixtures execute without stubs, crashes, hangs, or unexplained mismatches. Any longer numerical divergence has a measured first divergent step and remains an open defect. This is not whole-DLL completion.

### M2 — Close object, mesh, and pruning ownership in parallel

Primary code: `NpActor.cpp`, `Scene.cpp`, `ObjectModel.cpp`, `TriangleMesh.cpp`, shape and pruning units. Coordinate edits to `Scene.cpp` through its owner.

- Audit the eight Phase 5 discovered rows and the real factory-to-wrapper-to-final-vtable paths for every shape family, static/dynamic bodies, meshes, compounds, controllers, and release/rollback.
- Phase 5 now passes its recorded gate. Continue the broader object audit: compare `NpActorVtable` and shape tables against the pinned census (`phys_data_000678` and `phys_data_000679`), inventory all 87/88 entries, group inherited/common slots, and add oracle-backed execution where the gate does not yet drive a concrete path. `NxPhysicsShapeVtableTests` covers shape cases but does not replace the complete actor/object-family contract.
- Replace remaining parameterized stand-ins or incomplete constructors on product paths with recovered implementations. Verify all vtable slots against the pinned oracle, including deleting destructors, base adjustments, and return ABI.
- Complete mesh loading/serialization and actual asset ownership where current tests assemble internal fixtures directly. Round-trip real asset data through the public API, not only synthetic internal objects.
- Drive dynamic shape mutation, mass recomputation, callbacks, shared ownership, allocation failure, and populated-scene teardown through the rebuilt DLL. Verify allocator choice and ordering.
- Regenerate the vtable census and attach current execution evidence. Replace the unconditional Phase 5 marker with executable dispatch/ownership assertions only when the whole family it represents is covered. A passing count or a deleted marker alone does not close Phase 5.

Exit: Phase 5 passes honestly, required object/mesh/pruner paths have no unknown dispatch, and M1's lifecycle and contact scenarios pass through these real objects. M1 and M2 exchange fixtures early; M1 does not wait for unrelated closure paperwork.

### M3 — Complete collision, spatial, vendor, and joint integration

- Finish the remaining collision/pruning/penetration-map clusters and joint/articulation/solver dependencies selected by the call graph. Prioritize anything M1 reaches or blocks on.
- Audit already vendored OPCODE/qhull correspondence before rewriting those rows. Preserve pinned upstream trees and use documented overlays for oracle-specific changes.
- Turn important direct-function comparisons into public-path scenarios: primitive/mesh contacts, mesh/mesh, compound shapes, filters/materials, continuous collision where present, raycast/overlap/culling, joint limits/motors/breakage, and effectors during simulation.
- Review existing divergence ceilings. Record actual divergent inputs and causes; frozen nonzero ceilings are useful regression controls but are not proof of exact reconstruction. Resolve them or prove the difference is outside the oracle contract before final acceptance.

Exit: every recovered shape pair and joint family participates correctly in simulation; filters, contact streams, callbacks, and spatial queries agree with the oracle for the recorded corpus.

### M4 — Complete the remaining full-DLL surface

Run independently of the rigid-body critical path once shared lifecycle contracts are stable.

**Immediate Unreal-blocking packet (promote ahead of unrelated M4 work):** the DemoGame `Physics.war` probe proves its KActor convex elements enter the missing `NX_SHAPE_MESH` route. Implement in this dependency order, reusing the recovered hull core and collision kernels:

1. Keep extending `NxPhysicsTriangleMeshApiTests` across descriptor, actor, lifetime, and contact behavior; verify `000242 -> 000478 -> 002251 -> 002260 -> 002233` and the mapped hull output/internal-mesh dependencies. The current public probe cooks the eight cube points to 8 vertices / 12 triangles, round-trips the descriptor counts, creates the mesh shape actor, and matches mass/inertia. The 120-step plane settle now completes with zero velocity: oracle y is `3f73332a`, candidate y is `3f73332b`. The cooked coordinate geometry is equivalent but its vertex numbering differs under the known qhull `0x027f` divergence; keep that full-fidelity debt open. Preserve SDK release ownership checks.
2. Continue auditing the `NX_SHAPE_MESH` runtime family in actor factory `000032` (0xe8 bytes, `001379` constructor), including public shape handles, mesh references, mass walk, scene auxiliary registration, contact dispatch, and release/rollback. The plane/mesh pair now has a candidate implementation and the standalone smoke reaches it. Fixed the first Unreal crash in `nxContactCompoundPair`: compare each child's AABB with the other leaf shape's AABB, never ask a shape group for an aggregate AABB through oracle-null slot 9. A focused collision regression leaves that slot null and the group's world bounds stale; the collision differential passes.
3. T2's first consumer smoke is complete: staged the rebuilt pair in the engine Binaries directory, loaded `Physics.war`, ran the six-second DemoGame benchmark, and observed exit code 0 plus normal `appRequestExit(0)` shutdown. Restore and verify both installed oracle hashes after every candidate run. Extend Unreal coverage to transitions, rigid-body interactions, joints, mesh collision, queries, and any fluids/controllers the consumer uses; the first launch does not close the wider M4/M5 work.

This packet unblocks early Unreal testing but does not close all mesh work: non-convex triangle assets, serialization/round-trip, pmap, cooking failures, mutation/lifetime cases, and Phase 4/5 falsification remain in M2/M3/M4/M6.

- Reconstruct fluids, fluid emitters/managers, implicit meshes, controllers, notification/reporting, remaining scene configuration/statistics, visualization, and serialization/export paths present in this exact binary.
- Distinguish genuine oracle no-op/unsupported slots from reconstruction placeholders using the binary and behavior. Reproduce unsupported behavior when it is the actual contract; do not invent functionality based only on public header declarations.
- Test fluids/controllers through their public scene lifecycle, interactions, callbacks, and destruction. Include SDK/scene release while those objects exist.
- Audit remaining SDK exports, initialization/teardown, error and assertion behavior, allocator crossing, and late/uncommon paths.

Exit: every in-scope public and internal behavior has an implementation or evidence-backed oracle-native classification. No subsystem is excluded because Unreal rarely invokes it.

### M5 — Unreal integration and regression feedback

Begins after T1 and the actual consumer's required public paths are covered; it may overlap M3/M4. Proposed runner: `run_consumer_smoke.ps1`.

- Inventory imports and observed calls of the Unreal executable and its Novodex consumers under `D:\FlamingEnt__\Unreal_3`; identify a reproducible launch/map workflow and compatible mesh assets.
- Stage a recoverable test installation of the rebuilt Physics/Foundation pair, with recorded original/candidate hashes and automatic restoration. Keep the original oracle installation available for comparisons.
- Start with SDK initialization, map load, scene creation, one simulated scene, and shutdown. Extend to map transitions, rigid-body interactions, joints, mesh collision, queries, and fluids/controllers where the consumer uses them.
- Capture crashes, first state divergence, callback order, allocation problems, and performance regressions as owned reconstruction defects. Consumer coverage adds to the standalone corpus; it does not replace full-DLL verification.

Exit / **T2: Unreal testing begins**: reproducible load/simulate/unload runs succeed with the staged candidate pair, without fallback to the original Physics DLL. Testing then continues throughout remaining work.

### Execution checkpoint — 2026-10-01 (Phase 5 and initial DemoGame probe; resolved below)

- The installed oracle pair still runs `DemoGame.exe Physics.war -windowed -benchmark -seconds=6` to `appRequestExit(0)` and a closed log. This confirms the user's working setup outside the managed worktree; use DemoGame for the consumer milestone.
- The fresh Phase 5 gate now passes all 2,037/2,037 recorded assertions after reconciling the inventory's stale `ContactPlaneMesh.cpp` unresolved-path entry, removing unsupported implementation mappings from two still-discovered SDK wrapper rows, and adding link-only dependencies to the focused test harnesses. Both public-header checks passed for all 80 files. The phase gate still does not certify every vtable family.
- `NxPhysicsTriangleMeshApiTests` exits 0 against oracle and candidate. Creation, actor shape count, mass, inertia, and zero final velocity agree; settle y differs by one ULP (`3f73332a` oracle, `3f73332b` candidate). Candidate cooked vertex/triangle hashes differ because of the known qhull vertex-order divergence, even though the cube coordinates and triangles are equivalent. The qhull/mesh fidelity item remains open.
- The initial candidate DemoGame probe crashed with `0xc0000005` while loading `Physics.war`. CDB identified a null call at `shapeOwnerWorldAABB` (candidate RVA `0x4295e`): it dispatches through `[object.vtable + 0x24]`. The object is an NxShapeGroup whose slot 9 is null in both candidate and oracle. The corrected stack trace reaches this adapter from `NxShapeWorldBounds` (return RVA `0x2aff3`), called by the compound contact expander (return RVA `0xedb2`). The resolved cause and fix are recorded in the checkpoint below; no group slot was added.
- Candidate SHA256 at this probe: Physics `55E426DB7A787F809F4596F2C999A6166FD4AFA66A4016743594845FFE2320D8`, Foundation `9972A8E9F182C1A3C3ACF6E4BE364C176A83D0E0B0490D3825DE9DA493C81A11`. Installed originals were restored and re-hashed after the probes: Physics `4B7DB3E126735C576F79FE5666E6FA661DE9724B2A78808BB0924325AC79602C`, Foundation `7E0596E45AF2F1AB937A948100528E51C80DFD02B295BA678898081883AE0990`.

### Execution checkpoint — 2026-10-01 (compound dispatch fix and first candidate Unreal run)

- The null call came from `NxContactCompoundShape` entering `nxContactCompoundPair`, which passed the group to `NxShapeWorldBounds`; its stale group AABB then reached the group table's null slot 9 through the owner callback. The expander now measures each child against the other leaf shape's bounds. The group slot remains null, matching the oracle.
- The regression was observed red first (`matrix contact_compound ... wrong=1`) and then green (`wrong=0`). `NxPhysicsCollisionTests` passes. The standalone `NxPhysicsSimulationTests` differential passes with oracle and candidate stdout identical.
- A fresh Phase 5 run passes all 2,037 recorded assertions. The candidate DemoGame run exits 0 after loading Physics.war, running the six-second benchmark, and logging `appRequestExit(0)` / `Game engine shut down`.
- Candidate pair staged for that run: Physics `3f4aa48cd9aea87a89028d22822236fd643274e406497baf6142b476f62ff396`, Foundation `9972a8e9f182c1a3c3acf6e4be364c176a83d0e0b0490d3825de9da493c81a11`. Afterward, the installed pair was restored and hash-checked: Physics `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, Foundation `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`.

### M6 — Full reconstruction acceptance

Proposed artifacts: `run_final_verification.ps1`, registered Phase 8 targets, final source/data ownership report, reproducible build manifest, and handoff notes.

- Clean configure and build from the recorded source revision; verify all 41 exports, ordinals, calling conventions, public header hashes, concrete vtables, imports, Foundation linkage, and candidate-owned data relocations.
- Audit all 6,338 function records, distinguishing executable code from compiler/runtime/other classified records. Audit all 5,138 data records by ownership, alias, generated equivalent, or legitimate non-runtime classification. No unowned required code/data, unresolved indirect target, unimplemented product path, or silently deferred required row remains.
- Complete the original plan's closure/falsification requirements. A trace hit proves execution; a matching fixture proves that fixture; neither alone proves all semantics. Exercise mutation sensitivity at meaningful boundaries and use whole-body listing/relocation checks where appropriate.
- Run all phases 2–8, the complete tooling suite, standalone simulation corpus, resource stress/soak cases, and Unreal regression suite on the final pair. Final verification rejects absent/skipped required gates and unresolved divergence.
- Confirm the release package runs with the oracle unavailable, contains the CMake-built DLL pair, and includes hashes, build instructions, test commands, and remaining limitations (none that contradict full reconstruction).

Exit: the full objective is demonstrated on the final source/build. Green component gates or an Unreal launch alone do not satisfy this milestone.

## 4. Work allocation for speed

Suggested capacity assumption: four concurrent implementers, with one also owning integration. This is a proposed execution arrangement, not a dispatch performed by this planning task.

| Owner | Initial responsibility | Boundary |
|---|---|---|
| A: simulation/integration | M0 baseline and M1 public step-to-solver chain | Owns NpScene and scene orchestration; defines shared lifecycle contracts |
| B: objects/assets | M2 object finals, meshes, ownership, Phase 5 closure | Publishes narrow interfaces to A; coordinates Scene.cpp edits |
| C: collision/spatial | M3 missing collision/pruner/vendor clusters | Owns corresponding source units and oracle fixtures |
| D: remaining subsystems/verification | M4 fluids/controllers; harness and M5 preparation when dependencies block | Does not rewrite A's scheduler or shared ownership abstractions |

One integrator owns shared `inventory.json`, gate registries, closure ledgers, and generated work-unit output. Implementers deliver row-level evidence/registration patches, which the integrator reconciles against current main. This directly addresses the duplicated map and differing historical gate floors seen in the baseline.

Use one isolated worktree per owner, stable-ID ownership, small dependency-complete commits, and frequent integration. Do not assign the same shared file independently without a named merge owner. If capacity is lower, retain this order and reduce concurrency rather than weakening acceptance.

## 5. Execution rules that reduce wasted work

- **One packet per coherent call-graph/source cluster**, ordinarily about 5–12 KB of oracle code; split large anonymous gaps after caller/callee analysis. Avoid another sequence of unrelated one-method probes.
- **Audit before transcription.** Reuse existing bodies, listings, bundles, and fixtures. Promote evidence only after checking current source, not from old branch status.
- **Wire immediately.** A reconstructed function must be reached by the correct product caller, or its intentional standalone role must be documented. Do not postpone integration until a directory is complete.
- **Test at the right cadence.** Run the changed target and its direct dependency targets per edit; run the affected phase per packet; run gates 2–7 and tooling at integration checkpoints; clean builds at milestone boundaries. Repeat broader suites only after new changes or unresolved failures justify them.
- **Freeze fixtures and expectations from the oracle.** Register input coverage as well as results; prevent tests from passing through constant answers, empty paths, or skipped callbacks.
- **Avoid broad refactors.** Introduce private units only where required to own a recovered subsystem or isolate shared-file contention. Preserve public ABI and the existing CMake topology.
- **Measure time to verified behavior.** Track time in analysis, implementation, differential debugging, and integration separately. Count verified product-path rows and passing end-to-end scenarios; source bytes alone are not a completion metric.

Largest remaining discovered clusters for initial triage (regenerated map; names indicate address neighborhoods, not proven subsystem boundaries):

| Cluster | Rows | Bytes |
|---|---:|---:|
| IcePrunable → OPC_MeshInterface gap | 58 | 40,204 |
| Controller → Fluid gap | 91 | 29,058 |
| ContactPlaneMesh → PenetrationMap gap | 43 | 20,213 |
| TriangleMesh → Controller gap | 24 | 16,739 |
| Fluid → FluidManager gap | 46 | 14,051 |
| SphereShape → ConvexHull gap | 22 | 10,004 |
| Scene.cpp | 42 | 6,849 |
| TriangleMesh.cpp | 25 | 6,647 |
| Shape.cpp | 18 | 6,528 |

These queues do not establish the solver's location. M0 must resolve that from executable dependencies rather than treating a neighboring source-file name as ownership.

## 6. Scheduling and review checkpoints

Do not promise a completion date from the 749-row count. It mixes existing implementations awaiting proof, vendor correspondence, small thunks, and complex missing algorithms. Previous timing tables include approximated starts and integration overlap, so extrapolating a single rows/hour rate would be misleading.

The first execution checkpoint is M0 plus one complete scene-stepping dependency packet. At that point report: remaining critical-path dependency clusters, observed packet throughput, verification cost, and an optimistic/likely/pessimistic forecast for T1 and full completion. Reforecast after T1 using actual collision/solver behavior. Keep the first testing date separate from the full-DLL completion date.

Review order:

1. Approve this project scope, dependency order, and proposed parallel ownership.
2. Produce the short execution contracts for M0/M1 and each independent workstream, with concrete stable IDs, files, test entry points, and dependency handoffs. Reuse existing contracts wherever current main already satisfies them.
3. Select execution capacity and begin; integrate at each dependency-complete packet.
4. Review T1 standalone results, then T2 Unreal results, then the full M6 acceptance report.

The original full-DLL goal stays intact throughout these intermediate milestones.

### Execution checkpoint — 2026-10-01 (triangle-mesh PMap load path)

- Added an oracle-backed public `NxTriangleMesh` PMap fixture. The test first failed on the candidate and passed on the pinned DLL. `TriangleMesh` now loads descriptor-supplied PMap data after building the mesh model, supports explicit `loadPMap`, reports `hasPMap`, discards the previous PMap on a malformed non-empty reload, and releases PMap state with the mesh. The candidate now passes the same descriptor attach, valid load, malformed reload, and teardown scenario.
- At this checkpoint, the comparison used the existing 17-byte minimal PMap fixture. Oracle `getPMapSize()` returned 24 bytes and `getPMapData()` required the caller to preallocate exactly that size; its normalized payload was `504d415004000000010000001fffffff800000003fffffff`. The candidate accessors were still stubs at the time; the serializer and accessor reconstruction is recorded in the 2026-10-02 checkpoint below.
- Adding the new `TriangleMesh::loadPMap` references exposed a missing `PMap.cpp` dependency in `NxPhysicsThirdPartyTests`. The target now links that source with the DLL-side export definition, matching the existing `NxPhysicsAssetTests` setup.
- Fresh Release build and triangle-mesh API runs pass against both the candidate pair and installed oracle pair. Phase 4 passes at 249/249 and Phase 5 at 2,037/2,037; both immutable public-header checks pass for all 80 files. The tested candidate still has the separately tracked qhull mesh ordering and one-ULP settle divergences. This closes the load/attachment subpath, not PMap serialization, PMap computation, M2/M4, or full reconstruction.

### Execution checkpoint — 2026-10-02 (PMap cell runs and public serialization)

- Reconstructed `phys_fn_001990`'s Morton-ordered 5-bit cell-run encoder and `phys_fn_002008`'s 32-way decoder, including neighbor moves and absolute-coordinate escapes. Reconstructed `phys_fn_002017` value grouping, repeat/absolute id coding, terminator, sign plane, and header, then wired `phys_fn_002172`/`002174` to the public size/data methods. No public Physics headers changed.
- The candidate first failed the newly added public API test at the old stubs. It now matches the pinned oracle exactly for the minimal map (24 bytes and `504d415004000000010000001fffffff800000003fffffff`) and a resolution-32 four-cell run (size 4,124; FNV-1a `575faca3773bc417`). The latter covers step codes 19, 1, and 8 plus code 26 with an absolute x coordinate. Both sides reject adjacent buffer sizes; after malformed reload both report no PMap, size 0, and export false.
- A first implementation using `std::vector` made the module audit load `MSVCP140.dll`, which the oracle pair does not use. The implementation now uses the existing CRT allocation and `qsort` path; the pair audit reports zero rejected modules.
- Fresh `run_phase_gate.ps1 -Phase 4` passes at 249/249 (`build/phase4-pmap-serialization.log`); fresh Phase 5 passes at 2,037/2,037 (`build/phase5-pmap-serialization.log`). Both immutable-header checks pass all 80 files and inventory validation reports 6,338 functions, 5,138 data objects, zero unexplained rows. The focused `NxPhysicsTriangleMeshApiTests` also exits 0 against both candidate and installed oracle pairs. See `evidence/phase4-pmap-serialization.md` for row mapping and fixture details.
- Remaining PMap work includes `NxCreatePMap` computation and the alternate filename/implicit-stream creation arms. The qhull/mesh vertex ordering and one-ULP settle difference remain open, as do the full M2/M4 workstreams and M6 full-DLL closure.

### Execution checkpoint — 2026-10-02 (computed PMap oracle fidelity)

- Reproduced the computed-map mismatch in `NxPhysicsTriangleMeshApiTests`: the candidate emitted 10,434 bytes (`61b011762be70a0b`) while the oracle emitted 10,444 (`9a70de00aaf0edd4`). Capturing `mGrid` at both serializer entry points narrowed the difference to 17 face labels with occupancy and interior bits equal.
- The exact face labels came from the point/triangle distance kernel. Reusing the recovered point-triangle arithmetic aligned the raw candidate grid with the oracle. The remaining byte mismatch was the cell-run key's axis lane order: oracle `phys_fn_001990` puts z in the low Morton lanes, y in the middle, and x in the high lanes. Correcting that order makes the computed payload match byte-for-byte.
- Kept the kernel local to `PMap.cpp` because the standalone asset and third-party harnesses compile that file directly; linking the whole `Distance.cpp` translation unit would also pull unrelated segment/contact dependencies. No public Physics headers or CMake target dependencies changed.
- Fresh Release builds of `NxPhysics`, `NxPhysicsAssetTests`, `NxPhysicsThirdPartyTests`, and `NxPhysicsTriangleMeshApiTests` pass. The focused API test exits 0 on candidate and oracle; both compute the resolution-32 authored tetra map as 10,444 bytes with hash `9a70de00aaf0edd4`. The candidate then passes the remaining PMap accessors, malformed reload, actor, and simulation assertions.
- Fresh Phase 4 passes at 249/249 and Phase 5 passes at 2,037/2,037. Both public-header checks cover all 80 files; inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained records. This closes the tested PMap computation fixture only. Broader topology/resolution coverage, filename/implicit-stream creation, the qhull/mesh-ordering discrepancy, and full M2/M4/M6 remain open.

### Execution checkpoint — 2026-10-02 (PMap resolution-64 fidelity probe)

- Added an isolated density-64 mode to `NxPhysicsTriangleMeshApiTests`. It reuses the authored four-face tetrahedron, pins the raw vertex/index arrays before computation, and runs as a fresh process so each DLL begins with its own initial random-ray stream. The oracle exits 0 with 74,563 bytes/hash `2c38820e277e9465`; the current candidate exits 1 with 74,576 bytes/hash `6a2ccf683df333aa`. The default density-32 test still passes on both pairs with 10,444 bytes/hash `9a70de00aaf0edd4`.
- Decoding the cell groups shows 178 stored face-label differences. An initial coordinate-level inspection attributed them to equal-distance nearest-face ties, but a later review found the Morton-axis mapping used for that inspection was wrong, so tie ownership is not established. The oracle takes the OPCODE point-distance query path (`phys_fn_005337` / `FUN_100e8650`, using `FUN_100e7c50`) while the candidate uses its local `nxPMapNearestNoLeafNode` scan. Reversing the candidate child order made the comparison worse (288 differing labels), so that experiment was reverted. Exact traversal/float behavior remains unresolved; no Physics implementation was changed in this checkpoint.
- The test output and evidence are in `evidence/pmap-resolution64.md`. This is a reproducible focused red probe, not a claim that the map-compute surface is complete. Density-80 remains unverified in an isolated process; earlier sequential runs consumed different DLL-local random streams and are not a valid signature for it.

### Execution checkpoint — 2026-10-02 (PMap mismatch triage)

- IDA's `sub_100E8650` decompilation confirms the no-leaf traversal order and pruning currently modeled in `nxPMapNearestNoLeafNode`: first child at `+0x18`, then sibling at `+0x1c`; internal children recurse and node AABBs prune against the current best float. This makes child order alone a weaker cause, though tree-build equivalence is still unproven.
- Rejected and reverted a global `<=` tie update / smaller-face tie rule (density-64 `74574 / abfedae03fe9973b`; smaller-face also breaks density 32) and a widened final-quadratic experiment based on the existing closest-point parameters (density-64 `74573 / 331649a5849dcb5f`, density-32 `10446 / 358d922e23c58b0f`). Neither matches both oracle fixtures. The decoder-axis assumption used for one attempted coordinate analysis was also found wrong and corrected before drawing conclusions.
- Restored production and test sources to the committed baseline, rebuilt Release `NxPhysics` and `NxPhysicsTriangleMeshApiTests`, then re-ran the candidate default probe (pass, `10444 / 9a70de00aaf0edd4`) and density-64 probe (still red, `74576 / 6a2ccf683df333aa`). See `evidence/pmap-resolution64.md`; precise distance/sample ownership remains open.
- Captured the raw 64^3 grids at `PenetrationMap::finish`: 105 words differ, all in face labels, with identical filled bits. Independent point-to-triangle calculations place the selected faces at equal squared distance on each differing coordinate. Candidate and oracle face preorder both resolve to `3, 1, 2, 0`, ruling out a simple preorder leaf-order mismatch. A temporary change to the candidate's coefficient/interior summation order based on the separate `NxPointTriangleSquareDistance` kernel did not change either PMap hash and was reverted; the remaining target is the OPCODE helper `phys_fn_005324` (`0x000e7c50`) and its x87/AABB behavior. Focused baseline reruns remain density-32 green and density-64 red; details and capture hashes are in `evidence/pmap-resolution64.md`.
