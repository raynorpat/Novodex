# Novodex Physics Full Reconstruction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete a semantic full reconstruction of the UE3-shipped Win32 Release `NxPhysics.dll` in Foundation-style C++ with exhaustive Ghidra/Capstone provenance, ABI parity, subsystem differential gates, and a controlled Unreal_3 drop-in proof.

**Architecture:** An evidence-first pipeline assigns every oracle function and significant data object a stable ID, reconciles Ghidra semantics with independent Capstone disassembly, and builds a dependency graph. Eight phase plans reconstruct leaf dependencies upward into SDK objects, collision, assets, actors, joints, scenes, and simulation; a row cannot close without static provenance and the strongest applicable dynamic proof.

**Tech Stack:** MSVC Win32 Release; CMake; C++ in the established Novodex Foundation style; Python 3 PE/evidence tools; Ghidra headless analysis; Capstone x86; PowerShell isolated-process differential runners; Unreal_3 consumer smoke.

**Spec:** `docs/superpowers/specs/2026-08-09-novodex-physics-reconstruction-design.md`

---

## Repositories and immutable pins

- Implementation: `D:\github\Novodex`
- Evidence: this Unreal_3 tree under `docs/reconstruction/novodex-physics/`
- Oracle: `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
- Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
- Oracle size: `1253376`
- Oracle named exports: `41`
- Link oracle: `Development\External\Novodex\Physics\lib\win32\Release\NxPhysics.lib`
- Link-oracle SHA-256: `a99cddd2057b18407294f1621c1938be5f304c3712b03c167d9c8bbfded83c47`
- Installed Foundation oracle: `D:\FlamingEnt__\Unreal_3\Binaries\NxFoundation.dll`
- Installed Foundation SHA-256: `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`
- Public headers: `Development\External\Novodex\` at SDK identity `2.1.2.6000`
- Foundation dependency: reconstructed `D:\github\Novodex` Foundation implementation, with the exact execution-start commit recorded before Phase 1.

Public Novodex headers are immutable. Physics product code must be maintainable C++ matching `Foundation/src` conventions. Ghidra pseudocode and generated C are evidence only.

## Phase plans

| Phase | Plan | Required terminal gate |
|---|---|---|
| 1 | `2026-08-09-novodex-physics-phase1-oracle-census.md` | Every executable oracle byte classified; stable-ID inventory and dependency graph validate |
| 2 | `2026-08-09-novodex-physics-phase2-sdk-core.md` | DLL and SDK lifecycle, allocation, errors, registries, factories, and shared infrastructure close |
| 3 | `2026-08-09-novodex-physics-phase3-geometry-collision.md` | Geometry, broad/narrow phase, queries, filtering, and contact primitives close |
| 4 | `2026-08-09-novodex-physics-phase4-mesh-assets.md` | Meshes, convex data, PMaps, acceleration data, ownership, and runtime queries close |
| 5 | `2026-08-09-novodex-physics-phase5-objects.md` | Actors, bodies, shapes, materials, descriptors, mass/inertia, and lifecycle close |
| 6 | `2026-08-09-novodex-physics-phase6-joints-effectors.md` | All recovered joint families and effectors close |
| 7 | `2026-08-09-novodex-physics-phase7-scenes-simulation.md` | Scene lifecycle, stepping, islands, constraints, solver, sleeping, and callbacks close |
| 8 | `2026-08-09-novodex-physics-phase8-full-audit.md` | Entire census closed; full ABI/static/differential/trajectory/consumer gates pass |

Phases run in order. A later phase may add a stronger regression for an earlier subsystem, but it may not weaken or silently reclassify a closed proof.

### Named-export ownership lock

All 41 named oracle exports have an initial phase owner. Phase 1 may refine the owning source component but may not leave an export unowned.

| Phase | Exports |
|---|---|
| 2 | `NxCreatePhysicsSDK`; `NxFluidAssert`; `NxFluidDebugAABB`; `NxFluidDebugArrow`; `NxFluidDebugLine`; `NxFluidDebugPoint`; `NxFluidDebugSphere`; `NxFluidDebugTriangle`; `NxFluidFree`; `NxFluidPAlloc` |
| 3 | `NxBoxBoxIntersect`; `NxBuildSmoothNormals`; `NxComputeBoxDensity`; `NxComputeBoxInertiaTensor`; `NxComputeBoxMass`; `NxComputeConeDensity`; `NxComputeConeMass`; `NxComputeCylinderDensity`; `NxComputeCylinderMass`; `NxComputeEllipsoidDensity`; `NxComputeEllipsoidMass`; `NxComputeSphereDensity`; `NxComputeSphereInertiaTensor`; `NxComputeSphereMass`; `NxRayAABBIntersect`; `NxRayAABBIntersect2`; `NxRayCapsuleIntersect`; `NxRayOBBIntersect`; `NxRayPlaneIntersect`; `NxRaySphereIntersect`; `NxRayTriIntersect`; `NxSegmentAABBIntersect`; `NxSegmentBoxIntersect`; `NxSegmentOBBIntersect`; `NxSegmentPlaneIntersect`; `NxSeparatingAxis`; `NxSweptSpheresIntersect` |
| 4 | `NxCreatePMap`; `NxReleasePMap` |
| 6 | `NxJointDesc_SetGlobalAnchor`; `NxJointDesc_SetGlobalAxis` |

The export gate compares exact names and ordinals against both the DLL and the pinned import library. Internal virtual API coverage is tracked through vtables and the stable-ID census, not inferred from this small export table.

## Shared evidence contract

`docs/reconstruction/novodex-physics/inventory.json` is the program state. Each `functions[]` row contains:

```json
{
  "id": "phys_fn_000001",
  "rva": "0x00000000",
  "size": 1,
  "kind": "code",
  "label": "phys_fn_000001",
  "label_confidence": "stable-id",
  "section": ".text",
  "phase": 1,
  "state": "discovered",
  "source": null,
  "ghidra_ref": "ghidra/functions/phys_fn_000001.json",
  "capstone_ref": "capstone/phys_fn_000001.json",
  "static_proof": null,
  "dynamic_proof": null,
  "notes": ""
}
```

Allowed function states are exactly `discovered`, `typed`, `decompiled`, `reconstructed`, `statically_reviewed`, `dynamically_gated`, and `closed`. Compiler/runtime artifacts use `kind: compiler_artifact`, retain byte ownership, and require a classification proof rather than product source.

Each `data_objects[]` row uses the same stable-ID principle and records RVA, byte range, type, owner, references, source representation, structural proof, and state. Every row has exactly one phase owner.

## Global gates

Run after every production commit:

```powershell
cmake -S D:\github\Novodex -B D:\github\Novodex\build -A Win32 --fresh
cmake --build D:\github\Novodex\build --config Release --target NxPhysics --clean-first
python docs\reconstruction\novodex-physics\tools\validate_inventory.py docs\reconstruction\novodex-physics\inventory.json
powershell -NoProfile -ExecutionPolicy Bypass -File docs\reconstruction\novodex-physics\tools\run_phase_gate.ps1 -Phase completed
```

`-Phase completed` reads `program.json`, finds the highest phase marked `pass`, and runs every gate through that phase. Expected: all commands exit `0`; no public-header hash changes; all rows owned by completed phases are `closed`; no earlier regression fails.

Exact output comparison is the default. A tolerance requires a checked-in repeated-oracle measurement and numerical rationale. Oracle and candidate calls run in separate processes from isolated, hash-verified DLL-pair directories; every child transcript must prove the loaded `NxPhysics.dll` and `NxFoundation.dll` paths and hashes match its pair.

## Program execution protocol

### Task 1: Record clean starting state

**Files:**
- Create: `docs/reconstruction/novodex-physics/program.json`

- [ ] **Step 1: Verify implementation and evidence repository states**

```powershell
git -C D:\github\Novodex status --short --branch
git status --short --branch
```

Expected: the Physics execution worktree is clean; unrelated primary-worktree changes are not used.

- [ ] **Step 2: Record exact commits and pins in `program.json`**

The file contains `schema_version`, `oracle`, `link_oracle`, `header_root`, `foundation_commit`, `evidence_commit`, `phases`, and `global_gates`. Set all eight phases to `pending` and all gates to `pending`.

- [ ] **Step 3: Commit the program record**

```powershell
git add docs/reconstruction/novodex-physics/program.json
git commit -m "docs: pin Novodex Physics reconstruction inputs"
```

### Task 2: Execute phases with hard checkpoints

**Files:**
- Modify: `docs/reconstruction/novodex-physics/program.json`

- [ ] **Step 1: Execute Phase 1 and require its terminal gate**

Use `docs/superpowers/plans/2026-08-09-novodex-physics-phase1-oracle-census.md`. Do not begin product reconstruction until unexplained executable bytes and duplicate byte ownership are both zero.

- [ ] **Step 2: Execute Phases 2 through 7 in order**

After each phase, record its implementation commit, evidence commit, gate artifact, closed function/data counts, and remaining counts in `program.json`. The next phase begins only after the previous phase status is `pass`.

- [ ] **Step 3: Execute Phase 8 without deploying during intermediate failures**

Deployment is permitted only after clean build, ABI, inventory, structural, static, differential, and trajectory gates pass on the exact retained candidate.

- [ ] **Step 4: Commit each phase transition**

Use the exact close-out commit message stated at the end of the completed phase plan. Stage only that phase's evidence plus `program.json`; do not combine evidence from the next phase.

### Task 3: Close the umbrella program

**Files:**
- Modify: `docs/reconstruction/novodex-physics/program.json`
- Create: `docs/reconstruction/novodex-physics/closeout.md`

- [ ] **Step 1: Verify the retained Phase 8 pair from clean commits**

Run Phase 8 `VerifyRetained -NoWrite` against the exact consumer-tested Foundation/Physics pair; do not rebuild. Expected: all eight phases `pass`; both retained hashes unchanged; all inventory rows closed; unexplained bytes `0`; missing exports `0`; public-header drift `0`; all required dynamic suites pass.

- [ ] **Step 2: Verify consumer restoration**

```powershell
Get-FileHash D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll -Algorithm SHA256
Get-FileHash D:\FlamingEnt__\Unreal_3\Binaries\NxFoundation.dll -Algorithm SHA256
Get-ChildItem D:\FlamingEnt__\Unreal_3\Binaries -Filter 'NxPhysics.dll*.bak'
Get-ChildItem D:\FlamingEnt__\Unreal_3\Binaries -Filter 'NxFoundation.dll*.bak'
Get-Process DemoGame -ErrorAction SilentlyContinue
```

Expected: Physics oracle hash `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; Foundation oracle hash `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`; no backup; no test process.

- [ ] **Step 3: Write `closeout.md`**

Record exact commits, both Foundation and Physics candidate hashes, proof counts, gate artifacts, consumer result, restoration result, and explicit proof limitations. Do not claim exhaustive coverage of all possible scene inputs.

- [ ] **Step 4: Commit**

```powershell
git add docs/reconstruction/novodex-physics/program.json docs/reconstruction/novodex-physics/closeout.md
git commit -m "docs: close Novodex Physics reconstruction gates"
```

## Plan-package coverage

| Design requirement | Plan |
|---|---|
| Complete Ghidra/Capstone census | Phase 1 |
| Foundation-style C++ and shared runtime | Phase 2 and all product phases |
| Geometry and collision | Phase 3 |
| Mesh/spatial assets | Phase 4 |
| Actors/bodies/shapes/materials | Phase 5 |
| Joints/effectors | Phase 6 |
| Scenes/simulation/solver | Phase 7 |
| Full static, differential, trajectory, and consumer proof | Phase 8 |
| Exact restoration and close-out | Phase 8 plus umbrella Task 3 |
