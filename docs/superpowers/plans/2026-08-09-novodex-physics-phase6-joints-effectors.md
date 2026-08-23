# Novodex Physics Phase 6 Joints and Effectors Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct all Phase 6 joint descriptors, concrete joint families, limits, motors, springs, projection, callbacks, and effectors.

**Architecture:** Recover shared joint bases and constraint-row generation before family-specific behavior. Each family has exact descriptor, vtable, state, solver-input, projection, callback, and lifecycle differential cases.

**Tech Stack:** Foundation-style C++; Ghidra class/vtable recovery; Capstone constraint math and dispatch analysis; deterministic joint harnesses.

---

### Task 1: Recover the joint hierarchy and shared constraint contract

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `docs/reconstruction/novodex-physics/evidence/phase6-joint-model.md`
- Create: `docs/reconstruction/novodex-physics/joint_model.json`
- Create: `D:\github\Novodex\tests\PhysicsJointTests.cpp`

- [ ] Map `NxJoint` and all ten public families—revolute, spherical, prismatic, cylindrical, point-on-line, point-in-plane, D6, distance, fixed, and pulley—including bases, vtables, descriptor conversions, factory/enum slots, actor links, local/global frames, and solver-row callbacks.
- [ ] Include the exported `NxJointDesc_SetGlobalAnchor` and `NxJointDesc_SetGlobalAxis` rows and record exact transform/evaluation order.
- [ ] Add RED modes for null/one/two actors, anchors/axes, limits, motors, springs, flags, break thresholds, projection, state reporting, user data, and release.
- [ ] Serialize complete joint state and recovered constraint rows rather than summary tokens.
- [ ] Carry the `simulation_control_word_scope` escalation in `gates/phase3.json`. Joints are solved inside `Scene::simulate`, which installs `_PC_64 | _RC_CHOP` (`0x0f7f`), so a joint row measured through the exported descriptor API is measured under the wrong word. Drive each reconstructed row under both and record which one it executes under; the ten joint families dispatch through their own vtables, and every table recovered widens the reachability graph and can move rows another phase has already closed.

### Task 2: Reconstruct shared joint infrastructure

**Files:**
- Create: `D:\github\Novodex\Physics\src\Joint.cpp`
- Create: `D:\github\Novodex\Physics\src\JointConstraint.cpp`
- Create: private headers `Joint.h` and `JointConstraint.h`

- [ ] Reconstruct descriptor validation, coordinate-frame conversion, actor ownership, common getters/setters, breakage state, solver-row allocation, projection dispatch, and destruction.
- [ ] Preserve row order, axis normalization behavior, comparison boundaries, callback timing, and allocation rollback.
- [ ] Require exact oracle/candidate shared-joint transcripts before family work.
- [ ] Close all Phase 6 stable IDs owned by the shared component and commit production/evidence separately.

### Task 3: Reconstruct every joint family

**Files:**
- Create: `D:\github\Novodex\Physics\src\RevoluteJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\SphericalJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\PrismaticJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\CylindricalJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\PointOnLineJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\PointInPlaneJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\D6Joint.cpp`
- Create: `D:\github\Novodex\Physics\src\DistanceJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\FixedJoint.cpp`
- Create: `D:\github\Novodex\Physics\src\PulleyJoint.cpp`

For each inventoried family:

- [ ] Prove an old predicate cannot pass a deliberately wrong unchecked field/row; strengthen the RED harness to serialize the complete observable record.
- [ ] Reconstruct layout, vtable, validation, setters/getters, constraint rows, limits/motor/spring behavior, projection, and release.
- [ ] Compare exact outputs at inactive, boundary, active, and broken states.
- [ ] Commit one family at a time and close all its function/data rows.

Before family implementation, create an explicit disposition row for every public joint family and every joint enum/factory slot: `supported`, `oracle_unsupported`, or `absent_from_oracle`. Supported and oracle-unsupported families receive concrete vtable/source implementations and RED/GREEN behavior tests. `Absent_from_oracle` is permitted only with Phase 1 proof that no RTTI, vtable, factory entry, call target, or public method body exists; it does not erase associated compiler/error stubs from the function census.

### Task 4: Reconstruct effectors

**Files:**
- Create: `D:\github\Novodex\Physics\src\Effector.cpp`
- Create: `D:\github\Novodex\Physics\src\SpringAndDamperEffector.cpp`
- Create: matching private headers under `Physics\src\include`

- [ ] Add RED creation, attachment, parameter, force/impulse, active/inactive, callback, and release modes.
- [ ] Reconstruct all inventoried effector implementations and dispatch/factory data.
- [ ] Compare exact per-step effector outputs and object lifecycle transcripts.

### Task 5: Close Phase 6

```powershell
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsJointTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 6
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: every inventoried joint/effector family and dispatch row closed; exact full records; no unknown factory or vtable entries; Phases 1-5 remain pass.

Commit: `docs: close Physics joint and effector gates`.
