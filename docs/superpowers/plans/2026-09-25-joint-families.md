# Remaining Joint Families — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct the other nine joint families (and the shared joint code they need) as real source, the same way the revolute pilot did, and wire each family into `Scene::createJoint` behind a byte-exact test against the original DLL.

**Architecture:** This plan continues the translation-unit pilot (`docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`, results in `docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md`). Two shared steps come first: (1) the shared NpJoint public-method bodies move into one base; (2) a contract covers the shared base code the families need. After that, each family is one self-contained task: contract, rows, wiring, one new test case per family in `tests/PhysicsJointTests.cpp`, and inventory. The staged-pair runner compares the whole test transcript between the original and the candidate, so each new case is a real regression check.

**Tech Stack:** MSVC Win32 Release via CMake, x87 via `/arch:IA32` for internal joint files, Python evidence tools, PowerShell gate runners, pinned Ghidra supplement for rows without a decompile.

## Global Constraints

- Worktree root: `D:\github\Novodex\.claude\worktrees\reconstruction-progress-5b3127` (call it `$WT`). Build directory: `$WT\build`. Evidence root: `docs/reconstruction/novodex-physics` (`$EV`).
- Public headers are immutable: nothing under `Physics/include/**` or `Foundation/include/**` changes. `verify_public_headers.py` must pass.
- Product code must not forward to, load, or read the oracle DLL.
- No row is promoted above `reconstructed`. No closure ledger rule changes; ledger counts/reasons change only as the validator requires for newly reconstructed rows.
- No existing expected transcript line in `$EV/tools/gate_targets.ps1` is edited to make a gate pass. A transcript difference is a defect in the new source. New registered lines may be added only by copying them verbatim from the ORACLE side of a staged-pair run.
- The parameterised row models in `Physics/src/ObjectModel.cpp` stay; they may only gain a `// Product row: Physics/src/core/<File>.cpp.` pointer comment.
- Every product function in `Physics/src/core/` is preceded by a comment line naming its stable ID, RVA and size, in exactly this form and nothing else on the line: `// phys_fn_004330 (0x000a8d40, 281 B)`. No other comment line may begin with `// phys_fn_`.
- Tabs for indentation in C++. Python uses 4 spaces.
- Internal joint translation units (`core/<Family>Joint.cpp`, and any shared internal joint file) go on the `/arch:IA32` list in `CMakeLists.txt`. Np wrapper files stay on the default architecture.
- x87 fidelity: register lifetimes are `double`, spilled values `NxReal`, keep the listing's operation grouping and order; acos goes through the x87 `_CIacos` reproduction (`revoluteCIacos` in `core/RevoluteJoint.cpp`; move it to a shared internal header before reuse). No CRT math where the oracle uses x87 instructions.
- The Capstone listing (`$EV/oracle/capstone/manifest.json`) is authoritative over any decompile. Calling conventions and stack purge come from the listing. Supplement rows carry `calling_convention "unknown"`.
- Allocation through `nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT)` and placement new; classes free through a class `operator delete` on the SDK allocator.
- Public object lock links at np+0x10 / np+0x14 are passed BY VALUE (the link pointer) to the `nxNpSceneGuard*` helpers, as in `core/NpRevoluteJoint.cpp`.
- `dynamic_proof` only with evidence that the row executes (a debugger trace or instrumented run); being in the call graph is not evidence.
- Every task appends one row to the timing table in `$EV/evidence/joint-families.md` (created in Task 1): task, start/end local ISO time, rows written, bytes written, notes.
- Do not use bare `git stash`. Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Known pre-existing failure, not to be fixed here: Phase 2/3 gates fail at `build_physics` (`IcePrunable.h` not found in the standalone test targets).
- Standard verification commands:
  - Build: `cmake --build build --config Release --target NxPhysics` (reconfigure with `cmake -S . -B build -A Win32` after CMakeLists changes).
  - Joint gate: `powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 6 -RepoRoot "$PWD" -BuildRoot "$PWD/build"` → `status=pass`.
  - Also `-Phase 7` → `status=pass`; `-Phase 5` → fails only on `candidate CANDIDATE-MISSING family=vtables`.
  - `python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json` exits 0.
  - `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'` → OK.
  - Stable-ID form check: every line in `Physics/src/core/*.cpp` matching `^\s*// phys_fn_` fullmatches `\s*// phys_fn_\d{6} \(0x[0-9a-f]{8}, \d+ B\)`.

