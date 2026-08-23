# Novodex Physics Phase 1 Oracle Census Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a reproducible, exhaustive Ghidra/Capstone census of the pinned `NxPhysics.dll`, with stable IDs, labels, byte ownership, class/data structure, and a dependency graph suitable for reconstruction planning.

**Architecture:** Independent PE, Ghidra, and Capstone extractors emit normalized JSON. A reconciler assigns stable IDs and refuses overlaps, gaps, invalid branch targets, or unsupported semantic labels; inventory validation makes census completeness a hard gate.

**Tech Stack:** Python 3 standard library plus `capstone`; Ghidra headless Java scripting; PE32/x86; unittest; Graphviz DOT output; PowerShell.

---

### Task 1: Scaffold evidence and lock schemas

**Files:**
- Create: `docs/reconstruction/novodex-physics/README.md`
- Create: `docs/reconstruction/novodex-physics/inventory.json`
- Create: `docs/reconstruction/novodex-physics/labels.json`
- Create: `docs/reconstruction/novodex-physics/analysis_toolchain.json`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_inventory.py`
- Create: `docs/reconstruction/novodex-physics/tools/validate_inventory.py`
- Create: `docs/reconstruction/novodex-physics/tools/verify_toolchain.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_toolchain.py`
- Create: `docs/reconstruction/novodex-physics/public_header_hashes.json`
- Create: `docs/reconstruction/novodex-physics/tools/verify_public_headers.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_public_headers.py`

- [ ] **Step 1: Write the failing schema and toolchain tests**

Test a minimal fixture and assert: unique stable IDs; non-overlapping code ranges; allowed states; one phase owner; required Ghidra and Capstone refs for code; unique export ownership; label-ledger referential integrity; and zero unexplained executable bytes when `census.status == "pass"`. Toolchain tests reject changed Ghidra version/path/hash, changed Capstone version/package hash, and changed analysis options. Public-header tests reject recursive missing, extra, or byte-different files.

- [ ] **Step 2: Run the test and verify RED**

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_inventory.py' -v
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_toolchain.py' -v
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_public_headers.py' -v
```

Expected: import failures for `validate_inventory`, `verify_toolchain`, and `verify_public_headers`.

- [ ] **Step 3: Implement the inventory and recursive public-header validators**

Expose `validate_inventory(data: dict) -> list[str]` and a CLI accepting one JSON path. Print each error to stderr and exit `1`; on success print `inventory=pass` followed by the measured integer function/data counts and `unexplained=0`, then exit `0`.

Expose public-header manifest generation and verification over the recursive UE3 `Development/External/Novodex/Physics/include` tree. Record relative paths, sizes, and SHA-256 values; reject missing, extra, or byte-different files. Shared Foundation types remain pinned by `docs/reconstruction/novodex-foundation/dumps/immutable_public_hashes.json`.

- [ ] **Step 4: Implement and pin the analysis toolchain**

`verify_toolchain.py` exposes `measure_toolchain(ghidra_home, analysis_options) -> dict`, `verify_toolchain(pin, measured) -> list[str]`, `--write-pin`, and `--pin`. It records Ghidra version, headless executable path/hash, Capstone Python version, installed package-root hash, Python version, processor/language/compiler spec, analyzer enablement, timeout, and a canonical analysis-options hash.

```powershell
$ghidraHome=$env:NOVODEX_GHIDRA_HOME
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --write-manifest --root D:\FlamingEnt__\Unreal_3\Development\External\Novodex\Physics\include --output docs/reconstruction/novodex-physics/public_header_hashes.json
python docs/reconstruction/novodex-physics/tools/verify_toolchain.py --write-pin --ghidra-home $ghidraHome --output docs/reconstruction/novodex-physics/analysis_toolchain.json
python docs/reconstruction/novodex-physics/tools/verify_toolchain.py --pin docs/reconstruction/novodex-physics/analysis_toolchain.json --ghidra-home $ghidraHome
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_toolchain.py' -v
```

Expected: recursive header manifest records the pinned UE3 tree; the tool prints `toolchain=pass` with the exact measured Ghidra and Capstone versions stored in the pin; tests pass.

- [ ] **Step 5: Create initial inventory and label ledger**

Inventory top-level keys are `schema_version`, `pins`, `sections`, `functions`, `data_objects`, `exports`, `imports`, `coverage`, `phases`, and `gates`. Initial census and all gates are `pending`; arrays are empty. `labels.json` contains `schema_version` and `labels: []`.

Every extractor must call the verifier and reject a mismatched tool version or analysis-option hash before producing evidence.

