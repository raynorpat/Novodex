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
