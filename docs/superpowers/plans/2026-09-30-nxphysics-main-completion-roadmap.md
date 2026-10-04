# NxPhysics completion project plan from main

Date: 2026-09-30
Baseline: `main` at `b942f01` (Merge convex-mesh gap reconstruction into main)
Status: Active execution. The user selected full-DLL reconstruction and standalone simulation tests before Unreal integration.

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
| Scene simulation API | `getGravity`/`setGravity`, `getTiming`/`setTiming`, and the write-lock `isWritable` probe match oracle outputs; `startRun`/`finishRun`/`runFor` follow the oracle's deprecated-warning and call sequences. `simulate`, `checkResults`, `fetchResults`, and fence APIs remain open. | The worker/event lifecycle and real stepper are the immediate blockers to useful physics simulation tests |
| Final gate | Phase 8 has no registered test targets and coverage floor zero | A separate whole-DLL acceptance gate must be built |

The original current-main artifacts are `build/main-phase5.log` and `build/main-phase7-with-simulation.log`; they record the pre-repair state. Subsequent work added the real public scene step/result path, so the old `stdout_delta=30` stationary-candidate failure is superseded by the checkpoint below. The explicit-friction differential was the last Phase 7 failure and now passes on the current worktree revision; the test-matrix and full-DLL work remain open. Other phase results should be tied to their current logs and source revision. Older branch reports have different coverage floors and must not be presented as current-main verification.

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
- A temporary public-API D6 swing-limit probe confirms kind 2 reaches the solver. Comparing the same four-step 60-degree swing fixture showed that candidate and oracle positions and quaternions match bit-for-bit; only tiny angular-velocity values differ (six stdout bit-field differences). The initial solver-row inputs match, and tracing narrowed the residual to angular-impulse cancellation around the final solver pass, but did not yet prove the exact rounding source. The probe and logging instrumentation were removed without registering the unresolved case in Phase 7. Revisit with a focused `004395` angular-impulse differential before claiming kind-2 response closure (`build/d6-diag-diff4.log`, `build/d6-kind2-oracle-all-cdb.log`).
- On 2026-10-03, the formerly red `NxTriangleMeshShapeDesc` actor probe was traced to `nxActorShapeFactory` omitting type 4, with no installed MESH family table or public mesh handle. The factory now builds the 0xe8 runtime shape, dispatches descriptor load and owner update through the MESH table, exposes type 4 and the original public mesh through the shape handle, and dispatches mesh slot 0 during actor teardown. A differential probe first confirmed actor creation, then a second probe caught and fixed the wrapper offset in `getTriangleMesh`. The registered `NxPhysicsSimulationTests` target now matches exactly; Phase 7 passes at 1,300/1,300 and Phase 5 at 2,037/2,037, inventory validation passes with 6,338 functions / 5,138 data objects / zero unexplained, and the CMake Release build succeeds. This closes the exercised single-mesh actor create/readback/release route only: `phys_fn_000032` remains partial, compound construction, remaining MESH virtuals, mesh model/tree construction, mesh contact, and full Phase 5 closure remain open. No public Physics headers changed.
- The actor's `getWorldBounds` probe showed the new mesh path returning a zero box. Oracle evidence pins `TriangleMesh+0x44..+0x58` as local min xyz/max xyz; the constructor initializes it to the empty-box sentinels, and MESH slot 9 consumes it directly when no support tree exists. The private layout now names those six floats and ordinary mesh loading accumulates the input vertex bounds. The fresh `NxPhysicsSimulationTests` differential matches exactly (`stdout_delta=0`, stderr exact), and the Win32 Release `NxPhysics` target builds. This is evidence for the ordinary loaded-mesh/no-tree bounds path only; `phys_fn_002260` mesh-model construction, tree-backed bounds, serialization completeness, mesh collision, and the broader TriangleMesh unit remain open. No public Physics headers changed.
- On 2026-10-03, the registered mesh simulation now queries the built OPCODE model for sphere candidates, and ordinary sphere/mesh contact rejects an overlapping sphere on the triangle's back side. The regression first showed seven candidate callbacks against zero in the oracle; after the face-side filter, the drop and underside cases match bit-for-bit. Matrix-B `NxOverlapSphereMesh` is also wired from the `001925` listing and reaches a public mesh-trigger enter callback; the fixture rotates and translates the trigger mesh and matches the oracle. A second trigger fixture puts a sphere inside the mesh bounds but outside both triangles; the oracle and candidate both report exactly one expected enter, with no unexpected callback. A boundary-hit fixture places the sphere center outside the mesh footprint; its reported contact point is now the oracle's closest mesh point `(2,0,0)` and its separation matches bit-for-bit after using the stored local delta and x87 operation order. A separate 90-degree rotation plus translation fixture matches the world-space contact point at `(-10,5,0)`. The focused differential and fresh Phase 7 gate pass at 1,305/1,305 (`build/phase7-mesh-edge-separation.log`); the mesh target has `stdout_delta=0` and exact stderr. Boundary normal and post-solver trajectory still need exact comparison and reconstruction. Both installed Physics public-header manifests pass (80 files). This advances the ordinary sphere/mesh paths, not full semantic closure of `001927`/`001929`: heightfield-specific feature normals, the full edge/corner branches, broader transformed/multi-face contact coverage, solver response for the boundary case, and mutation evidence remain open. Inventory closure evidence also remains open. No public Physics headers changed.