---

### Task 1: Shared NpJoint base

**Files:**
- Create: `Physics/src/include/core/NpJointShared.h` (and `Physics/src/core/NpJointShared.cpp` if bodies are out of line)
- Modify: `Physics/src/core/NpRevoluteJoint.cpp`, `Physics/src/include/core/NpRevoluteJoint.h`
- Create: `$EV/evidence/joint-families.md` (timing table header, same columns as `unit-pilot-revolute.md`)

**Interfaces:**
- Produces: a shared implementation of the 13 folded NpJoint bodies (getActors 004539, getGlobalAnchor 004437, getGlobalAxis 004441, getGlobalAnchorVal 004497, getGlobalAxisVal 004499, getState 004483, getBreakable 004573, getLimitPoint 004577, hasMoreLimitPlanes 004491, getNextLimitPlane 004635, getType 004443, is 004479, getName 004743) plus the other NxJoint-level forwarders that every Np family shares (setGlobalAnchor, setGlobalAxis, setBreakable, setLimitPoint, addLimitPlane, purgeLimitPlanes, resetLimitPlaneIterator, setName, …). Identify these from the per-family dispatch tables in `$EV/inventory.json` (data objects `phys_data_0027xx`: slots that name the same target row across families are shared). Use a template `template<class Iface> class NpJointShared : public Iface, public EmbeddedHookBase` (or the equivalent that keeps the 0x1c layout and the compiler-generated vtable order of each public `Nx<Family>Joint`) so each family class derives from `NpJointShared<NxXJoint>` and adds only its family-specific methods.
- Shared bodies carry the comment `// Shared NpJoint body; the oracle keeps one folded copy at 0x… (phys_fn_NNNNNN).` — never the stable-ID form. Rows shared across families are claimed by the unit the census assigns them to; if that unit is an Np file other than revolute, claim them in Task 1 by placing the stable-ID line on the shared body and recording which unit owns the row.

- [ ] **Step 1:** List every Np family's primary dispatch table (slot → target row) from `$EV/inventory.json` data objects and write the shared/family-specific split into `$EV/units/joint-families-contract.md` under `## Shared NpJoint slots`.
- [ ] **Step 2:** Extract the shared bodies into `NpJointShared`, make `NpRevoluteJoint` derive from `NpJointShared<NxRevoluteJoint>`, keep its `static_assert`s (size 0x1c, +0x04/+0x08/+0x10/+0x14/+0x18).
- [ ] **Step 3:** Build; run the joint gate (revolute transcript must stay byte-identical); run the stable-ID check.
- [ ] **Step 4:** Commit ("Share NpJoint public bodies across joint families").

---

### Task 2: Shared base contract and remaining Joint.cpp rows

