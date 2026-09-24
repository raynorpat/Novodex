# Novodex Physics Phase 5 Actors, Bodies, Shapes, and Materials Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct the complete Phase 5 public object model: descriptors, materials, shapes, actors, rigid bodies, mass/inertia, flags, references, callbacks, and lifecycle.

**Architecture:** Recover interface-to-concrete mappings and vtables before behavior. Descriptor validation and object creation form RED gates; implementations close leaf state accessors first, then mutation, ownership, and actor/body orchestration.

**Tech Stack:** Foundation-style C++; UE3 immutable Physics headers; Ghidra RTTI/vtable/type recovery; Capstone field/call evidence; structural and separate-process differential harnesses.

---

### Task 1: Lock the object model and structural probes

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `docs/reconstruction/novodex-physics/evidence/phase5-object-model.md`
- Create: `docs/reconstruction/novodex-physics/object_model.json`
- Create: `D:\github\Novodex\tests\PhysicsObjectLayoutTests.cpp`

- [ ] Map every recovered public interface to concrete classes, base offsets, vtables, constructors, destructors, factories, and owner links.
- [ ] Record every vtable slot as stable function ID or pure/unsupported entry; reject unknown slots and duplicate ownership.
- [ ] Build candidate-only layout probes with `sizeof`, alignment, base offsets, and private-field offset accessors guarded to tests.
- [ ] Build oracle behavioral probes that distinguish slot order through public calls without static-linking the oracle DLL.
- [ ] Cross-check the `borrowed_phase_5_and_7_layout` escalation in `gates/phase3.json`. Phase 3 could not reconstruct a matrix entry without `Shape+0x04`, `Shape+0x0c..+0x38`, `Shape+0x9c`, `Shape+0xe0..+0xec` and `Shape+0xe4` as three floats, and it recorded an address for every one of them. Phase 5 owns those rows: confirm each offset against the constructors rather than inheriting it, resolve vtable slot 7 for every shape type — it is unresolved for all of them and is what makes the continuous-collision sweep a stop — and settle whether `Shape+0x3c..+0x68` is a second pose.
- [ ] Carry the `simulation_control_word_scope` escalation in `gates/phase3.json`. Every shape vtable Phase 5 recovers widens the reachability graph, and Phase 3's slot-5 recovery already moved five closed exports into the `0x0f7f` window. Drive reconstructed rows under both control words and record which one each executes under.
- [ ] Take on the `mesh_column_blocks_phase3_mesh_deferrals` escalation in `gates/phase4.json`. **Thirteen** Phase 3 deferrals are waiting on the mesh column Phase 4 did not reconstruct -- `phys_fn_001757`, `001772`, `001777`, `001779`, `001781`, `001783`, `001870`, `001876`, `001893`, `001895`, `001897`, `001925` and `001929`, every one `blocked_on_type: TriangleMesh` with `driving_phases: [4]` and the note *"It needs the triangle mesh, its acceleration structure and the vertex and index data behind them"*. Phase 4 discharged none of them and closed anyway, so `driving_phases: [4]` now names a phase that is finished. The MeshShape constructor and two MESH slots this phase has already transcribed into `Physics/src/ObjectModel.cpp` do not reach them: what the thirteen need is the internal mesh -- `InternalTriangleMesh`, the `OPCODECREATE` fill at `phys_fn_002083` and the accept arm of the mesh reader -- and vendoring OPCODE cleared only the fourth prerequisite. **None of the thirteen has a candidate implementation.** Three carry a `source` naming `Physics/src/ContactMeshMesh.cpp` or `Physics/src/ContactPlaneMesh.cpp`, neither of which exists: `source` is the *oracle's* translation unit, not a file to build on. Reconstruct the mesh with the thirteen in scope and either discharge each one with `discharged_by_phase: 5` on the Phase 3 ledger or re-point its `driving_phases` at a phase that can.
- [ ] Take on the `deferrals_left_on_passed_phase_4` escalation in `gates/phase4.json` for the six of its seven rows that are this phase's. Five are Phase 2 deferrals that wait on the same internal mesh as the thirteen above -- `phys_fn_000242` and `phys_fn_000478` (createTriangleMesh), `phys_fn_000244` and `phys_fn_000470` (releaseTriangleMesh) and `phys_fn_000246`, the outlined unwind of releaseTriangleMesh -- each `blocked_on_type: TriangleMesh` with `driving_phases: [4]`; bring them into the mesh reconstruction and discharge each with `discharged_by_phase: 5` on the Phase 2 ledger, or re-point its `driving_phases`. The sixth is `phys_fn_000965`, the `xor eax,eax; ret` stub `ShapeBase::ShapeBase` installs as the first Prunable owner adapter at `.data 0x10128470` and that is slot 5 of the box-hull table: it is reconstructed and nothing has falsified it, so aim a mutation at it through this phase's constructor drive. A discharge counts only once this phase has passed. The seventh row, `phys_fn_001751`, is Phase 8's.
- [ ] Take on the `deferrals_left_on_passed_phases_3_and_4` escalation in `gates/phase4.json` for `phys_fn_001502`, the one of its eighteen this phase inherits: a Phase 2 homeless shared row whose driving phases, 3 and 4, have passed, and that MESH slot 3 (`phys_fn_001393`, Phase 5 under `shape_slot_ruling.json`) calls. Drive it through that slot and discharge it with `discharged_by_phase: 5` on the Phase 2 ledger, re-pointing its `driving_phases` to include 5, or say which phase can. A discharge counts only once this phase has passed.
- [ ] Commit the model and RED probes before implementations.

