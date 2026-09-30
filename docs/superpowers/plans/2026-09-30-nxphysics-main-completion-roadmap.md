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
| Phase 5 | The baseline run passed 13 staged-pair targets (2,037/2,037 assertions). The object-layout target still emits one explicit `CANDIDATE-MISSING family=vtables` marker and exits red; the marker names final shape/actor vtables. `NpActorVtable` confirms that most actor virtuals are still unimplemented. | Reconstruct and differentially exercise the concrete actor and shape dispatch tables. Remove the marker only after real candidate dispatch checks cover the represented families; do not waive the red status. |
| Inventory validation | Pass; 6,338 function records, 5,138 data records, zero unexplained rows | This proves ledger consistency, not completion |
| Executable code census | 2,787 code rows / 938,498 bytes: 749 discovered, 1,887 reconstructed, 145 dynamically gated, 6 statically reviewed | Separate missing implementation from verification debt |
| Discovered code | 218,282 bytes across 749 unique IDs | This is an audit queue, not a claim that every byte is unwritten |
| Data | All 5,138 records are classified | Prove candidate ownership and relocation for required tables/globals; classification alone is insufficient |
| Work-unit map | Committed map has 143 records, 31 duplicate names and 2,023 multiply assigned IDs; regeneration in `build/main-planning-work-units.json` produces 109 units, 60 named and 49 gaps | Repair generated scheduling data before assigning work |
| Scene simulation API | `getGravity`/`setGravity`, `getTiming`/`setTiming`, and the write-lock `isWritable` probe match oracle outputs; `startRun`/`finishRun`/`runFor` follow the oracle's deprecated-warning and call sequences. `simulate`, `checkResults`, `fetchResults`, and fence APIs remain open. | The worker/event lifecycle and real stepper are the immediate blockers to useful physics simulation tests |
| Final gate | Phase 8 has no registered test targets and coverage floor zero | A separate whole-DLL acceptance gate must be built |

Build and Phase 5 logs from this assessment are local artifacts at `build/main-planning-build.log` and `build/main-planning-phase5.log`. Other phases and the full Python suite were not rerun for this planning assessment. Older branch reports have different coverage floors and must not be presented as current-main verification.

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
- Run a clean CMake build, header check, inventory validator, tooling suite, and gates 2–7 once on a single main SHA. Record candidate hashes and exact failures. Phase 5's existing marker is the starting exception, not a new pass condition.

Exit: every remaining ID and unresolved dependency has one owner and a test route; no duplicate work assignments; the first simulation dependency packet is ready. Timebox the initial scheduling pass to one work session and deepen contracts as each packet starts.

### M1 — Bring up the real standalone simulation path (critical path)

Primary code: `Physics/src/NpScene.cpp`, `Physics/src/Scene.cpp`, their private headers, and the actual solver/scheduler units identified by M0. Proposed test target: `NxPhysicsSimulationTests`.

- Reconstruct scene gravity, timing, writable/running state, lock and result semantics, and error paths from the oracle. Follow the oracle's relationship between old run APIs and newer simulate/fetch APIs rather than imposing a new engine design.
- Gravity and timing reads/writes have been implemented and pinned at bit level; keep this verified slice while completing the remaining scene methods.
- The current standalone fixture reaches the public scene API but the candidate leaves result flags false and the body stationary. First copy the oracle's worker/event ownership and simulate/check/fetch transitions; then wire the actual stepper. Keep this as the critical path for starting useful tests.
- Wire the recovered body state, forces, and joint implementations through the real stepping path. Complete missing solver and integration callees as a dependency cluster; do not create a substitute Euler integrator just to pass a falling-box test.
- Build independent oracle and candidate processes from identical serialized fixtures. Compare per-step poses, velocities, forces, wake state, result status, callbacks, and allocation/lifetime events.
- Start with an empty scene and one body under gravity/force, then static contact, two-body collision, kinematic interaction, sleep/wake, a small stack, and a jointed pair. Include variable step sizes and the oracle's FP control-word transitions.
- Exercise at least 1,000 steps for basic stable fixtures, repeated scene/SDK lifecycle, and both blocking and nonblocking result calls. Expand cases to reach every new branch before claiming its closure.

Exit / **T1: standalone testing begins**: the candidate runs the genuine public step/result path through collision and solver code; the above fixtures execute without stubs, crashes, hangs, or unexplained mismatches. Any longer numerical divergence has a measured first divergent step and remains an open defect. This is not whole-DLL completion.

### M2 — Close object, mesh, and pruning ownership in parallel

Primary code: `NpActor.cpp`, `Scene.cpp`, `ObjectModel.cpp`, `TriangleMesh.cpp`, shape and pruning units. Coordinate edits to `Scene.cpp` through its owner.

- Audit the eight Phase 5 discovered rows and the real factory-to-wrapper-to-final-vtable paths for every shape family, static/dynamic bodies, meshes, compounds, controllers, and release/rollback.
- Phase 5's current red gate is specifically blocked by the missing concrete actor/shape vtable family. `NpActorVtable` has placeholder defaults for most public slots; use the pinned vtable census (`phys_data_000678` and `phys_data_000679`) to inventory all 87/88 entries, group inherited/common slots, and implement oracle-backed targets to make the concrete dispatch contract real. Drive slots through constructed candidate objects and a pinned-oracle process. The existing `NxPhysicsShapeVtableTests` covers shape cases but does not replace the actor table contract.
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
