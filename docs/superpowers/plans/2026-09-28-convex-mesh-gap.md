# IceAdjacencies..ContactConvexHeightfield Gap — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconstruct the not-started code between `IceAdjacencies.cpp` and `ContactConvexHeightfield.cpp`: work unit `gap:IceAdjacencies.cpp..ContactConvexHeightfield.cpp` (157 rows, 109 not started, about 61 KB), plus the adjacent small units `IceAdjacencies.cpp` (2 rows), `ContactConvexHeightfield.cpp` (3 rows) and `gap:ContactConvexHeightfield.cpp..ContactMeshMesh.cpp` (12 rows, about 9.5 KB). The work is split into source-file sub-units and each is proved against the original DLL.

**Architecture:** This follows the translation-unit workflow the joints used (`docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`; worked examples in `docs/reconstruction/novodex-physics/units/revolute-contract.md` and `joint-families-contract.md`):
1. A survey and contract splits the range into sub-units by evidence: call-graph clusters, dispatch tables, asserts and strings, and correspondence with any vendored ICE/OPCODE source.
2. Each sub-unit is written from the listing (bundle, contract, faithful x87 source) and wired where the candidate already has callers.
3. Each sub-unit is checked with an oracle-vs-candidate differential, using the Phase 3/4 harness style (`tests/PhysicsCollisionTests.cpp`, `tests/PhysicsThirdPartyTests.cpp`: oracle rows called by RVA against candidate functions, ceilings for divergent families).

**Tech Stack:** MSVC Win32 Release via CMake, x87 (`/arch:IA32` where the oracle is x87), Python evidence tools (`unit_bundle.py`, `vendored_match.py` if ICE-vendored), capstone/pefile, cdb, PowerShell gate runners.

## Global Constraints

- Worktree `D:\github\Novodex\.claude\worktrees\trusting-shaw-883d69` (branch `claude/trusting-shaw-883d69`, which also carries the in-progress qhull-gap plan commits). Evidence root `$EV` = `docs/reconstruction/novodex-physics`.
- Public headers under `Physics/include/**` and `Foundation/include/**` are immutable. Upstream vendored trees are immutable; changes to vendored code go in overlays, recorded in `MODIFICATIONS.md`.
- Product code must not forward to, load or read the oracle DLL.
- Every product function carries a stable-ID comment line in exactly this form, with nothing else on the line: `// phys_fn_NNNNNN (0x%08x, N B)`. No other comment line may begin with `// phys_fn_`.
- x87 fidelity:
  - Register lifetimes are `double`, spills are `NxReal`, and the listing's grouping and order are kept.
  - No CRT math where the oracle uses x87 instructions: use `Physics/src/include/X87Sqrt.h` helpers, with the operands in the listing's order.
  - Files the oracle compiles as x87 go on the `/arch:IA32` list.
  - The listing wins over any decompile.
- Allocation follows each oracle row's own allocator: the imported `nxFoundationSDKAllocator` (`[0x101041bc]`) or the 004803 getter. Malloc and free are always paired on the same allocator.
- No row is promoted above `reconstructed`. `reconstructed` requires a static proof that cites the listing. `dynamic_proof` requires committed execution evidence: a cdb trace excerpt with a sha pin, or a differential run.
- Ledger reasons change only as `validate_inventory.py` requires. Every row moved to `reconstructed` gets the file's standard `reconstructed_not_falsified` note.
- Registered gate lines are append-only for pre-existing lines. New lines are copied verbatim from the oracle side. Divergent families carry enforced ceilings, following the pattern in `tests/PhysicsThirdPartyTests.cpp`.
- At the end of every task: gates 2, 3, 4, 6 and 7 pass; Phase 5 fails only on `candidate CANDIDATE-MISSING family=vtables`; tool tests pass; the validator exits 0.
- Timing table: `$EV/evidence/convex-mesh-gap.md` (created in Task 1). Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

---

### Task 1: Survey and contract

- [ ] Generate bundles for the four units with `tools/unit_bundle.py`. Supplement missing decompiles with the pinned `DecompileSupplement.java`, passing the union of existing requested RVAs as the earlier plans did.
- [ ] Write `$EV/units/convex-mesh-gap-contract.md` with the following sections.
  - `## Sub-units`: split every code row in the range into probable source files, with evidence for each:
    - call-graph clusters and dispatch tables (vtables installed in the range and their slot owners);
    - asserts and strings, and data objects referenced;
    - correspondence with the vendored ICE code in `External/opcode/novodex/Ice` (IceAdjacencies-like, edge lists, hull or valency helpers).
    Run `vendored_match.py`-style name matching if any rows correspond to available source.
  - For each sub-unit: rows, bytes, states, phases, what it does, and its callers outside the range, with the candidate code that stands in for those callers today (grep `Physics/src` for the stable IDs and for existing candidate functions that model these rows, e.g. `ContactGeneration.cpp`, `NarrowPhase.cpp`, `Geometry.cpp`, `TriangleMesh.cpp`, `SmoothNormals.cpp`).
  - For each sub-unit: its test route. Which existing differential already reaches it, if any (Phase 3 collision, Phase 4 asset/third-party), or how a new oracle-vs-candidate family would drive it (entry row, inputs, outputs to compare).
  - `## Task split`: an ordered list of sub-unit tasks, each at most about 12 KB, dependencies first.
- [ ] Commit the contract, the bundles and the timing file.

### Task 2 (template): One sub-unit

Tasks 2a, 2b, … apply this template to each sub-unit in `## Task split`, in order:
- [ ] **Rows.** Write every row faithfully from the listing, in the owning source file the contract names (a new `Physics/src/<File>.cpp` or an existing candidate file). Stable-ID lines, x87 rules and allocator rules as in the Global Constraints. Where the candidate already has a model or stand-in for a row, replace it with the product row or route it through the product row. Keep existing behaviour-pinning tests green.
- [ ] **Wire.** Point existing candidate callers at the new rows where the oracle calls them.
- [ ] **Differential.** Add or extend an oracle-vs-candidate family that drives the sub-unit's entry rows over meaningful inputs, including degenerate ones, and compares outputs. Register a few oracle-sourced lines, add ceilings for any divergent family, and raise floors and Python pins.
- [ ] **Evidence.** Take a cdb trace of the rows executed. Inventory: move written rows to `reconstructed` with static proofs; set `dynamic_proof` only for traced rows. Update the ledger per the validator. Add a timing row. Commit.

### Task 3: Results

- [ ] Regenerate `work_units.json` and the four units' bundles.
- [ ] Complete `$EV/evidence/convex-mesh-gap.md`: rows and bytes moved per sub-unit, defects found by the differentials, rows left and why, the rate, and verification (fresh configure and clean build, headers, tool tests, validator, gates 2–7).