- [ ] **Step 6: Run tests and commit**

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py' -v
git add docs/reconstruction/novodex-physics
git commit -m "docs: scaffold Physics oracle census"
```

Expected: tests pass; no `__pycache__` is created.

### Task 2: Build the independent PE manifest

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/pe_manifest.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_pe_manifest.py`
- Create: `docs/reconstruction/novodex-physics/oracle/pe.json`

- [ ] **Step 1: Write failing PE fixture tests**

Cover PE32 validation, section RVA mapping, executable-range extraction, imports, named exports with ordinals, base relocations, resources, and rejection of unterminated strings or out-of-section RVAs. Reuse the proven strict parsing patterns from `docs/reconstruction/novodex-foundation/tools/pe_exports.py` rather than weakening bounds checks.

- [ ] **Step 2: Verify RED**

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_pe_manifest.py -v
```

Expected: `ModuleNotFoundError` for `pe_manifest`.

- [ ] **Step 3: Implement and run the manifest tool**

CLI:

```powershell
python docs/reconstruction/novodex-physics/tools/pe_manifest.py `
  --input D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll `
  --output docs/reconstruction/novodex-physics/oracle/pe.json
```

The JSON stores hashes, image base, entry point, machine, timestamp, sections, executable intervals, imports, exports, relocations, resources, independently scanned strings, and pointer-sized values into mapped code/data with integer RVAs plus canonical hexadecimal display strings. Scan every mapped non-resource section; retain source RVA, target RVA/value, relocation association, string encoding/length, and section bounds.

- [ ] **Step 4: Assert pinned identity**

Expected: SHA-256 and size match the umbrella pins; machine is `0x014c`; optional-header magic is `0x010b`; named export count is `41`.

- [ ] **Step 5: Commit**

```powershell
git add docs/reconstruction/novodex-physics/tools docs/reconstruction/novodex-physics/oracle/pe.json
git commit -m "docs: inventory Physics PE structure"
```

### Task 3: Export deterministic Ghidra semantics

**Files:**
- Create: `docs/reconstruction/novodex-physics/ghidra/ExportPhysicsAnalysis.java`
- Create: `docs/reconstruction/novodex-physics/ghidra/ApplyPhysicsTypes.java`
- Create: `docs/reconstruction/novodex-physics/ghidra/physics_x86_msvc.h`
- Create: `docs/reconstruction/novodex-physics/ghidra/physics_type_inputs.json`
- Create: `docs/reconstruction/novodex-physics/tools/generate_ghidra_types.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_generate_ghidra_types.py`
- Create: `docs/reconstruction/novodex-physics/tools/normalize_ghidra.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_normalize_ghidra.py`
- Create: `docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json`

- [ ] **Step 1: Write failing normalization tests**

Fixtures cover deterministic address ordering, volatile Ghidra metadata removal, normalized type strings, function chunks, thunk targets, calls, branches, p-code memory references, strings, symbols, vtables, RTTI, and data references.

Run RED for normalization and type generation:

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_normalize_ghidra.py -v
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_generate_ghidra_types.py -v
```

Expected: import failures for `normalize_ghidra` and `generate_ghidra_types`.

- [ ] **Step 2: Generate the public-type ABI shim reproducibly**

`physics_type_inputs.json` records the recursive Physics header manifest hash, the existing immutable Foundation-header manifest hash, include roots, Win32/MSVC preprocessor definitions, forced declarations, type-selection rules, and generator version. `generate_ghidra_types.py` consumes only those manifests and the pinned UE3 header tree; it emits deterministic C-compatible enum, layout-struct, inheritance/base-offset, and function-prototype declarations without hand-authored type bodies.

```powershell
python docs/reconstruction/novodex-physics/tools/generate_ghidra_types.py --headers D:\FlamingEnt__\Unreal_3\Development\External\Novodex --inputs docs/reconstruction/novodex-physics/ghidra/physics_type_inputs.json --output docs/reconstruction/novodex-physics/ghidra/physics_x86_msvc.h
Copy-Item docs/reconstruction/novodex-physics/ghidra/physics_x86_msvc.h $env:TEMP\physics_x86_msvc.second.h
python docs/reconstruction/novodex-physics/tools/generate_ghidra_types.py --headers D:\FlamingEnt__\Unreal_3\Development\External\Novodex --inputs docs/reconstruction/novodex-physics/ghidra/physics_type_inputs.json --output $env:TEMP\physics_x86_msvc.regenerated.h
if((Get-FileHash $env:TEMP\physics_x86_msvc.second.h).Hash -ne (Get-FileHash $env:TEMP\physics_x86_msvc.regenerated.h).Hash){throw 'type shim is not deterministic'}
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_generate_ghidra_types.py -v
```

