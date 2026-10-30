# The vendored rows: design

Date: 2026-10-30. Follows the remaining-rows pass (`2026-10-30-remaining-rows-design.md`), which closed every
first-party row and held these.

## Goal

Move the 86 vendored rows still `discovered` (71 qhull, 15 OPCODE) to `reconstructed`. Each row carries both:

- **a static proof:** a naked listing transcription that `listing_compare_lib.py` finds instruction-for-instruction
  equal to the oracle;
- **a dynamic proof:** an oracle-vs-candidate execution of its group that matches in every word (Task 5b arm (i),
  `evidence/vendored-correspondence.md`, "Task 5b: promotion"). The rule is unchanged.

A group that cannot be driven safely stays `discovered`, with the reason in its row notes and the hand-off. None is
expected; none is forced.

## Where the rows stand

From `evidence/vendored-correspondence.md`, "Remaining-rows pass":

| Library | Standing | Rows |
|---|---|---:|
| opcode | DIFF | 6 |
| opcode | review `equivalent`, no x87 | 1 |
| opcode | x87 (the 8 ICE plane, triangle and matrix rows the final review held) | 8 |
| qhull | DIFF | 27 |
| qhull | review `equivalent`, no x87 | 28 |
| qhull | x87 | 16 |

By `vendored_coverage.csv`, the rows fall in 64 groups. 32 groups already execute (18 `exact`, 13 `discrete`, 1
`layout`); 32 do not (30 qhull, 2 OPCODE).

## 1. Transcription

The vendor rule (`External/CMakeLists.txt`) holds: `upstream/` is never edited; a file in `novodex/` wins.

- **Guards.** Each transcribed function's body is wrapped in `#ifndef NX_VENDORED_LISTING_ROWS` in its `novodex/`
  copy, with a one-line comment naming the transcription file. A file with no `novodex/` copy gets one, identical to
  upstream apart from the guards (qhull `merge.c`, `poly.c`, `qhull.c`, `stat.c`, `global.c`; OPCODE's Ice
  triangle, matrix, plane and container files, as the rows need). `NX_VENDORED_LISTING_ROWS` is defined on
  `NxQhull` and `NxOpcode` only.
- **Transcription files.** `External/qhull/novodex/QhullRows.cpp` and `External/opcode/novodex/OpcodeVendoredRows.cpp`,
  added to the libraries' source lists.
- **Original symbols, no aliases.** qhull rows are `extern "C" __declspec(naked)` definitions under their own names
  (`qh_distplane`, ...). OPCODE rows are naked member definitions (`__declspec(naked) float Triangle::Area() const`),
  so the mangled names come from the class declarations. Callers left in upstream source link to them unchanged.
- **Tooling.** `tools/transcribe_rows.py`, with a bundle per library in
  `evidence/raw/vendored-rows-{qhull,opcode}-20261030/` (cfg, header, manifest, `listing-compare.txt`).
  - `CALLS` names callees by their vendored symbols (`{qhull,opcode}_match.csv`).
  - `DATA` names qhull's `qh_qh`, `qhmem`, `qhstat` and OPCODE's statics. The transcriber already refuses unnamed
    writable data.
- **Split rows.** A function spanning several rows (`qh_voronoi_center`, 4; `qh_init_qhull_command`, 3; others) is
  transcribed once over its whole range, and its rows close together.
- **Static proof.** `evidence/vendored-rows-{qhull,opcode}-listing-compare.py` (on `listing_compare_lib.py`)
  reports ALL EQUAL with zero unexpected mappings. A `tools/vendored_match.py` re-run classes every transcribed group
  MATCH, so the DIFF class, and with it the DIFF-equivalent arm, no longer applies to these rows.

## 2. Execution

All drives live in `tests/PhysicsThirdPartyTests.cpp` and keep its rules:

- each input is built once and handed to both sides;
- the oracle runs at its RVAs on objects its own constructors initialise;
- output words go on two tapes, compared word for word;
- pointers are taped as indices.

The candidate side is the vendored code linked into the test exe, so the transcriptions are what executes.

- **Groups already executed (32).** No new drive. A re-trace should show the 13 `discrete` groups and the `layout`
  one `exact`, since the instructions now match.
  - **Isolation fallback.** If a family still diverges because an upstream callee groups its sums differently, the
    row gets a direct call by RVA on identical inputs, so its result no longer depends on that callee.
