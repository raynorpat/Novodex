# NxPhysics portable scalar conversion

Date: 2026-10-09
Status: Approved in-chat design; written specification awaiting review.

## Intent and compatibility target

Replace NxPhysics's x87 assembly dependencies with portable scalar source so future Linux and macOS builds are feasible. The user selected practical physics equivalence rather than bit-for-bit reproduction of the original Windows binary, and approved the incremental migration described here.

“Standard C” means C-compatible arithmetic, control flow, and math-library operations within the existing C++ engine. Preserve the public C++ API. This work does not convert the SDK into a C API or require every translation unit to compile as C.

Success means equivalent useful physics behavior, explicit numeric assumptions, and a portable implementation that does not require x87 instructions, MSVC inline assembly, or implicit register calling conventions. Exact NaN payloads, extended-precision intermediates, binary layout compatibility with the shipped DLL on other architectures, and cross-platform bitwise determinism are not requirements.

## Repository evidence

- Initial text search found assembly matches in 33 files under `Physics/src`; this is an inventory starting point, not an audited count of live assembly functions.
- `Physics/src/include/X87Sqrt.h` contains shared naked x87 helpers and scalar fallbacks. Its comments document extended precision, rounding, and intermediate storage dependencies.
- `Physics/src/Scene.cpp` saves and changes the x87 control word during simulation and restores it afterward.
- `CMakeLists.txt` applies `/arch:IA32` to numerous translation units, including ordinary C++ whose behavior consequently depends on x87.
- `ContactConvexConvex.cpp`, `ContactConvexHeightfield.cpp`, and `TriangleMeshPolygons.cpp` contain complete naked routines and register-based interfaces. Converting arithmetic alone cannot make these portable.
- Existing collision and oracle tests include Windows DLL loading, raw object layouts, and exact byte comparisons. They cannot simply become the portable acceptance suite unchanged.
- `Physics/src/include/ObjectModel.h` encodes fixed object layouts. `Foundation/include/NxSimpleTypes.h` includes an Apple four-byte `bool` assumption. These are separate barriers to modern 64-bit builds.

## Selected approach

Migrate incrementally, retaining the current Win32 x87 configuration as a temporary reference. Provide an explicit portable build selection usable on Windows before requiring a new operating system. A portable selection must never silently choose legacy assembly or placeholder implementations.

Do not create a permanent software x87 emulator or promise permanent maintenance of two implementations. Keep the legacy reference until the portable validation gates pass; record its source revision and fixtures before retiring it. Do not rewrite unrelated engine architecture as part of numeric conversion.

The portable math layer holds shared operations, explicit integer conversions, and documented storage boundaries. Keep algorithm-specific geometry and solver operations with their current subsystems. Replace implicit register parameters with typed internal parameters and ordinary calls, preserving external API signatures.

## Numeric contract

1. Use existing float-facing storage and double intermediates where the recovered algorithm benefits from them. Preserve operation grouping and intentional float stores; do not replace every intermediate with float or rely on `long double` for consistent precision.
2. Use standard math functions for square root and trigonometry, including reviewed existing scalar fallbacks. Preserve domain handling and observable error paths where those are defined by the API.
3. Target round-to-nearest execution. The implementation must document and test how it establishes or verifies that environment. If it changes caller state, restoration must cover every exit path. Do not carry x87 precision-control changes into the portable implementation.
4. Implement each float-to-integer conversion according to its intended rounding and range behavior. Check nonfinite and out-of-range inputs before casts that would have undefined behavior; preserve documented failure behavior rather than inventing silent clamps.
5. Initially disable fast-math, reassociation, and fused-operation contraction through compiler-appropriate options. Check the emitted build configuration, not just the source expressions. Optimization changes require separate measured validation.
6. NaN payload identity and x87 exception flags are excluded. Still exercise NaN, infinity, signed zero, subnormal, degenerate, and overflow inputs for defined handling and memory safety. Audit any sign-bit or nonfinite branch whose outcome affects control flow.
7. Do not alter geometric epsilons merely to make a differential test pass. Any threshold change requires a reproducer and a justified behavior decision.

## Migration stages and gates

### 1. Audited inventory and baseline

Catalog executable assembly, naked functions, calling conventions, control-word operations, architecture-dependent generated arithmetic, and existing fallback branches. Classify each entry as shared math, conversion, geometry/contact, body/joint/step, or ABI/platform work. Record files, callers, dependencies, and available tests.

Resolve indirect dispatch and continuation routines when ordering conversions; repository reconstruction evidence explicitly warns that direct-call closure is insufficient.

Capture immutable input/output fixtures from a recorded legacy build and source revision. Store input bytes rather than regenerating sensitive inputs using compiler-dependent floating-point arithmetic. Include representative finite inputs, boundary cases, and explicit special-value cases. Preserve existing oracle digest tests.

Gate: every discovered dependency has an owner category, conversion order, and validation route; baseline fixture generation is reproducible.

### 2. Portable math and test foundation

