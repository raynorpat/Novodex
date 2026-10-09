# NxPhysics Portable Scalar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Execution method is selected by the user after review.

**Goal:** Add portable scalar implementations while permanently preserving x87 for Win32 32-bit production builds.

**Architecture:** Select one backend at build time. Keep existing x87 bodies and ABI conventions on Win32; use typed scalar implementations elsewhere, with a Win32 portable test override. Validate both backends independently and compare portable behavior against immutable reference fixtures.

**Tech Stack:** Existing C++11 engine, C-compatible scalar math, CMake 3.16+, MSVC Win32 reference, GCC/Clang portable kernel runners.

**Spec:** [Approved design](../specs/2026-10-09-nxphysics-portable-scalar-design.md).

## Global Constraints

- Win32 32-bit production builds continue to select x87; other targets select portable scalar code.
- Preserve the public C++ API.
- Maintain both backends and their respective validation suites.
- Preserve Win32 x87 instructions, calling conventions, control-word behavior, and relevant compiler options.
- Do not weaken historical assertions to make the new implementation pass.
- Do not simply disable layout assertions to obtain a successful compilation.
- Viewer/graphics porting is outside this scope.
- Portable tolerances are measured and fixed before acceptance, not chosen afterward to hide discrepancies.
- Existing unrelated working-tree changes are not part of these commits. Execute in an isolated checkout at implementation time.

## Review Focus

- A Win32 build silently selecting scalar code: Task 2 tests default and override selection explicitly.
- Assembly helpers reached through function-pointer tables: Tasks 1 and 5 audit indirect consumers and exercise dispatch.
- Float-to-integer overflow and nonfinite input: Task 3 checks before conversion and preserves defined failure behavior.
- Caller rounding state leaking across scene exits: Task 7 tests normal, early-return, and nested entry paths.
- Small arithmetic differences changing contact decisions: Tasks 4–8 test discrete decisions and boundary reproducers, not just numeric tolerances.

## File structure and ownership

Existing `.cpp` files retain public entry points and x87 bodies. Small portable alternatives belong beside their x87 implementation under a single backend predicate. Large all-assembly families receive separate files under `Physics/src/portable/`; CMake must explicitly select these because its current source glob does not recurse into that directory. Never link both definitions of the same exported entry.

New shared files:

- `cmake/NxPhysicsBackend.cmake`: backend selection, capability checks, and portable floating-point flags.
- `Physics/src/include/NxPhysicsBackend.h`: build-defined backend predicate with no compiler-only fallback selection.
- `Physics/src/include/portable/NxScalarMath.h`: dependency-light scalar arithmetic; no SDK object layout dependencies.
- `tests/portable/CMakeLists.txt`: independently configurable portable kernel tests, also usable by the main build.
- `tests/portable/FixtureSupport.h` and `.cpp`: fixture decoding and acceptance comparison.
- `tests/portable/fixtures/manifest.json`: schema version, fixture hashes, reference revision/toolchain and numeric budgets.
- `docs/reconstruction/novodex-physics/evidence/portable-scalar-inventory.md`: dependency ledger and recovered interface contracts.
- `docs/reconstruction/novodex-physics/evidence/portable-scalar-acceptance.md`: measurements and discrepancy dispositions.
- `docs/reconstruction/novodex-physics/evidence/portable-platform-blockers.md`: separate platform-enablement backlog.

## Verification conventions

All commands run from the repository root unless `--test-dir` is specified. `build/x87`, `build/portable-win32`, and `build/portable-kernels` are new isolated build directories, not existing user build trees. On a machine with a Visual Studio CMake generator, configure the first two using `-A Win32`; record the actual generator/version in the report. Do not assume a specific installed Visual Studio release.

New tests use CTest names beginning `Portable.` or `Backend.` and executables named `NxPortable*Tests`. A successful command means exit code zero and at least the expected named tests executed; an empty selection is not a pass. Existing legacy harnesses are not assumed to be registered in CTest: record and run their actual repository commands during Task 1, including required DLL/fixture arguments.

Each code task follows a failing-test, implementation, passing-test cycle and ends with a focused commit containing only its files. Register the new test in the appropriate CMake file in the same task. Commands below describe future targets; they have not been executed during planning.

### Task 1: Audited inventory and reproducible fixtures

**Files:** Create the inventory and blocker documents above; create `tests/portable/FixtureSupport.h`, `.cpp`, `FixtureSupportTests.cpp`, `CMakeLists.txt`, and `fixtures/manifest.json`. Read existing collision, kernel fuzz, geometry, actor, joint, and scene harnesses and reconstruction contracts.

**Interfaces:** Fixture reader uses `bool nxReadFixture(const char* path, std::vector<unsigned char>& bytes, std::string& error)`; malformed/truncated inputs return false with a diagnostic. Numeric comparator is `bool nxWithinBudget(double actual, double reference, double absoluteBudget, double relativeBudget)`, finite-only. Callers handle special-value classes separately.