**Files:**
- Modify: `$EV/units/joint-families-contract.md`, `Physics/src/core/Joint.cpp`, `Physics/src/core/JointSupport.cpp`, their headers
- Create (generated, via `tools/unit_bundle.py`): bundles for `Joint.cpp`, `gap:NpSpringAndDamperEffector.cpp..Joint.cpp`, `gap:Joint.cpp..D6Joint.cpp`, `gap:core\PrismaticJoint.cpp..core\NpD6Joint.cpp`
- Modify (if needed): `$EV/oracle/ghidra/supplement.json` via `DecompileSupplement.java` (same command as the pilot's Task 2, `-readOnly -noanalysis`; request only rows with no `ok` decompile)

**Interfaces:**
- Produces: `## Shared rows` in the families contract — for every row in those units: owning file (`core/Joint.cpp`, `core/JointSupport.cpp`, or a new file named by what it is, e.g. `core/SpringAndDamperEffector.cpp`), which families call it, and `write` / `defer` / `reuse`. Rows that are effector code, not joint code, are listed but written only if a joint family calls them.
- Writes the remaining `write` rows in `core/Joint.cpp` / `core/JointSupport.cpp` (Joint.cpp has 13 not-started rows; the gaps hold shared helpers).

- [ ] **Step 1:** Generate the bundles; supplement missing decompiles.
- [ ] **Step 2:** Write `## Shared rows` with evidence per row (callers by family, from `oracle/dependencies.dot` and dispatch tables).
- [ ] **Step 3:** Write the `write` rows (procedure: pilot plan Tasks 6–9 "Procedure", reproduced in Task 3 below). Build; joint gate pass.
- [ ] **Step 4:** Update the inventory for rows written (state `reconstructed`, `implementation`/`source` = file path, `static_proof` = "transcribed from units/<bundle>.md (<label>) and checked against the Capstone listing; see units/joint-families-contract.md"), ledger reasons/counts as the validator requires. Validator exits 0.
- [ ] **Step 5:** Commit ("Reconstruct shared joint base rows for the joint families").

---

### Task 3 (template): One joint family

Tasks 3a–3i each apply this template to one family, in this order (smaller first; D6 last): 3a prismatic (type 0), 3b cylindrical (2), 3c spherical (3), 3d point-on-line (4), 3e point-in-plane (5), 3f distance (6), 3g pulley (7), 3h fixed (8), 3i D6 (9). The units are `core\<Family>Joint.cpp` and `core\Np<Family>Joint.cpp` plus the neighbouring gap rows the contract assigns (e.g. `gap:core\SphericalJoint.cpp..core\CylindricalJoint.cpp` holds 10 KB of spherical/cylindrical rows).

**Files (per family):**
- Create: `Physics/src/core/<Family>Joint.cpp`, `Physics/src/core/Np<Family>Joint.cpp`, `Physics/src/include/core/<Family>Joint.h`, `Physics/src/include/core/Np<Family>Joint.h`
- Modify: `CMakeLists.txt` (add `core/<Family>Joint.cpp` to the `/arch:IA32` list), `Physics/src/Scene.cpp` (`NxSceneInternal::createJoint` case for this type), `tests/PhysicsJointTests.cpp` (one new case), `$EV/tools/gate_targets.ps1` (new oracle-sourced lines only), `$EV/tools/validate_inventory.py` (remove allowlist entries the validator names as no longer unresolved), `$EV/inventory.json`, `$EV/gates/phase6-closure.json`, `$EV/units/joint-families-contract.md`, `$EV/evidence/joint-families.md`
- Generated: `$EV/units/core__<Family>Joint.cpp.md`, `$EV/units/core__Np<Family>Joint.cpp.md`

**Interfaces:**
- Consumes: `NpJointShared` (Task 1); shared rows (Task 2); `Joint` base class (`Physics/src/include/core/Joint.h`).
- Produces: `class <Family>Joint : public Joint` (internal, size from the createJoint allocation literal) and `class Np<Family>Joint : public NpJointShared<Nx<Family>Joint>` (0x1c); createJoint returns the public object for this type.

- [ ] **Step 1: Contract.** Add `## <Family>` to `$EV/units/joint-families-contract.md` with: row assignment (check the unit extents and ambiguous neighbours by hand, as the pilot's Task 4 Step 1 did); the createJoint case arm for this type (from the `0x14590` switch table in the Capstone manifest: allocation size, constructor row); object layout with offsets and the row that establishes each field (unknown fields by offset only); dispatch tables (internal and public, slot → row → `Nx<Family>Joint`/`NxJoint` virtual in header order); dependency closure (`write` / `defer` / `reuse`); which rows the new test case will reach. Commit.
- [ ] **Step 2: Scaffold.** Headers with offset `static_assert`s for every established field and size; stubs (`NX_ASSERT(0);` + zero return, stable-ID line, then `// (unimplemented)`); CMake `/arch:IA32` entry; allowlist cleanup. Build; joint gate pass. Commit.
- [ ] **Step 3: Rows.** For each row in address order: read the bundle entry; write the body (named members; pilot rows call their C++ functions; `reuse` deps call the named symbol; `defer` deps keep an asserting declared function with a `// (deferred: <reason>)` line; indirect calls through the contract's vtables; assert/report file/line/expression from the image; float order and precision from the listing); record every decompile/listing disagreement in the row comment; add the `Product row:` pointer to any ObjectModel model the row covers. Build after every row of 500 B or more. Joint gate pass. Commit.
- [ ] **Step 4: Wire.** In `NxSceneInternal::createJoint`, replace this type's generic `nxJointConstruct` path with the contract's construction chain: SDK-allocator malloc + placement new of `<Family>Joint`, read the public object at the byte offset the listing reads (+0x48 for revolute; verify for this family), null handling as the oracle does, copy Scene+0x6cc holder[3]/holder[4] into the public object's links through a helper like `nxRevoluteJointAttachScene`, register via `nxSceneAddJoint`, run the shared exit sequence (`++[Scene+0x6c8]`, `[Scene+0x6bc]=[Scene+0x59c]`, 0x14529–0x1453f), return the public object. Leave release unwired. Build.
- [ ] **Step 5: Test case.** Add `nx<Family>Case` to `tests/PhysicsJointTests.cpp`, modelled on `nxRevoluteCase`: build a valid `Nx<Family>JointDesc` over the existing two-actor fixture with two anchor/axis variants (identity and one non-trivial), `createJoint`, then print as hex words: created, getGlobalAnchor/getGlobalAxis, getState, getActors match, getType, `is<Family>Joint()` non-null, and every family getter that reads stored state without simulation (limits, motor, spring, distances, projection as applicable), then `releaseJoint`. Call it from `wmain` after the revolute cases. Both `NxPhysicsJointTests` and `NxPhysicsJointStagedPairTests` build from this file.
- [ ] **Step 6: Verify.** Run the joint gate. The staged pair must report `stdout_delta=0`. Any difference is a defect: debug with superpowers:systematic-debugging, fix the source, never the expectation. Then register two to four lines for this family in both joint target lists in `gate_targets.ps1`, copied verbatim from the oracle-side transcript (created, anchor/axis/state, and one family-getter line), and update the Phase 6 coverage floor count and any tool tests that pin it. Re-run the joint gate, Phase 7, Phase 5 (vtable marker only), validator and tool tests.
- [ ] **Step 7: Inventory.** Rows with product source → `reconstructed` (existing proofs kept, new text appended), `implementation`/`source` = file path, `static_proof` as in Task 2 Step 4. `dynamic_proof` only for rows shown executing by a cdb breakpoint trace of the candidate `NxPhysicsJointTests.exe` (method: the pilot's `evidence/unit-pilot-revolute-trace.txt`); commit a trimmed trace excerpt as `$EV/evidence/joint-families-trace-<family>.txt` if you take one. Ledger reasons/counts as the validator requires. Validator exits 0.
- [ ] **Step 8: Record and commit.** Append the timing row (rows, bytes, defects found by the transcript, rows deferred). Commit ("Reconstruct and wire the <family> joint").

If a transcript difference cannot be resolved within the task: revert only Step 4's wiring for this type (generic path again), remove this family's test case, keep the rows as source-only `reconstructed` with static proof, record the unresolved difference in the contract and timing notes, and report DONE_WITH_CONCERNS.

---

### Task 4: Results

**Files:** `$EV/evidence/joint-families.md`, `$EV/work_units.json` and bundles (regenerate)

- [ ] **Step 1:** Regenerate `work_units.json` and every joint bundle (commands as in the pilot plan Task 1 Step 6 and Task 3 Step 5).
- [ ] **Step 2:** Complete `joint-families.md`: `## Result` (per family: rows and bytes moved, wired or not, test lines registered; rows deferred and why; open items carried forward, including release wiring, rotated-body cases, simulation-only rows never executed), `## Defects found by the transcript`, `## Rate` (rows/hour and bytes/hour per family from the timing table, compared with the pilot's; same caveats), `## Verification` (full fresh build, headers, tool tests, gates 4–7 lines; Phase 2/3 pre-existing).
- [ ] **Step 3:** Commit ("Record joint family reconstruction results").
