# Novodex Physics Phase 7 Scenes and Simulation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct all scene lifecycle, query, stepping, integration, island, constraint, solver, sleeping, reporting, callback, and remaining Phase 7 orchestration behavior.

**Architecture:** Establish scene storage and queries first, then active-set/island construction, integration, constraints/solver, sleeping, and reporting. Multi-frame trajectory harnesses serialize complete observable state at every step and isolate oracle/candidate processes.

**Tech Stack:** Foundation-style C++; Ghidra/Capstone dependency graph; deterministic binary scenario format; separate-process trajectory runner; UE3-compatible x87/SSE handling.

---

### Task 1: Define deterministic scene scenarios and state serialization

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `D:\github\Novodex\tests\PhysicsSceneTests.cpp`
- Create: `D:\github\Novodex\tests\PhysicsTrajectoryTests.cpp`
- Create: `docs/reconstruction/novodex-physics/cases/scenes/`
- Create: `docs/reconstruction/novodex-physics/evidence/phase7-scenarios.md`

- [ ] Define a binary scenario format with SDK/scene settings, materials, actors, shapes, joints, effectors, controllers, fluids/implicit meshes, fence/result operations, commands, timestep sequence, and requested observations. Store floats as hexadecimal bits.
- [ ] Serialize after every command/frame: actor transforms and velocities, awake state, contacts, joint state, query results, callbacks, errors, allocation deltas, and enumeration order.
- [ ] Create fixed scenarios for empty scene, falling body, static contact, stacked bodies, multiple islands, jointed bodies, forces/impulses, kinematic motion, sleep/wake, create/delete during lifecycle, collision filtering, queries, controller creation/motion/release, fluid/implicit-mesh supported or oracle-unsupported behavior, fence/result sequencing, scene statistics, and callback ordering.
- [ ] Run oracle scenarios twice and require deterministic normalized output or record a measured instability artifact before allowing tolerance.
- [ ] Run current candidate and prove RED before scene production changes.

### Task 2: Reconstruct scene lifecycle and queries

**Files:**
- Create: `D:\github\Novodex\Physics\src\Scene.cpp`
- Create: `D:\github\Novodex\Physics\src\SceneQueries.cpp`
- Create: `D:\github\Novodex\Physics\src\Controller.cpp`
- Create: `D:\github\Novodex\Physics\src\Fluid.cpp`
- Create: `D:\github\Novodex\Physics\src\ImplicitMesh.cpp`
- Create: `D:\github\Novodex\Physics\src\SceneResults.cpp`
- Create: `D:\github\Novodex\Physics\src\SceneStats.cpp`
- Create: private headers `Scene.h` and `SceneQueries.h`

- [ ] Recover scene layout, vtable, creation validation, collections, limits, timing state, callback pointers, actor/joint/material/controller/fluid integration, fence/result state, statistics, and release order.
- [ ] Reconstruct create/release, enumeration, settings, broad-phase insertion/removal, raycast/overlap/cull queries, and result reporting.
- [ ] Preserve query ordering, early-out, buffer limits, callback termination, group/mask filtering, and alias behavior.
- [ ] Require exact scene-lifecycle and query transcripts before stepping work.
- [ ] Carry the `static_proof_closures_have_no_oracle_side` escalation in `gates/phase2.json`. The SDK lock rows `phys_fn_002362`, `phys_fn_002364` and `phys_fn_002366` are closed against checks derived from the disassembly, with the reentrancy of `tryLock` on the owning thread never observed against the shipped DLL. Scene stepping is where the lock is actually contended: gate its dynamic behaviour against the oracle here, or record why the scenarios cannot reach it.
- [ ] Carry the `simulation_control_word_scope` escalation in `gates/phase3.json`. `phys_fn_000659` at `0x00013c40` is a Phase 7 row and it is the thing that makes the window: it saves the caller's control word, probes the rounding mode with `1.9f`/`fistp`, sets `_PC_64 | _RC_CHOP` and restores on the way out. Reconstruct it so the word is installed, because everything Phase 3 closed inside the step was measured against a harness that installs it, and record which word every Phase 7 row executes under.
- [ ] Cross-check the `borrowed_phase_5_and_7_layout` escalation in `gates/phase3.json`. Phase 3 borrowed the contact sink — a `0x10` prefix plus a `0x34` sub-object to `+0x44`, the cached contact normal at `sink+0x28..+0x30`, and `sink+0xe8` — with an address for each. Phase 7 owns those rows: confirm each offset against `phys_fn_000893` and `phys_fn_002356` rather than inheriting it, and settle the sink past `+0x44` at `+0xdc..+0xe9`, which is one of the three reasons the continuous-collision sweep is a stop.
- [ ] Carry the `contact_sink_warm_start_axis_survives_reset` escalation in `gates/phase3.json`. `sink+0xe8` is a warm-start separating-axis index that `phys_fn_002354` at `0x0005b620` does not clear — it covers `+0x10..+0x43` — so narrow-phase state carries from one box pair to the next inside one step, and a warm byte makes `phys_fn_001745` skip nine edge-axis tests entirely. Any scene-level reset Phase 7 writes must reproduce that rather than tidy it, and the trajectory gates should expect a pair's result to depend on what was tested before it.
- [ ] Carry the `phase4_never_driven_under_simulate_word` escalation in `gates/phase4.json`. Not one Phase 4 row was ever measured under `0x0f7f`: every mutation pass, both of its oracle differentials and all 26 of its closed rows run under the CRT default `0x027f`, while at least 21 OPCODE rows sit inside `Scene::simulate`'s closure in the image and two closed rows -- `phys_fn_004886` and `phys_fn_004894` -- are inside it as well. Those two are integer-only and provably indifferent, but `Segment::SquareDistance` at `0x000f0560` already disagrees with the image by up to 8,420 ULP under `0x027f`, so the mesh and acceleration column is where the word most plausibly changes an answer. The 21 is derived in `evidence/phase4-third-party-sources.md` §4.6 by walking `jmp` edges beside `call` edges. When Phase 7 installs the word, re-drive `NxPhysicsAssetTests` and `NxPhysicsThirdPartyTests` under it and record which Phase 4 rows agree under which word.
- [ ] Carry the `box_box_contact_frame_overflow` escalation in `gates/phase3.json`. `phys_fn_001741` has no contact cap and `phys_fn_001749`'s frame holds sixteen, with the return address flush against the sixteenth point and no `/GS` cookie; driven with the SAT-selected face and ordinary geometry it returns eighteen, about 2 in 32,000 aimed calls. A Phase 7 scenario that stacks boxes is the first thing in this programme that can reach it through a real step, so expect it as shipped behaviour rather than as a candidate defect, and do not fix it into a divergence.