- [ ] Inventory assembly instructions, naked functions, register arguments, `/arch:IA32`, FPU control operations, and fallback stubs across Physics and reachable Foundation/vendor dependencies. Record exact symbols, source locations, data layouts, direct/indirect callers, backend strategy, and existing tests. Do not interpret the initial 33-file search as complete coverage.
- [ ] Record the working x87 configure/build/test commands and run them without changing baselines. Record existing failures separately; stop a affected family's conversion until its baseline can distinguish new regressions.
- [ ] Write `FixtureSupportTests.cpp` asserting truncated input fails with a diagnostic; finite comparator cases include exact equality, near-zero absolute tolerance, relative tolerance, and rejection of NaN/infinity. Confirm failures before implementing the helpers.
- [ ] Implement the helpers and standalone test configuration. Use fixed-width encodings and explicit byte order; never serialize pointers or native object padding into portable fixtures.
- [ ] Capture fixed input bytes and semantic outputs from existing x87 harnesses with minimal fixture-export adapters. Record oracle versus reconstructed-reference provenance separately. Verify repeat exports have identical hashes. Do not generate nonfinite inputs through floating-point arithmetic.
- [ ] Measure baseline sensitivity and pin per-family, per-unit budgets in the manifest before accepting that family's translation. Record justified boundary-case classifications; an unmeasured family cannot pass acceptance.
- [ ] Run `cmake -S tests/portable -B build/portable-kernels`, then build and run `ctest --test-dir build/portable-kernels --output-on-failure -R Portable.Fixture`. Commit fixture infrastructure and evidence.

### Task 2: Permanent backend selection and guard tests

**Files:** Create `cmake/NxPhysicsBackend.cmake`, `Physics/src/include/NxPhysicsBackend.h`, `tests/portable/BackendSelectionTests.cmake`; modify root `CMakeLists.txt` and relevant source-selection blocks in `External/CMakeLists.txt`.

**Interfaces:** CMake cache option `NOVODEX_TEST_PORTABLE_WIN32` defaults OFF. Target definition `NX_PHYSICS_USE_X87` is exactly 0 or 1. `NOVODEX_TEST_PORTABLE_WIN32=ON` is valid only for Win32 32-bit test builds. Backend selection is based on target platform and pointer size, not host platform.

- [ ] Add configure tests for Win32-32 default => x87, Win32-32 test override => portable, Windows-64/Linux/macOS => portable, and invalid override => explicit error. Verify tests fail before implementing selection.
- [ ] Implement selection and an x87 compiler capability check. Unsupported Win32 toolchains fail clearly. Do not silently substitute portable code for the requested production backend.
- [ ] Guard architecture/compiler flags and source definitions by backend. Preserve existing x87 flags; apply strict portable FP options only to portable targets/sources. Audit Foundation/vendor flags without changing upstream vendor files; use the established `novodex/` overlay if needed.
- [ ] Configure `cmake -S . -B build/x87 -A Win32 -DNOVODEX_BUILD_VIEWER=OFF` and `cmake -S . -B build/portable-win32 -A Win32 -DNOVODEX_BUILD_VIEWER=OFF -DNOVODEX_TEST_PORTABLE_WIN32=ON`. During migration the portable full build may remain incomplete; never disguise it with stubs.
- [ ] Run selection tests and the existing x87 harness commands from Task 1. Commit selection logic. Full portable linking is a later gate, not claimed here.

### Task 3: Shared scalar math and explicit conversions

**Files:** Create `Physics/src/include/portable/NxScalarMath.h`, `tests/portable/ScalarMathTests.cpp`; modify `Physics/src/include/X87Sqrt.h`, `Physics/src/include/core/JointAcos.h`, and inventory-listed conversion sites such as `Quantizer.cpp` and `core/SceneDump.cpp`.

**Interfaces:** Preserve existing helper names/signatures for callers of `X87Sqrt.h` and `JointAcos.h`; dispatch their bodies using `NX_PHYSICS_USE_X87`. Scalar helper header must compile without `Nxp.h`. Conversion-specific signatures and rounding rules are recorded per existing caller in Task 1; do not invent a single conversion policy for all `fistp` sites.

- [ ] Add fixtures for every shared helper, including grouped sums/products, negative root domains, endpoint acos, signed zero, very small norms, and large finite inputs. For conversions test the destination limits, values just outside them, infinities, and NaNs.
- [ ] Confirm the portable tests expose missing helpers or current unsupported behavior.
- [ ] Implement standard math bodies with double intermediates and deliberate float stores. Make out-of-range conversion checks precede casts. Retain the x87 bodies unchanged under their backend guard.
- [ ] Build standalone kernels and run `ctest --test-dir build/portable-kernels --output-on-failure -R Portable.ScalarMath`. Compile that target with GCC or Clang on an available runner; record unavailable runners rather than claiming a pass.
- [ ] Run affected exact x87 tests, review floating-point compiler flags, and commit.