### Task 2: Reconstruct descriptors and materials

**Files:**
- Create: `D:\github\Novodex\Physics\src\Descriptors.cpp`
- Create: `D:\github\Novodex\Physics\src\Material.cpp`
- Create: private headers `DescriptorValidation.h` and `Material.h`
- Create: `D:\github\Novodex\tests\PhysicsObjectTests.cpp`

- [ ] Add descriptor corpus modes covering defaults, every flag family, invalid enums, invalid numeric ranges, null dependencies, and combinations observed in validation branches.
- [ ] Serialize validation returns, emitted errors, normalized stored state, and material combine behavior.
- [ ] Reconstruct validation and material components leaf-first, preserving comparison strictness and callback order.
- [ ] Require exact oracle/candidate output and close their stable-ID/data rows.

### Task 3: Reconstruct shape implementations

**Files:**
- Create: `D:\github\Novodex\Physics\src\Shape.cpp`
- Create: `D:\github\Novodex\Physics\src\BoxShape.cpp`
- Create: `D:\github\Novodex\Physics\src\SphereShape.cpp`
- Create: `D:\github\Novodex\Physics\src\CapsuleShape.cpp`
- Create: `D:\github\Novodex\Physics\src\PlaneShape.cpp`
- Create: `D:\github\Novodex\Physics\src\MeshShape.cpp`
- Create: matching private headers under `Physics\src\include`

For each recovered shape family:

- [ ] Add RED creation, getters/setters, transform, bounds, flags, material, group/mask, query, ownership, and release cases.
- [ ] Reconstruct vtables and layouts first, then behavior in dependency order.
- [ ] Compare every observable field, bounds bit pattern, callback, allocation, query result, and destruction event.
- [ ] Commit each non-trivial family independently with static and dynamic evidence.

### Task 4: Reconstruct actors and bodies

**Files:**
- Create: `D:\github\Novodex\Physics\src\Actor.cpp`
- Create: `D:\github\Novodex\Physics\src\Body.cpp`
- Create: private headers `Actor.h` and `Body.h`

- [ ] Add RED modes for static/dynamic actors, multiple shapes, transforms, velocities, forces/torques, damping, momentum, mass/inertia updates, flags, sleeping requests, user data, enumeration, and release order.
- [ ] Recover construction rollback, shape attachment, scene owner links, active/sleep state storage, and body/actor base offsets.
- [ ] Implement all Phase 5 actor/body stable IDs while calling the already closed Phase 3 mass-properties component and preserving the later Phase 7 solver boundary as an explicit interface, not a passing stub.
- [ ] Re-run shape/material cases through actors and require exact state/callback transcripts.

### Task 5: Close Phase 5

```powershell
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsObjectLayoutTests NxPhysicsObjectTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 5
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: all Phase 5 rows closed; no unknown vtable slots; exact descriptor/object lifecycle transcripts; earlier phase gates pass.

Commit: `docs: close Physics object model gates`.
