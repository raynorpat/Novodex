# Task 1 report — audited inventory and fixture bootstrap

Status: implemented; awaiting controller review. Worktree `C:/Users/raynorpat/.codex/worktrees/nxphysics-portable/Novodex`, branch `codex/nxphysics-portable`, reference source revision `a4838ddf282ee859471e1526145b616de5ddedb6`. No production files/public headers/historical assertions changed. No main-checkout DLLs or user modifications consumed. Pinned shipped inputs came from `D:/FlamingEnt__/Unreal_3/Binaries` through the existing verified runner.

## Deliverables and rulings

* Whole-tree comment/string-masked dependency census: 57 files, 527 grouped candidate entries across Physics, Foundation and External, exact original source lines, observed reference sites, assembly instructions/call/tail-jump operands, labels, conditional branches, vendor shadowing, SHA-256 source identities. Human ledger assigns owner/order/strategy/test routes and compiler-generated x87 dependencies, with recovered table/layout contracts and explicit unrecovered register-interface obligations before Task 5 translation. Scanner grouping is not a compiler call graph or proof all candidate branches emit; it cannot replace the existing reconstruction dispatch/continuation contracts.
* Separate native-platform blocker ledger: layout/pointer width, Apple bool, calling conventions/exports, FPU environment, vendor flags/effective overlays, platform APIs, serialization and incomplete production closure. Win32 x87 retained permanently.
* NXPF v1 fixture reader: explicit LE header/record sizes, bounded allocation, clear output and diagnostic on malformed/truncated/missing files. Finite numeric comparator validates inputs/budgets, uses the specified absolute-plus-relative formula, handles finite opposite-extreme subtraction overflow without accepting infinity/NaN arguments. No pointers/native padding serialized.
* 360 shared-math records: 18 helpers × 10 literal IEEE binary64 input vectors × two legacy CWs. Captured reconstructed helper reference, explicitly **not shipped oracle rows**. Manifest pins helper source revision/options, input classes, operation units, measured API-vs-simulation CW sensitivity and special-class discrepancies. Exact repeat budget is zero; portable numeric acceptance remains pending owning-task measurements. CW sensitivity is not a tolerance.
* 299 semantic geometry records exported from the unchanged existing harness against the shipped DLL; provenance/hash separate from reconstructed reference. Inputs/results retain literal u32 words. Exact repeat equality and oracle/candidate transcript equality established. This auxiliary capture does not establish a geometry acceptance budget.
* All conversion families have explicit fixture/unit/budget obligations with null budgets that cannot pass acceptance. Per preflight ruling, no arbitrary budgets fabricated for unconverted families; Tasks 3–9 must measure and approve budgets before their owning conversion is accepted.

## Baseline commands and results, before production changes

All commands below run in the worktree using PowerShell. `build/task1-evidence` is ignored; logs remain locally for review.

```powershell
New-Item -ItemType Directory -Force build/task1-evidence | Out-Null
cmake -S . -B build/x87-baseline -G 'Visual Studio 18 2026' -A Win32 -DNOVODEX_BUILD_VIEWER=OFF
cmake --build build/x87-baseline --config Release --target NxPhysics NxPhysicsGeometryTests NxPhysicsKernelFuzzTests NxPhysicsCollisionTests NxPhysicsActorMassTests NxPhysicsJointSupportTests NxPhysicsSimulationTests NxPhysicsBodyCreationTests --parallel 6
$baselineRoot = (Resolve-Path build/x87-baseline).Path
New-Item -ItemType Directory -Force build/x87-baseline/pairs | Out-Null
$baselinePairs = (Resolve-Path build/x87-baseline/pairs).Path
& docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsGeometryTests,NxPhysicsKernelFuzzTests,NxPhysicsActorMassTests,NxPhysicsSimulationTests,NxPhysicsBodyCreationTests -BuildRoot $baselineRoot -PairsRoot $baselinePairs
$baselineOracle = Join-Path $baselinePairs 'oracle'
$baselineHash = '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
& build/x87-baseline/Release/NxPhysicsCollisionTests.exe $baselineOracle $baselineHash
& build/x87-baseline/Release/NxPhysicsJointSupportTests.exe $baselineOracle $baselineHash
```

Configure/build exit 0, MSVC 19.51.36260.0, VS18 2026 Win32 Release. Five staged-pair differentials exit 0; every oracle/candidate child exits 0, stdout_delta=0, stderr_exact=True. These prove exact transcript equality, with loader-pinned identities validated by the unchanged runner. Collision is a separate in-process reconstructed-vs-shipped oracle probe: exit 0, matrix_wrong=0, index_wrong=0, mismatches=0. JointSupport likewise: exit 0, two kind5 cases, oracle/candidate digest `88b713b7bc0870c9`, input digest `85a7065a061c36cd`, mismatches=0. These are seven selected x87 target baselines, not all registered reconstruction gates.

Oracle Physics SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; Foundation `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`. Worktree-built Physics `583ecd4faaca65eb9d548d2dfd905024f5f01907b5ebb1bf1e35b9ce0fd114ed`; Foundation `c8d572e1f994f13672ec4378a3746c2ba69d3bbfa1bfe42ff646f944466b62a4`.