### Task 4: Geometry and small assembly islands

**Files:** Modify `Geometry.cpp`, `Distance.cpp`, `SmoothNormals.cpp`, `ShapeRaycast.cpp`, `PMap.cpp`, and other small-island entries in the inventory. Paths are under `Physics/src/`. Create `tests/portable/GeometryTests.cpp`.

**Interfaces:** Preserve exported declarations. Internal helpers gain typed arguments only on the portable path; document any changed private contract in the inventory before converting its callers.

- [ ] Add portable fixture tests for ray/triangle, distance, normalization, and shape-raycast branches with hit/miss assertions away from boundaries and stored touching/degenerate reproducers. Assert output initialization and failure-path behavior as well as numeric values.
- [ ] Run `Portable.Geometry` to demonstrate the uncovered/missing portable cases.
- [ ] Translate assembly islands one independently testable kernel at a time, preserving subtraction direction, branch conditions, grouping, and explicit stores. Add meaningful tests for any existing scalar fallback before reusing it.
- [ ] Run `ctest --test-dir build/portable-win32 -C Release --output-on-failure -R Portable.Geometry` once the target's dependency subset builds; use standalone kernel targets earlier when possible. Run affected legacy geometry/collision tests after every kernel batch and commit each batch.

### Task 5: Mesh/support interfaces and complete convex routines

**Files:** Existing `Physics/src/TriangleMeshPolygons.cpp`, `IceSupportMaps.cpp`, `IceMeshTools.cpp`, `IceMeshBuilder2.cpp`, `EdgeList.cpp`, `ConvexHull.cpp`, `TriangleMeshTopology.cpp`, `ContactConvexConvex.cpp`, `ContactConvexHeightfield.cpp`; create portable counterparts for all-assembly units under `Physics/src/portable/`, `Physics/src/include/portable/NxConvexInterfaces.h`, and `tests/portable/ConvexContactTests.cpp`.

**Interfaces:** Public collision entries retain signatures from `Physics/src/include/ContactGeneration.h`. `NxConvexInterfaces.h` records ordinary typed equivalents of recovered polygon/support callbacks, including receiver, scratch state, transforms, axis, and output pointers. Exact types and parameter order come from Task 1's register/stack and function-table audit; do not infer ABI contracts from parameterless naked declarations.

- [ ] Before translating any function, write its typed interface declaration and a fixture call exercising every recovered argument; verify slot consumers and producers agree. This is a prerequisite for the next step, not a license to guess missing arguments.
- [ ] Split this stage into reviewable commits: polygon access/dispatch, support-map helpers, mesh topology helpers, convex-convex, convex-mesh, then convex-heightfield. Each commit gets its own failing fixture tests before implementation.
- [ ] Test shared edges, parallel faces, winding, touching versus separation, containment, invalid/empty inputs where supported, and nonuniform scales supported by the API. Exercise real function-table dispatch, contact sink writes, and scratch reuse across successive calls.
- [ ] Implement structured portable source with explicit parameters and scratch ownership. Replace `_chkstk` and linker-name aliases with ordinary compiler-managed stack allocation and declared calls, without changing x87 paths. Select one definition of each public symbol in CMake.
- [ ] Run `Portable.ConvexContact` plus exact legacy mesh/convex suites after each family; compare contact geometry and discrete decisions, not incidental contact order. Commit each passing family separately.

### Task 6: Remaining contact and object helpers

**Files:** Inventory entries in `Physics/src/ContactBoxMeshICE.cpp`, `ContactCapsuleMesh.cpp`, `ContactMeshHeightfield.cpp`, `ContactMeshMesh.cpp`, `ContactSphereMesh.cpp`, `ContactGeneration.cpp`, `ContactPairManager.cpp`, `ObjectModel.cpp`, `TriangleMesh.cpp`, and `SceneVisualize.cpp`; create `tests/portable/ContactDispatchTests.cpp`.

**Interfaces:** Preserve shape-pair dispatch and contact-sink contracts; consume Task 5's typed support interfaces. Do not change native object layouts in this stage.

- [ ] Add one fixture per supported shape-pair dispatch arm, including reversed ordering where applicable, and verify it invokes the portable implementation. Add canaries around output buffers and repeated scratch reuse cases.
- [ ] Confirm missing portable arms fail explicitly; translate inventory-listed assembly and ABI bridges without substituting contact stubs.
- [ ] Run `Portable.ContactDispatch` and prior portable families, then the affected exact x87 suites. Review the inventory to ensure every remaining contact entry has real coverage; commit per independently tested family.