- **New drives for the 32 unexecuted groups.** One new section, `nxDriveVendoredRows`, with families named
  `vendored_rows_<group>`:
  - **qhull output paths:** `qh_printvoronoi`, `qh_printvdiagram2`, `qh_eachvoronoi`, `qh_detvridge3`,
    `qh_printhashtable`, `qh_setprint`. Whole qhull runs on both sides, with the options that reach them (`v`, `Fv`,
    `FN`, `Fi` and the like). Each side writes to its own temporary `FILE*`, and the bytes written are taped.
  - **qhull initialisation and option paths:** `qh_init_qhull_command`, `qh_initqhull_start`,
    `qh_initqhull_globals`, `qh_initqhull_mem`, `qh_initqhull_buffers`, `qh_scalelast` (`Qbb`), `qh_setdelaunay`
    (`d`). Each runs with an option string that reaches it. The `qh` block is taped afterwards, pointer fields as
    indices or skipped, as the existing qhull families do.
  - **merge, vertex and joggle paths:** `qh_mergecycle*`, `qh_find_newvertex`, `qh_vertexridges`, `qh_joggleinput`
    (`QJ`), `qh_rotateinput` (`QR`), and the others. Hulls run on inputs that force them (coplanar clusters for the
    merges; `QJ` with a fixed seed). Facets and vertices are taped as `qhull_hull` tapes them.
  - **`qh_errprint` and `qh_printallstatistics`.** `qh_errprint` runs on a facet built for it, and
    `qh_printallstatistics` after a hull. File bytes are taped. Nothing goes through `qh_errexit`.
  - **The two unexecuted OPCODE groups:** direct calls by RVA on oracle-constructed objects, as `nxDriveIceMaths`
    does.
- **Evidence.** A `tools/vendored_trace.py` re-run over the new exe regenerates the `hits`, `outcome` and
  `family_best` columns of `vendored_coverage.csv` and `evidence/vendored-trace-{qhull,opcode}.txt`.
- **Held.** A group that would abort the oracle, or that nothing reaches safely, stays `discovered`. Its trace line,
  row notes and the hand-off say why.

## 3. Promotion and recording

- **The rule (arm (i), unchanged).** A row is promoted when its group:
  - is MATCH after the re-match;
  - has `family_best=exact` in the re-traced `vendored_coverage.csv`;
  - has a proof that lists and attributes every divergent family the group also ran in;
  - has no open triage item.

  The listing compare stands in for the hand review.
- **The tool.** `tools/close_static_rows.py` gains a `--vendored` mode. Without the flag it keeps refusing vendored
  rows. With it, it refuses any row whose group lacks `family_best=exact` in the coverage CSV, and any row that is
  not vendored. It writes:
  - `static_proof`, the listing compare;
  - `dynamic_proof`, the exact family and its trace line;
  - the state `reconstructed`;
  - a move of the closure entry from `vendored_not_falsified` to `reconstructed_not_falsified`.

  Tests, written failing first, cover both refusals.
- **Ceilings.** A family that becomes exact has its `kDivergentCeilings` entry lowered to zero, or removed if nothing
  in it diverges any more (`qhull_hull_x87`, `ice_plane_triangle`, `ice_matrix4x4`, and any other that changes). A
  regression then fails the Phase 4 gate.
- **Docs.**
  - `evidence/vendored-correspondence.md` gets a "Vendored-rows pass" section: the rows by group, each row's proving
    family, and anything held, with the reason.
  - `evidence/remaining-rows-handoff.md` drops the vendored hold.
  - The 8 ICE rows' notes keep why they were held and add what released them.
- **Registry.**
  - `validate_inventory.py` reports `unexplained=0`.
  - `work_units.py` and `report_completion.py` are regenerated.
  - Oracle-only baselines and proofs are re-pinned wherever a fixture hash changes.
  - Gates 3–7 (`run_phase_gate.ps1`) pass.

## 4. Order and completion

Branch `vendored-rows` from `main` (`6e1ab2f41`), executed inline.

1. **Tooling:** the `--vendored` mode, and any transcriber support C++ naked members need. Tests first.
2. **OPCODE (15 rows):** guards and copies, `OpcodeVendoredRows.cpp`, listing compare, re-match, re-trace,
   promotion, ceilings. This goes first because it is small and exercises the C++ member path.
3. **qhull float-divergence sources and their callees:** `qh_distplane`, `qh_getdistance`, `qh_gram_schmidt`,
   `qh_getangle`, `qh_randommatrix`, and the rest of the x87 rows.
4. **The remaining qhull rows existing drives reach:** the merge, hash and vertex rows.
5. **New drives:** output, initialisation and option, merge and joggle, error-print and statistics paths, then the
   unexecuted OPCODE groups.
6. **Docs, registry and hand-off;** gates 3–7.

**Every stage ends green:**

- the Release build is clean;
- the listing compare reports ALL EQUAL;
- the tooling tests pass;
- the harness is deterministic and within its ceilings;
- `validate_inventory.py` reports `unexplained=0`;
- gates 3 and 4 pass.

The final stage also runs gates 3–7.

**Done:** all 86 rows `reconstructed` with static and dynamic proofs, except any group recorded as undrivable under
section 2. Then one whole-branch review on the most capable model, and the user decides on the merge.

**Out of scope:** rows already promoted, other vendored functions, wiring changes outside qhull and OPCODE, and
pushing.
