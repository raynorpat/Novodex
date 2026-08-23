# Novodex Physics Phase 2 SDK Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct the Win32 DLL lifecycle, `NxCreatePhysicsSDK`, SDK implementation, allocation/errors, registries, factories, containers, RTTI, and shared runtime rows owned by Phase 2.

**Architecture:** Transplant immutable UE3 Physics headers, establish a loadable Foundation-linked DLL, then close Phase 2 dependency components leaf-first. C++ private types mirror recovered layouts and Foundation conventions; oracle/candidate harnesses run in separate processes.

**Tech Stack:** MSVC x86 C++; CMake; reconstructed NxFoundation; Ghidra/Capstone Phase 1 inventory; PE export and structural probes; PowerShell differential runner.

---

### Task 1: Transplant and verify the UE3 Physics public API

**Files:**
- Replace recursively: `D:\github\Novodex\Physics\include\**\*.h`
- Read: `docs/reconstruction/novodex-physics/public_header_hashes.json`
- Read: `docs/reconstruction/novodex-physics/tools/verify_public_headers.py`
- Test: `docs/reconstruction/novodex-physics/tools/tests/test_public_headers.py`

- [ ] Run the Phase 1 public-header tests and verify the pinned UE3 source tree still matches its recursive manifest before transplant.
- [ ] Recursively replace `Physics/include` with the exact directory topology and contents of `Development/External/Novodex/Physics/include`, including `fluids` and every nested directory; do not merge the old headers.
- [ ] Verify the recursive Novodex copy against the already committed Phase 1 manifest, including missing and extra paths, then commit only the Novodex header replacement with `physics: transplant UE3 2.1.2 public headers`. The evidence worktree must remain unchanged.