### Task 7: Bodies, joints, and scene floating-point state

**Files:** Modify inventory entries in `Physics/src/BodyCreation.cpp`, `BodyStep.cpp`, `NpActor.cpp`, `Scene.cpp`, `Physics/src/include/NpActorDynamicMath.h`, and `Physics/src/core/{D6Joint,RevoluteJoint,SphericalJoint,PulleyJoint,SpringAndDamperEffector}.cpp`; create `Physics/src/include/portable/NxScopedFloatEnvironment.h`, `tests/portable/DynamicsTests.cpp`, and `FloatEnvironmentTests.cpp`.

**Interfaces:** `NxScopedFloatEnvironment` has constructor, destructor, and `bool valid() const`; it is noncopyable. It saves/restores the caller environment using standard fenv facilities and establishes round-to-nearest. Guard actual outer portable API entry points, including simulation and public geometry entry paths; nested calls restore their parent's state. If environment setup fails, route through the engine's existing error path before mutating simulation state.

- [ ] Add body integration and joint residual fixtures, zero-angular-velocity and near-limit cases, sleep/wake transitions, and externally changed rounding modes. Assert caller mode restoration after normal and early exits and nested guards; inject setup failure to test the existing error path.
- [ ] Confirm failures, then implement portable dynamics and fenv guards. Keep legacy `fnstcw`/`fldcw` behavior behind the x87 predicate. Validate compiler support/options for dynamic rounding before relying on the guard.
- [ ] Build the complete Win32 portable engine and run `Portable.Dynamics` and `Portable.FloatEnvironment` in Debug and Release. Run legacy actor, body, joint, and scene harnesses from Task 1 and commit passing groups.

### Task 8: Multistep acceptance and backend regression coverage

**Files:** Create `tests/portable/SimulationTests.cpp`; update portable CMake and acceptance evidence. Extend the existing CI mechanism if present; otherwise document runnable commands instead of introducing an assumed CI provider.

**Interfaces:** Simulation fixtures pin initial scene, step count, timestep, reference provenance, and per-scenario metric budgets in the manifest. Test-only runners construct scenes through public APIs and use production collision/solver paths.

- [ ] Add deterministic initial scenes for resting stacks, joint chains, friction, CCD, sleep/wake, and convex/mesh contacts. Assert penetration and constraint budgets, bounded drift, and absence of unexpected nonfinite state; account for damping/external work in energy checks.
- [ ] Confirm tests detect a deliberately perturbed result locally, then remove the perturbation. Do not compare chaotic long-run poses bitwise.
- [ ] Run all portable tests in Debug and Release and all baseline legacy commands. Record performance and discrepancy dispositions. Any increased tolerance requires an investigation and documented decision.
- [ ] Audit portable preprocessed sources and linkage for live asm, naked routines, raw calling-convention bridges, and selected stubs. Audit Win32 selection to prove x87 remains compiled, with its flags and exact tests. A raw source grep is insufficient because retained assembly is expected.
- [ ] Update the inventory to account for every entry and commit the acceptance report. Do not remove the retained Win32 backend.

### Task 9: Platform-readiness handoff

**Files:** Update `portable-platform-blockers.md`; inspect `Foundation/include/NxSimpleTypes.h`, `Physics/src/include/ObjectModel.h`, `Physics/include/Nxp.h`, `External/CMakeLists.txt`, Foundation sources, and vendor overlays.

**Interfaces:** This deliverable is a dependency-ordered follow-on backlog, not a claim of full Linux/macOS support.

- [ ] Run standalone portable kernel tests on Linux x64 and macOS arm64 when runners are available. Record exact toolchains and results; missing runners remain explicit limitations.
- [ ] Attempt full-engine configuration/build on those targets and classify failures: pointer/native layout, packed file formats, type/platform macros, platform APIs, compiler/linker conventions, or vendor integration. Preserve actual diagnostics.
- [ ] Record concrete affected symbols/files and proposed acceptance tests for each blocker. In particular, distinguish historical raw 32-bit object access from serialized 32-bit fields, and audit the Apple bool assumption without disabling checks wholesale.
- [ ] Produce a separate platform-enablement design/plan if layout changes are required. Run `git diff --check`, review evidence against the approved spec, and commit the handoff document.

## Completion and execution handoff

Tasks 1–8 deliver a maintained Win32 x87 backend and tested portable scalar counterpart, first proven on Win32 to isolate arithmetic changes. Task 9 establishes what remains for full native Linux/macOS engine builds. Full engine platform support is only complete after those additional blockers are resolved and platform integration tests execute.

Recommend subagent-driven execution with review per independently testable family because recovered register interfaces and discrete collision behavior are high-risk. Native execution is also available. The user reviews this plan and chooses execution before any product changes begin.