### Execution checkpoint — 2026-10-03, sphere-mesh normal and Phase 5 repair

- The new normal observations localized the boundary differential to precision loss: face and edge contact points, separations and post-step actor state already matched, while patch normals differed by one or two ULPs. `NxContactSphereMesh` now keeps the `NxPointTriangleSquareDistance` double squared-distance through x87 `sqrt`, reciprocal and per-component multiplies. The two face patches and the boundary-edge patch now match all oracle normal words, and the fixture also pins patch counts, per-patch normals, separation, position and velocity.
- Fresh `NxPhysicsMeshSimulationTests` differential passes with `stdout_delta=0` and exact stderr. Phase 7 passes at 1,305/1,305 (`build/phase7-mesh-normal.log`).
- Phase 5's fresh build exposed missing `NxContactSphereMesh` and `NxOverlapSphereMesh` symbols in the static object-layout/vtable harness. Added link-only definitions to `tests/PhysicsInternalContactStubs.cpp`; production and simulation targets continue linking the real sphere/mesh implementations. Phase 5 now passes all 13 targets at 2,037/2,037 (`build/phase5-mesh-normal-final.log`). The focused gate-registry test suite passes all 37 tests.
- This closes only the exercised ordinary-mesh face and one boundary-edge normal/response route. Heightfield normals, other edge/corner branches, broader transformed/multi-face and mutation coverage, and full `001927`/`001929` reconstruction remain open. Public Physics headers were not changed.

### Execution checkpoint — 2026-10-03, sphere-mesh vertex branch

- Added a public sphere/mesh corner fixture with the sphere centered beyond the `(2,0,2)` mesh vertex. Oracle and candidate both report one callback and one patch at step 30, contact point `(2,0,2)`, normal words `bf073a69.bf2a2f2b.bf073a69`, separation `bcdaf02f`, and matching post-step Y position/velocity. This exercises a vertex-distance branch in addition to the previously pinned face and edge routes.
- The focused mesh differential passes with `stdout_delta=0`, exact stderr. Fresh Phase 7 passes at 1,306/1,306 (`build/phase7-mesh-vertex.log`).
- Remaining mesh work includes the other edge/corner branches, heightfield-specific normals, broader transformed/multi-face and mutation cases, and semantic closure of `001927`/`001929`. Full M1 and full-DLL completion remain active.

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