Verification:

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --manifest docs/reconstruction/novodex-physics/public_header_hashes.json --root D:\github\Novodex\Physics\include
```

Expected: `public_headers=pass` then `files=80`.

### Task 2: Establish the Phase 2 build and loader RED gate

**Files:**
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `D:\github\Novodex\tests\PhysicsExportTests.cpp`
- Create: `D:\github\Novodex\tests\PhysicsSDKTests.cpp`
- Create: `docs/reconstruction/novodex-physics/tools/run_differential.ps1`
- Create: `docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1`

- [ ] Add Win32-only `NxPhysicsExportTests` and `NxPhysicsSDKTests` targets. Each takes an absolute isolated DLL-pair directory, calls `SetDefaultDllDirectories`/`AddDllDirectory` (or the proven equivalent on the target Windows version), and loads `NxPhysics.dll` from that directory with explicit search flags. It rejects Novodex or other non-system dependencies outside the pair directory while allowing an explicit recorded set of trusted Windows modules loaded from the Windows system directory.
- [ ] Make `PhysicsExportTests` verify PE32 load, all Phase 2 expected exports, absence of unresolved imports, and exact loaded absolute paths plus SHA-256 identities for both `NxPhysics.dll` and `NxFoundation.dll`.
- [ ] Make `PhysicsSDKTests` serialize normalized results for exact version, wrong version, null/default allocator/output combinations, create, release, recreate, and callback transcript.
- [ ] Make `run_differential.ps1` create two external isolated directories under `D:\FlamingEnt__\Unreal_3\.analysis\novodex-physics\pairs`: oracle contains the pinned shipped Physics/Foundation pair; candidate contains the exact current rebuilt Physics/Foundation pair. It verifies all four source and staged hashes, passes only the pair directory to each child, and rejects a child unless its transcript reports both expected loaded paths and hashes.
- [ ] Make `run_phase_gate.ps1` accept phases `1` through `8` plus `completed`; `completed` reads `program.json`, runs immutable-header, inventory, build, export, isolated-pair identity, and every registered differential gate through the highest passing phase, and rejects an unknown/missing test target.
- [ ] Build and run against the empty Physics target. Expected RED: `NxCreatePhysicsSDK` missing or SDK creation unavailable.
- [ ] Commit the red harnesses separately from production code.

### Task 3: Reconstruct DLL and SDK lifecycle

**Files:**
- Create: `D:\github\Novodex\Physics\src\DllMain.cpp`
- Create: `D:\github\Novodex\Physics\src\PhysicsSDK.cpp`
- Create: `D:\github\Novodex\Physics\src\include\PhysicsSDK.h`
- Create: `D:\github\Novodex\Physics\src\include\PhysicsInternal.h`
- Modify: `D:\github\Novodex\Physics\src\win32\resource.rc`

- [ ] Select the Phase 2 rows the exported `NxCreatePhysicsSDK` row reaches, including any mutual recursion among them, and record all Ghidra/Capstone IDs in `evidence/phase2-sdk.md`.
- [ ] Recover the factory signature, version check, singleton/global state, Foundation calls, allocation sizes, constructor order, error callbacks, rollback edges, release order, and DLL attach/detach behavior.
- [ ] Write private C++ classes with explicit recovered fields and `static_assert(sizeof(RecoveredType) == measured_size)` plus `offsetof(RecoveredType, recoveredField) == measured_offset` checks for every measured layout.
- [ ] Implement the minimal complete component, build, and run `PhysicsSDKTests` separately against oracle and candidate.
- [ ] Require exact normalized transcripts; commit production as `physics: reconstruct SDK lifecycle`, then commit evidence.

### Task 4: Reconstruct shared runtime components leaf-first

**Files:**
- Create: `D:\github\Novodex\Physics\src\Allocator.cpp`
- Create: `D:\github\Novodex\Physics\src\ObjectRegistry.cpp`
- Create: `D:\github\Novodex\Physics\src\Factory.cpp`
- Create: `D:\github\Novodex\Physics\src\Containers.cpp`
- Create: `D:\github\Novodex\Physics\src\Serialization.cpp`
- Create: `D:\github\Novodex\Physics\src\FluidSupport.cpp`
- Create: `D:\github\Novodex\Physics\src\SDKParameters.cpp`
- Create: `D:\github\Novodex\Physics\src\Visualization.cpp`
- Create: `D:\github\Novodex\Physics\src\Performance.cpp`
- Create: `D:\github\Novodex\Physics\src\CoreDump.cpp`
- Create: matching private headers under `D:\github\Novodex\Physics\src\include\`
- Create: `D:\github\Novodex\tests\PhysicsCoreClusterTests.cpp`

For each Phase 2 dependency component:

- [ ] Add a RED mode to `PhysicsCoreClusterTests` covering valid lifecycle, capacity growth, removal/reuse order, allocation failure, destruction, and observable callbacks supported by its oracle callers.
- [ ] Run the mode against the oracle and capture the normalized expected transcript.
- [ ] Reconstruct every owned stable-ID function in Foundation-style C++, preserving allocator use, field layout, evaluation order, and destruction order.
- [ ] Compare Ghidra control flow and Capstone calls/branches/constants/memory effects; record each row's static proof.
- [ ] Run oracle and candidate in separate processes; require exact output and mark rows closed only after the component has no open callees in Phase 2.
- [ ] Commit one non-trivial component per production commit and one paired evidence commit.

`FluidSupport.cpp` owns the oracle exports `NxFluidAssert`, `NxFluidPAlloc`, `NxFluidFree`, `NxFluidDebugAABB`, `NxFluidDebugArrow`, `NxFluidDebugLine`, `NxFluidDebugPoint`, `NxFluidDebugSphere`, and `NxFluidDebugTriangle`. Reconstruct their exact allocator, assertion, debug-renderer, geometry-record, and null-input behavior; do not infer a separate fluid-simulation subsystem from these support exports.

The SDK-core gate must also enumerate and disposition every `NxPhysicsSDK` public vtable slot, including parameters, visualization settings, material creation/enumeration, `coreDump`, and performance-inspector behavior. Supported methods receive valid/boundary/failure differential cases; oracle unsupported/error stubs are reconstructed and gated as such. A public slot cannot close merely because UE3 does not call it.

### Task 5: Close Phase 2

**Files:**
- Modify: `docs/reconstruction/novodex-physics/inventory.json`
- Create: `docs/reconstruction/novodex-physics/gates/phase2.json`

```powershell
cmake -S D:\github\Novodex -B D:\github\Novodex\build -A Win32 --fresh
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsExportTests NxPhysicsSDKTests NxPhysicsCoreClusterTests --clean-first
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 2
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: all Phase 2 rows closed; SDK/core differential modes exact; header drift zero; later-phase rows remain explicitly open; earlier census gate remains pass.

Commit: `docs: close Physics SDK core gates`.
