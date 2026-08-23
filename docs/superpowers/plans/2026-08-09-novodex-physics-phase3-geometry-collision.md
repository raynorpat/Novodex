# Novodex Physics Phase 3 Geometry and Collision Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct all Phase 3 geometry, broad-phase, narrow-phase, query, filtering, and contact-generation functions, including the oracle's exported intersection and mass helpers.

**Architecture:** Close leaf mathematical kernels first, then pair dispatch and contact pipelines. Bit-serialized differential harnesses exercise permutations, degeneracy boundaries, aliasing, and multi-contact ordering; Capstone resolves x87/SSE and branch-sensitive edge cases.

**Tech Stack:** Foundation-style x86 C++; Ghidra/Capstone inventory; deterministic C++ cluster harness; serialized floating-point bit comparisons.

---

### Task 1: Create the geometry differential matrix

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `D:\github\Novodex\tests\PhysicsGeometryTests.cpp`
- Create: `docs/reconstruction/novodex-physics/cases/geometry.json`
- Create: `docs/reconstruction/novodex-physics/evidence/phase3-exports.md`

- [ ] Enumerate Phase 3 stable IDs and the exported rows `NxBoxBoxIntersect`, `NxBuildSmoothNormals`, all `NxCompute*Density/Mass/InertiaTensor`, `NxRay*`, `NxSegment*`, `NxSeparatingAxis`, and `NxSweptSpheresIntersect`.
- [ ] Define fixed hexadecimal float inputs covering hits, misses, tangent cases, reversed direction, zero length where permitted, normalized/non-normalized documented domains, degenerate geometry, output aliasing, and null optional outputs.
- [ ] Serialize return values and every output byte; add metamorphic permutations only as supplemental checks, never as oracle substitutes.
- [ ] Run against the oracle to establish GREEN oracle fixtures and against the current candidate to prove RED.
- [ ] Commit tests/cases/evidence without production implementation.

### Task 2: Reconstruct leaf geometry and mass kernels

**Files:**
- Create: `D:\github\Novodex\Physics\src\Geometry.cpp`
- Create: `D:\github\Novodex\Physics\src\MassProperties.cpp`
- Create: private headers `GeometryInternal.h` and `MassProperties.h`

For each leaf group of Phase 3 rows, mutual recursion taken together:

- [ ] Record Ghidra types and Capstone instruction/control-flow evidence for all owned IDs.
- [ ] Implement clear C++ preserving operation order, float rounding points, comparison strictness, output-write order, and alias behavior.
- [ ] Use a guarded x86 helper only when repeated C++ attempts cannot match a proven x87/SSE lifetime.
- [ ] Run the owning differential modes oracle/candidate separately and require exact bytes unless an approved tolerance artifact exists.
- [ ] Carry the `static_proof_closures_have_no_oracle_side` escalation in `gates/phase2.json`. `phys_fn_002362`, `phys_fn_002364` and `phys_fn_002366` — the SDK lock's `lock`, `tryLock` and `unlock` — are closed against checks derived from the disassembly and have never been observed against the shipped DLL. Phase 3 is the first phase to drive them through real geometry work: if any Phase 3 differential reaches the lock, record what it observed rather than leaning on the derived expectation.
- [ ] Commit production and paired evidence before proceeding.

### Task 3: Reconstruct broad phase, narrow phase, filtering, and contacts

**Files:**
- Create: `D:\github\Novodex\Physics\src\BroadPhase.cpp`
- Create: `D:\github\Novodex\Physics\src\NarrowPhase.cpp`
- Create: `D:\github\Novodex\Physics\src\ContactGeneration.cpp`
- Create: `D:\github\Novodex\Physics\src\Filtering.cpp`
- Create: matching private headers under `Physics\src\include`
- Create: `D:\github\Novodex\tests\PhysicsCollisionTests.cpp`

- [ ] Build the recovered shape-pair dispatch matrix from data tables, indirect call targets, and callers; assert every matrix entry maps to an owned stable ID or explicit unsupported entry.
- [ ] Add RED modes for pair symmetry, separation, penetration, touching, multiple contacts, normal orientation, feature IDs, filtering decisions, ordering, and buffer limits.
- [ ] Reconstruct dependency components leaf-first, preserving dispatch indices, cache mutation, contact ordering, early-outs, and callback timing.
- [ ] Compare complete contact records rather than counts or ranges.
- [ ] Close a component only when all its Phase 3 callees and associated dispatch/data rows are closed.

### Task 4: Close Phase 3

```powershell
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsGeometryTests NxPhysicsKernelFuzzTests NxPhysicsCollisionTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 3
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: Phase 2 remains pass; every Phase 3 function/data row closed; all owned exported functions present and exact on the fixed corpus; dispatch coverage has no unknown entries.

Commit: `docs: close Physics geometry and collision gates`.