Create a public-surface disposition matrix for every `NxScene` vtable slot and every controller, fluid, implicit-mesh, fence/result, and scene-statistics type present in the pinned headers or oracle RTTI/factories. Each row is `supported`, `oracle_unsupported`, or `absent_from_oracle`, with Ghidra/Capstone evidence. Supported and oracle-unsupported rows both require executable RED/GREEN cases; `absent_from_oracle` requires proof that no oracle vtable/factory/call target implements it.

### Task 3: Reconstruct active sets, islands, and integration

**Files:**
- Create: `D:\github\Novodex\Physics\src\Simulation.cpp`
- Create: `D:\github\Novodex\Physics\src\IslandManager.cpp`
- Create: `D:\github\Novodex\Physics\src\Integrator.cpp`
- Create: matching private headers under `Physics\src\include`

- [ ] Recover step state machine, timestep subdivision, active-body ordering, graph construction, island merge/split, gravity/force integration, kinematic handling, and transform propagation.
- [ ] Add focused RED scenarios for each recovered branch and boundary.
- [ ] Implement mutually recursive groups together and the rest in dependency order, comparing per-substep state, not merely final frames.
- [ ] Close associated scheduler/dispatch/data-table rows only after exact trajectory agreement.

### Task 4: Reconstruct constraints, solver, contacts, and sleeping

**Files:**
- Create: `D:\github\Novodex\Physics\src\ConstraintSolver.cpp`
- Create: `D:\github\Novodex\Physics\src\ContactSolver.cpp`
- Create: `D:\github\Novodex\Physics\src\Sleeping.cpp`
- Create: `D:\github\Novodex\Physics\src\Reports.cpp`
- Create: matching private headers under `Physics\src\include`

- [ ] Recover solver row layout, batching/order, warm-start/cache use, iteration loops, impulses, friction/restitution, projection, breakage, sleep metrics, and reporting sequence.
- [ ] Use Capstone to preserve accumulation order, x87/SSE lifetimes, comparison flags, loop bounds, and table dispatch where they affect results.
- [ ] Implement complete components in Foundation-style C++; do not substitute a different modern solver.
- [ ] Compare constraint rows, impulses, contact reports, actor trajectories, break events, and sleep/wake frames exactly or under an approved measured tolerance.
- [ ] Run long-enough fixed trajectories to expose accumulating divergence and lifecycle leaks.

### Task 5: Close Phase 7

```powershell
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsSceneTests NxPhysicsTrajectoryTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 7
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: all Phase 7 rows closed; every fixed scenario passes at each recorded step; callback and allocation transcripts agree; Phases 1-6 remain pass.

Commit: `docs: close Physics scene and simulation gates`.
