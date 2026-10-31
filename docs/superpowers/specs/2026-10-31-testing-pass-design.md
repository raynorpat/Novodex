# The testing pass: design

Date: 2026-10-31. It follows the remaining-rows pass (`2026-10-30-remaining-rows-design.md`), which left 184 first-party
rows `reconstructed` with a static proof only, and the vendored-rows pass, which took the 86 vendored rows. The
starting point is `docs/reconstruction/novodex-physics/evidence/remaining-rows-handoff.md`.

## Goal

Give each of the 184 rows the project's standard for a tested row, and merge the result to `main`:

- **Wired.** The product's callers reach the transcription, and the C++ stand-in it replaces is deleted.
- **Driven.** A gated oracle differential runs it and compares its effects word for word.
- **Falsified.** A recorded mutation of the row's own instructions makes that gate fail.

A row that meets all three becomes `dynamically_gated`, and its phase closure entry moves to `closed` with
`proof: oracle_differential_falsified` and `falsification {mutation, detected}`.

A row that cannot be wired or driven safely, or whose mutations no extended family catches, is held: it stays
`reconstructed` / `reconstructed_not_falsified`, keeps its static proof, and its reason goes in its notes and the
hand-off. No row is forced.

## The rows

| Unit | Source | Rows | Wired now |
|---|---|---:|---|
| internal shape | `Physics/src/ShapeRows.cpp` | 18 | mostly; 001303, 001305, 001309, 001345 are not |
| public shape handles | `Physics/src/NpShapeHandleRows.cpp` | 28 | yes (handle tables) |
| mesh-mesh contact | `Physics/src/ContactMeshMeshRow.cpp` | 1 | yes (`NxContactMeshMesh`) |
| triangle mesh API | `Physics/src/NpTriangleMeshRows.cpp` | 24 | no |
| triangle mesh | `Physics/src/TriangleMeshRows.cpp` | 16 | no |
| PMap | `Physics/src/PMapRows.cpp` | 13 | no |
| prunable | `Physics/src/opcode/PrunableRows.cpp` | 2 | no |
| pruner support | `Physics/src/opcode/PrunerSupportRows.cpp` | 46 | no |
| Phase 6 | `Physics/src/core/Phase6Rows.cpp` | 10 | no |
| Phase 7 | `Physics/src/Phase7Rows.cpp` | 26 | no |