Expected: hashes equal and tests pass.

- [ ] **Step 3: Implement the headless exporter and typed second pass**

`ApplyPhysicsTypes.java` imports the generated `physics_x86_msvc.h` after initial auto-analysis, applies types to exports, recovered vtables, and public call boundaries, reruns affected reference/parameter/decompiler analyzers, and emits a type-coverage record; every public type referenced by an export or recovered public vtable must resolve.

`ExportPhysicsAnalysis.java` must emit one JSON-lines stream containing records tagged `function`, `instruction_range`, `symbol`, `string`, `reference`, `vtable`, `rtti`, and `data`. Function records include entry RVA, body ranges, prototype, calling convention, stack purge, thunk target, called RVAs, decompiler C, and decompiler status.

- [ ] **Step 4: Run pinned two-pass headless analysis**

```powershell
$analysisRoot='D:\FlamingEnt__\novodex-analysis\novodex-physics'
$ghidraHome=$env:NOVODEX_GHIDRA_HOME
if(-not (Test-Path "$ghidraHome\support\analyzeHeadless.bat")){throw 'Set NOVODEX_GHIDRA_HOME to the pinned Ghidra installation'}
python docs\reconstruction\novodex-physics\tools\verify_toolchain.py --pin docs\reconstruction\novodex-physics\analysis_toolchain.json --ghidra-home $ghidraHome
& "$ghidraHome\support\analyzeHeadless.bat" $analysisRoot PhysicsOracle `
  -import D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll `
  -overwrite -analysisTimeoutPerFile 3600 `
  -scriptPath docs\reconstruction\novodex-physics\ghidra `
  -postScript ExportPhysicsAnalysis.java "$analysisRoot\pass1.raw.jsonl"
& "$ghidraHome\support\analyzeHeadless.bat" $analysisRoot PhysicsOracle `
  -process NxPhysics.dll -analysisTimeoutPerFile 3600 `
  -scriptPath docs\reconstruction\novodex-physics\ghidra `
  -postScript ApplyPhysicsTypes.java docs\reconstruction\novodex-physics\ghidra\physics_x86_msvc.h `
  -postScript ExportPhysicsAnalysis.java "$analysisRoot\ghidra.raw.jsonl"
```

Record the exact Ghidra version and analysis options in the normalized manifest.

- [ ] **Step 5: Normalize twice and prove determinism**

```powershell
$analysisRoot='D:\FlamingEnt__\novodex-analysis\novodex-physics'
python docs/reconstruction/novodex-physics/tools/normalize_ghidra.py --input "$analysisRoot\ghidra.raw.jsonl" --output docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json
Get-FileHash docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json -Algorithm SHA256
```

Repeat from a clean Ghidra project. Expected: identical normalized hash.

Run GREEN:

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_normalize_ghidra.py -v
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_generate_ghidra_types.py -v
```

Expected: all normalization tests pass.

- [ ] **Step 6: Commit scripts and normalized evidence**

The mutable Ghidra project and raw JSON-lines file remain under the external `.analysis` path and never dirty the evidence worktree. Commit scripts and normalized evidence only.

### Task 4: Generate the independent Capstone corpus

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/capstone_manifest.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_capstone_manifest.py`
- Create: `docs/reconstruction/novodex-physics/oracle/capstone/manifest.json`

- [ ] **Step 1: Write failing x86 normalization tests**

Cover relative calls/jumps, conditional branches, indirect calls, x87, SSE, stack deltas, switch-table candidates, import thunks, executable relocation/pointer targets, tail calls, and undecodable bytes. Expected records retain raw bytes and normalized mnemonic/operand text.

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_capstone_manifest.py -v
```

Expected: import failure for `capstone_manifest`.

- [ ] **Step 2: Implement disassembly over PE executable ranges**

The tool consumes `pe.json` and the Ghidra manifest. It seeds entries from the PE entry point, exports, Ghidra functions, direct call/jump targets, executable relocation targets, pointer-table/vtable entries, switch targets, callbacks, thunks, and recovered tail calls. Recursively disassemble newly found targets to a fixed point, then sweep remaining ranges only to propose code/data/alignment classifications; a linear sweep alone cannot prove alignment or artifact ownership. Never silently skip undecodable bytes.

- [ ] **Step 3: Run and verify coverage**

```powershell
python docs/reconstruction/novodex-physics/tools/capstone_manifest.py `
  --binary D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll `
  --pe docs/reconstruction/novodex-physics/oracle/pe.json `
  --ghidra docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json `
  --output docs/reconstruction/novodex-physics/oracle/capstone/manifest.json