Initial runner attempt used mixed-slash PairsRoot and was rejected by its exact immediate-parent check before staging. Classified invocation error; corrected with Resolve-Path, no runner/test alteration. No selected-family baseline failures remain. Collision still prints unreconstructed dispatch entries despite its current passing matrix/digest assertions: conversion closure must recover those entries and cannot cite this pass as evidence of complete portable coverage.

## TDD red → green and focused verification

Applied test-driven-development and verification-before-completion skills. Tests were written before helper semantics; initial linked skeleton returned false without diagnostics. Built successfully, then ran:

```powershell
cmake -S tests/portable -B build/portable-kernels
cmake --build build/portable-kernels --config Release
ctest --test-dir build/portable-kernels -C Release --output-on-failure -R Portable.Fixture
```

RED: exit 8, Portable.FixtureSupport failed on every truncation size's required diagnostic/output clearing, valid payload, malformed diagnostics, exact equality, signed zero, near-zero/relative/combined-budget acceptance. This was assertion failure for missing behavior, not compile/link error. Log `fixture-red.log` contains the observed failures. Implemented reader/comparator; GREEN: initially 1/1 passed. Added checked fixture witness (sqrt(1)=1), malformed size/endian and budget edge cases, then final fresh Release 2/2 passed (Portable.FixtureSupport and Portable.FixtureReference), exit 0. Reader errors are checked separately from special-value numeric handling. Both CTests marked serial because they use the same temporary test filename.

```powershell
cmake --build build/portable-kernels --config Debug
ctest --test-dir build/portable-kernels -C Debug --output-on-failure -R Portable.Fixture
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests
git diff --check
```

Debug 2/2 passed, exit 0. Reconstruction tooling suite: **803 tests in 202.935s, OK**, exit 0 (`tooling-suite.log`). Whitespace check clean. Historical targets are not assumed to be CTest-registered; baseline results above are real harness executions, not empty CTest selections.

Non-MSVC verification attempt:

```powershell
cmake -S tests/portable -B build/portable-clang -G Ninja '-DCMAKE_CXX_COMPILER=C:/Program Files/LLVM/bin/clang++.exe'
```

Compiler identified Clang 22.1.8, but configure exit 1: `No CMAKE_RC_COMPILER could be found` in Windows-Clang.cmake. This is the toolchain environment, before new target compilation. No Clang or non-Windows test success claimed; Task 2/9 should use a configured Windows SDK environment or native runner. No engine production fallback was selected.

## Reproducible captures

```powershell
cmake -S tests/portable -B build/portable-x87-export -G 'Visual Studio 18 2026' -A Win32
cmake --build build/portable-x87-export --config Release
& build/portable-x87-export/Release/NxPortableExportSharedMath.exe tests/portable/fixtures/shared-math-x87.nxpf
& build/portable-x87-export/Release/NxPortableExportSharedMath.exe build/task1-evidence/shared-math-repeat.nxpf
& build/x87-baseline/Release/NxPhysicsGeometryTests.exe $baselineOracle *> build/task1-evidence/geometry-oracle-repeat.log
# Save full runner stdout/stderr above to build/task1-evidence/differential-x87.log.
python docs/reconstruction/novodex-physics/tools/capture_portable_fixtures.py
python docs/reconstruction/novodex-physics/tools/audit_portable_dependencies.py
```

Shared exports both exit 0, records=360 width=80; both SHA-256 `974c39747d605651c4898378acbe91bb1096daeebb2c612f3478546f9458a2d9`. The capture script asserts equality of both binary exports and of all 299 geometry semantic records from first and repeat oracle runs before generating fixture metadata. The scripts run from repo root; capture metadata is tied to this bootstrap build/source revision, not a general acceptance generator. Header includes magic/version/payload-size/width; payload contains op/CW/eight u64 input words/one u64 output word. Special inputs originate from integer bit literals, never floating arithmetic. Output binary64 store is an explicit observation boundary; it does not preserve an unobservable st(0) extended return.

## Self-review and limitations

Checked the exact brief interfaces; dependencies/fixtures encoded with fixed widths; bounds and finite-only contracts covered; public headers/production sources remain byte-unchanged; retained x87 flags and exact oracle tests untouched. Source ledger includes unconditional vendor /Qfast_transcendentals and generated /arch:IA32 dependencies, unguarded naked units without scalar arms, thiscall/fastcall receiver/table semantics, indirect shape-pair/support callbacks and continuation labels. Native layout and round-environment work remain explicit.

Task 1 bootstraps audit/fixture infrastructure; it does not perform algorithm translation, recover every massive naked routine's full typed register contract, establish all portable acceptance budgets, build the full portable engine, or claim Linux/macOS support. Future acceptance must measure each listed unit/family rather than treat repeat-zero or cross-CW sensitivity as a portable error budget. The source scanner remains heuristic and overinclusive; authoritative per-family ABI reconstruction must verify calls/aliases/continuations before conversion. Broader x87-family baselines must run when their owning conversion begins.

Commit: recorded in agent final after commit creation. Report is intentionally force-added from the normally ignored .superpowers directory.