Introduce explicit portable build selection and a small standalone scalar test target that does not depend on the complete engine or Windows DLL loading. Convert shared square-root, trigonometric, normalization, and conversion helpers first. Review existing fallbacks before reuse.

Gate: tests exercise the portable implementation on Windows without falling back to x87, and standalone kernels can be compiled using a non-MSVC compiler when available.

### 3. Geometry and complete assembly routines

Convert support-map, mesh-access, and topology dependencies before their convex collision callers. Recover parameter types, return values, aliasing, temporary storage, and branch behavior from the existing implementation and reconstruction evidence. Translate whole algorithmic routines into structured source rather than retaining simulated CPU registers throughout the code.

Prioritize `TriangleMeshPolygons.cpp`, `IceSupportMaps.cpp`, `Geometry.cpp`, `Distance.cpp`, and the convex contact families according to the audited dependency graph. This list is illustrative; the inventory determines the final task order.

Gate: each converted family passes its kernel fixtures, branch-sensitive contact checks, and integration coverage before callers migrate.

### 4. Dynamics and simulation environment

Convert remaining actor/body arithmetic, joint math, and scene-step dependencies. Audit helpers reached both inside and outside simulation because their previous rounding environments differed. Remove portable-path x87 control-word operations and `/arch:IA32` requirements only after their consumers have migrated.

Gate: portable simulation runs through the actual collision and solver paths, with no hidden assembly dependency or test-only substitute.

### 5. Physics acceptance and default transition

Run kernel and multistep acceptance tests, record differences and performance, and make the portable path the default after the gates below pass. Preserve a reproducible legacy reference revision and fixtures. Remove temporary duplicated implementations only after portable coverage replaces their reference role.

Gate: approved numeric budgets pass, discrete discrepancies are explained, and no supported production path requires x87.

### 6. Separate platform-enablement work

Audit Foundation, OPCODE, qhull integration, export macros, calling conventions, compiler extensions, allocation/alignment, pointer truncation, fixed offsets, serialization, and platform APIs. Separate wire-format widths from native pointer widths. Do not simply disable layout assertions to obtain a successful compilation.

Stage full-engine validation through Windows portable builds, Linux x64, and macOS arm64. Portable kernel compilation should begin earlier so platform issues are discovered before the full port. Modern 64-bit object-layout changes may require a separate design and implementation workstream.

Gate: only claim operating-system support once its full engine builds and its portable integration suite runs on that operating system. Assembly-free kernels alone establish readiness, not a completed platform port. Viewer/graphics porting is outside this scope.

## Validation and acceptance policy

Maintain two distinct suites: historical exact oracle tests for the legacy configuration and behavior-oriented portable tests. Do not weaken historical assertions to make the new implementation pass.

For finite numeric kernel outputs, use a named per-kernel budget of the form `abs(actual - reference) <= absolute_budget + relative_budget * abs(reference)`. Use angular, distance, or residual metrics where componentwise error is misleading. Record units, input scale, and worst cases. Establish and review numeric budgets from baseline measurements before accepting a converted family; do not widen them after a failure without investigating its cause.

Require identical discrete outcomes away from identified geometric ambiguity boundaries. At touching, near-parallel, degenerate, or threshold cases, retain a reproducer and verify valid contact geometry, bounded penetration, and downstream stability. Compare contact sets geometrically where ordering is incidental. A differing boolean or missing contact is never excused solely by a small arithmetic error.

Multistep scenarios cover resting stacks, joint chains, friction, CCD, sleeping/waking, and convex/mesh contacts. Measure constraint residuals, penetration, pose/velocity drift, sleep behavior, and unexpected nonfinite state. Energy checks must account for damping, friction, external work, and impacts. Long chaotic trajectories need bounded behavioral metrics rather than exact final poses.

Use checked-in seeds/fixtures and test debug and optimized builds. Exercise GCC/Clang and supported architectures as runners become available, with memory/undefined-behavior checks where supported. Performance measurements are a regression signal; correctness cannot be traded for speed silently.

Record for each conversion: fixture identity, legacy/reference revision, compiler/options, error maxima, discrete disagreements and disposition, scenario metrics, and timing. Lack of an available target runner must remain an explicit verification limitation.

## Deliverables and completion boundaries

- Audited dependency inventory and immutable reference fixtures.
- Portable scalar math and translated production algorithms with explicit internal interfaces.
- Portable build configuration and independent kernel/integration tests.
- Measured acceptance report with fixed per-family numeric budgets.
- Separate platform-blocker inventory and follow-on scope for full Linux/macOS support.

The assembly-conversion milestone is complete when supported NxPhysics production paths use portable source and pass the behavior contract. The future-platform milestone additionally requires dependency, native-layout, and platform integration work plus execution on each target. Neither milestone requires a C-only SDK, original-DLL ABI compatibility on new architectures, or cross-platform bitwise determinism.

## Review and next step

This specification captures the in-chat approved approach. After review of this written artifact, produce a detailed implementation plan with concrete files, dependency-ordered tasks, verification commands, and explicit separation between numeric conversion and native-platform enablement. No product implementation is authorized by this document alone.
