# Novodex Physics Phase 4 Mesh and Spatial Assets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct triangle meshes, convex hull data, PMaps, acceleration structures, asset ownership/serialization, and all Phase 4 runtime queries.

**Architecture:** Recover binary formats and ownership before query algorithms. Tests use fixed byte fixtures and oracle-created assets, then compare structure, reference behavior, queries, enumeration, and destruction in separate processes.

**Tech Stack:** Foundation-style C++; Ghidra data/type recovery; Capstone construction/query paths; byte-level fixtures; differential asset harnesses.

---

### Task 1: Recover asset formats and lifecycle

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `docs/reconstruction/novodex-physics/evidence/phase4-formats.md`
- Create: `docs/reconstruction/novodex-physics/cases/assets/`
- Create: `D:\github\Novodex\tests\PhysicsAssetTests.cpp`

- [ ] Enumerate Phase 4 functions/data and group them into mesh, convex, PMap, spatial tree, serialization, and ownership components.
- [ ] Derive headers, tags, versions, counts, strides, offset bases, endian handling, alignment, and validation branches from Ghidra plus Capstone.
- [ ] Capture minimal valid, multi-element, malformed, truncated, and boundary fixtures without copying mutable oracle process pointers.
- [ ] Add RED lifecycle/query modes and record exact oracle results, allocations, references, and callbacks.
- [ ] Carry the `simulation_control_word_scope` escalation in `gates/phase3.json`. `Scene::simulate` installs `_PC_64 | _RC_CHOP` (`0x0f7f`) and the exported API sees `0x027f`, so a mesh row reached from inside the step runs under a different floating-point environment from the same row reached through a query. Phase 4 owns the mesh column's data and its own acceleration-traversal dispatch: drive each reconstructed row under both words and record which one it executes under, and expect recovering a table to move rows an earlier phase already closed rather than treating that as a regression.
- [ ] Carry the `third_party_container_rows_moved` escalation in `gates/phase2.json`. Four rows Phase 2 closed on static proofs are OPCODE `Ice/IceContainer.cpp` members and are now Phase 4's; they stand at `reconstructed` with no ledger behind them. Re-close all four here, against whatever a vendored `IceContainer.cpp` supplies, and keep the local modifications the escalation names, because a stock tree will compile without them and be wrong at every borrowed-buffer call site. There are **four** guarded members, not three, and they are guarded two different ways: `Empty` (`0x000b4d93`), `~Container` (`0x000b4f53`) and `SetSize` (`0x000b4e93`) each carry the same inlined `fld [this+0xc]; fcomp 0.0f; fnstsw ax; test ah,1; jne` skip-the-free, so they free when `mGrowthFactor >= 0.0f`; `Resize` (`0x000b4de7`) is instead an early `return false` under `test ah,0x41; jp`, so it refuses unless `mGrowthFactor > 0.0f` and refuses at exactly `0.0f` as well. `0x000b4f90` is the added external-buffer member that installs `-1.0f` as the marker. `RadixSort` carries the same pattern with a separate ownership byte -- see the Task 1c report §4a for `0x000e3ea0`, `0x000e32c0` and `0x000e32e0` -- and none of those three is vendorable either.
- [ ] Commit evidence and tests.

### Task 2: Reconstruct triangle mesh and convex runtime objects

**Files:**
- Create: `D:\github\Novodex\Physics\src\TriangleMesh.cpp`
- Create: `D:\github\Novodex\Physics\src\ConvexMesh.cpp`
- Create: `D:\github\Novodex\Physics\src\SpatialTree.cpp`
- Create: matching private headers under `Physics\src\include`

- [ ] Define private layouts with measured offsets and static assertions.
- [ ] Reconstruct load/validate/create/reference/release before spatial queries.
- [ ] Reconstruct acceleration traversal, triangle/vertex access, material indices, bounds, mass properties, and overlap/raycast callers in dependency order.
- [ ] Compare exact hit sets, ordering, barycentrics, normals, feature indices, mutations, and allocation/free transcripts.
- [ ] Commit one asset component at a time with paired evidence.

### Task 3: Reconstruct PMap exports and remaining spatial assets

**Files:**
- Create: `D:\github\Novodex\Physics\src\PMap.cpp`
- Create: `D:\github\Novodex\Physics\src\include\PMap.h`
- Modify: `D:\github\Novodex\tests\PhysicsAssetTests.cpp`

- [ ] Add exact RED coverage for `NxCreatePMap` and `NxReleasePMap`: valid density, invalid density, degenerate input, allocation failure, output bytes, ownership, and release.
- [ ] Reconstruct all owned PMap stable IDs and associated data tables with exact allocator and failure rollback behavior.
- [ ] Close remaining Phase 4 spatial/asset rows only after all readers and destructors agree on layout.
- [ ] Re-run Phase 3 collision queries that consume reconstructed assets.

### Task 4: Close Phase 4

```powershell
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsAssetTests NxPhysicsCollisionTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 4
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: Phases 1-3 remain pass; every Phase 4 row closed; PMap exports exact; asset formats have zero unexplained fields used by code; lifecycle tests show no unmatched allocation/reference.

Commit: `docs: close Physics mesh and asset gates`.