```

Expected: every executable byte is represented as an instruction byte, undecodable byte, or not-yet-classified coverage interval; every discovered executable target is a function/chunk entry or a proven intra-function block.

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_capstone_manifest.py -v
```

Expected: all Capstone manifest tests pass.

- [ ] **Step 4: Commit**

```powershell
git add docs/reconstruction/novodex-physics/tools docs/reconstruction/novodex-physics/oracle/capstone
git commit -m "docs: add complete Physics Capstone corpus"
```

### Task 5: Reconcile functions, labels, classes, and byte ownership

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/reconcile_analysis.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_reconcile_analysis.py`
- Create: `docs/reconstruction/novodex-physics/oracle/dependencies.dot`
- Create: `docs/reconstruction/novodex-physics/oracle/coverage.json`
- Create: `docs/reconstruction/novodex-physics/oracle/data-coverage.json`
- Modify: `docs/reconstruction/novodex-physics/inventory.json`
- Modify: `docs/reconstruction/novodex-physics/labels.json`

- [ ] **Step 1: Write reconciliation RED tests**

Reject overlapping function ownership, any direct/indirect recovered executable target that is not a function/chunk entry or proven intra-function block, a semantic label without evidence, duplicate stable IDs, missing Ghidra/Capstone references, and a passing census with unexplained bytes. Reject a pass when recursive target discovery has not reached a fixed point.

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_reconcile_analysis.py -v
```

Expected: import failure for `reconcile_analysis`.

- [ ] **Step 2: Implement deterministic stable-ID assignment**

Sort oracle code entries by entry RVA and assign `phys_fn_%06d`; sort significant data by RVA and assign `phys_data_%06d`. Stable IDs persist through later label changes.

- [ ] **Step 3: Reconcile until coverage is exact**

Use Capstone to split missed tails/thunks and resolve lost labels through calls, arguments, constants, switches, and floating-point behavior. Use Ghidra to recover types, inheritance, vtables, RTTI, and data ownership. Iterate new call/jump, executable relocation, address-taken pointer, vtable/callback, switch, thunk, and tail targets until a pass discovers no new entries. Every manual classification receives an evidence reference and reason.

Independently reconcile significant data from PE relocations/pointer scans, Ghidra references, RTTI/vtables, strings, code-addressed globals, lookup/dispatch tables, registries, constants/blobs read by code, and mutable mapped storage. Every referenced byte range must belong to a data object, function, resource, import structure, relocation metadata, or proven padding; `data-coverage.json` reports overlaps and unexplained referenced bytes separately from code coverage.

- [ ] **Step 4: Assign phase ownership from the dependency graph**

Assign from the translation-unit layout first and from the call graph only where the layout says nothing: the `NX_ASSERT` `__FILE__` seeds bound one contiguous address span per object, a span owns every entry inside it, and two spans of one phase bracket the unnamed units between them. Above the last entry naming a unit sits the statically linked runtime. Assign shared runtime to Phase 2, geometry/collision to 3, meshes/assets to 4, objects to 5, joints/effectors to 6, scenes/simulation to 7, and only compiler/runtime audit artifacts to Phase 8. Shared runtime means an entry no unit names that several phases call - not whatever is left over. No product function may be deferred to an unowned miscellaneous group, and every entry must record the rule that placed it.

- [ ] **Step 5: Close the Phase 1 gate**

```powershell
python docs/reconstruction/novodex-physics/tools/reconcile_analysis.py --pe docs/reconstruction/novodex-physics/oracle/pe.json --ghidra docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json --capstone docs/reconstruction/novodex-physics/oracle/capstone/manifest.json --inventory docs/reconstruction/novodex-physics/inventory.json --labels docs/reconstruction/novodex-physics/labels.json
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```

Expected: `census=pass`, unexplained executable bytes `0`, unresolved executable targets `0`, target-discovery additions on the final iteration `0`, unexplained referenced data bytes `0`, overlaps `0`, duplicate ownership `0`, and every function/data row has one phase.

```powershell
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p test_reconcile_analysis.py -v
```

Expected: all reconciliation tests pass.

- [ ] **Step 6: Commit**

```powershell
git add docs/reconstruction/novodex-physics
git commit -m "docs: close Physics oracle census"
```