Thirty-one rows have a caller that a gated family already drives (the hand-off's "Rows by the test target");
153 do not. Eighty-seven have indirect call sites.

## 1. Tooling

Both tools live in `docs/reconstruction/novodex-physics/tools/` and have unittest coverage written before them.

- **`mutate_rows.py`**, promoted from `evidence/raw/phase4-pruner-*-20261020/mutate_gen.py`. For one row:
  - It picks eligible instruction lines inside the row's range in its transcription file: a jump inverted, an offset
    or immediate moved by 4, an x87 operation swapped.
  - It applies one mutation, rebuilds the named test target, and runs it against the oracle.
  - It always restores the file, and checks the restore byte for byte.
  - It logs `row, line, before -> after, exit, detected`.
  - A mutation counts only when the gate reports a detection: `mismatches=N` (N > 0) from the named family, or
    `stdout_delta=N` for a staged-pair target. A crash, timeout or build failure is logged and not counted.
  - It tries mutations until one is caught, up to a per-row limit (default 6).
- **`close_static_rows.py --falsified`**, a third mode. The manifest gives each row a gate (a registered
  oracle-differential target), a family, the caught mutation's text and its detection. The mode:
  - sets the row to `dynamically_gated`;
  - writes its `dynamic_proof`: the family that runs it and its target;
  - moves the closure entry from `deferred` / `reconstructed_not_falsified` to `closed`, with
    `proof: oracle_differential_falsified`, `gate` and `falsification {mutation, detected}`, updating the counts.

  It refuses, each case with its own test:
  - a row that is not a static-proof-only transcription from the remaining-rows pass;
  - a detection that is not `mismatches=N` or `stdout_delta=N` with N > 0;
  - a gate that is not a registered oracle-differential target.

## 2. Wiring

Each unit's evidence doc gets a wiring table: the row, the candidate code that plays its role, what changes, and the
family that drives it. The candidate code takes one of four forms:

- **A C++ function with the row's role** (same entry point, vtable slot or call site). Its body becomes a forward to
  the transcription, or its symbol is aliased to it; the old body is deleted.
- **A slot in a candidate table** (pruner, shape and mesh tables). The slot is assigned the transcription.
- **An inlined copy inside another candidate function.** The call site is changed to call the row where the oracle
  calls it there. Where the oracle inlines it too, nothing changes, and the row is driven directly.
- **No counterpart.** The candidate does not build that path (for example 001303 on the CCD path, or 001345 through
  the BASE vtable). The row stays unwired and is driven by a direct call against the oracle's RVA; the table says why.

The hand-off's deferred re-points are resolved:

- **001305:** `NxPhysicsShapeVtableTests` binds its guard words to the candidate's `gParameter`, not the oracle's
  `.data`; then the row is wired.
- **001309, 001945, 001947:** the ObjectModel-only test targets link the scene globals they lack, by adding sources to
  their CMake lists rather than keeping a C++ copy.
- **001935:** the visualisation walk passes the oracle's callback shape.

If one proves unsafe (a gate the change cannot satisfy, or a test the oracle's behaviour cannot pass), the row is
held with its reason.

Where the product and the oracle disagree, the product follows the oracle. That includes the guards the hand-off
lists as dropped in `nxSceneMarkShapeDirty` and `NxContactMeshMesh`, the API reentry refusals, and anything else wiring
changes. Tests that relied on the old C++ behaviour are adjusted to the oracle's, each with the reason in its commit.
A unit's wiring commit passes gates 3–7 before its drives start.

## 3. Drives, mutation and recording

The caught mutation is the proof that a row is driven: only a gate that runs the row and compares its effects can be
made to fail by a change inside it. No cdb trace is needed.

- **Rows whose caller is already driven** (31): after wiring, mutations run against the existing family first. If none
  is caught, the family is extended with inputs that reach the row.
- **Rows with no driven caller** (153): new families, in the test target that matches the row's subsystem.
  - **A staged-pair target** where the row is reachable through the public API from both DLLs (shapes, meshes, the
    PMap, scene queries).
  - **An oracle-differential family** in the harness otherwise (pruner internals, OPCODE-side helpers, memory-stream
    I/O). It calls the oracle's row by RVA and the candidate's by symbol, on identical inputs.
  - Every new family is exact: both outputs compared word for word, pointers taped as indices, and tape overflow a
    failure.
  - Every new family is registered in `tools/gate_targets.ps1`, and the phase floors rise to match.
  - The 87 rows with indirect calls get live objects built by each side's own constructors, as the existing families
    do.
- **Mutation and recording.** `mutate_rows.py` runs per row against that row's gate. The caught mutation and its
  detection go into the unit's manifest, and `close_static_rows.py --falsified` records the rows. A split function's
  rows each need a mutation inside their own range.
- **Held.** A row whose mutations are not caught after its family has been extended, or that cannot be driven safely.

## 4. Order and completion

The work is on branch `testing-pass`, cut from `main`, and is executed inline.

1. **Tooling:** `mutate_rows.py` and the `--falsified` mode.
2. **The already-wired units (47 rows):** shape rows (with the deferred re-points 001305 and 001309), shape handles and
   001876, including the dropped-guard and reentry inputs.
3. **Phase 4 units (101 rows):** `NpTriangleMeshRows`, `TriangleMeshRows`, `PMapRows`, `PrunableRows` and
   `PrunerSupportRows`.
4. **Phase 6:** `Phase6Rows`.
5. **Phase 7:** `Phase7Rows`, in several commits if needed, with gate 7 after each.
6. **Docs and records:**
   - each unit's evidence doc gets its wiring table and mutation log;
   - a new `evidence/testing-pass-handoff.md` lists the held rows;
   - `remaining-rows-handoff.md` points to it.

Every unit ends green:

- the Release build is clean;
- the tooling tests pass;
- `validate_inventory.py` reports `unexplained=0`;
- `work_units.json` and `completion-backlog.json` are regenerated;
- oracle-only baselines and retained proofs are re-pinned wherever a harness changed;
- gates 3–7 pass.

**Done:**

- All 184 rows are `dynamically_gated` and closed as `oracle_differential_falsified` with a recorded mutation, except
  held rows, each with its reason.
- A whole-branch review on the most capable model has run.
- The branch is merged to `main`.

**Out of scope:** the 19 held vendored rows, falsifying the other reconstructed-but-unfalsified rows, the vendored
pass's deferred minors, and pushing.
