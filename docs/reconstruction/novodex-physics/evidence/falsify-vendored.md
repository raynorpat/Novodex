# Falsifying the vendored qhull and OPCODE rows (sub-project 2, fast gate)

**The gate.** The vendored sources are merged (upstream + NovodeX overlay) into `build/External/<lib>-tree` at configure time
and compiled from there. `mutate_cpp.py` mutates the merged copy (`vendor_gate.merged_path`), so the repository sources are never
touched, and a round (relink `NxPhysics.dll` and `NxPhysicsThirdPartyTests`, run) takes about ten seconds. The gate is
`NxPhysicsThirdPartyTests`, which prints one `thirdparty name=<family> ...` line per family. A row is detected when a line of
one of its families (read from `vendored_coverage.csv`, intersected with the names the target prints) differs from the unmutated
run; a row with no named family sees any change. The recorded count (`mismatches=N`, the kind the closer admits for an oracle
target) is the number of that row's family lines that changed, not a count of disagreeing checks; the gate's pinned expected
lines make any such change a real gate failure. A crashed run is **not** a catch (its truncated output would look like a change
in every family). The DLL range check, the oracle-range check for listing transcriptions and the shared-function guard are those
of the first-party pass.

**Method.** First pass: five workers in parallel, each with its own build directory (`NX_BUILD_DIR`), 428 batches of up to 6
mutations per row, rows with disjoint families sharing a build. An independent review then showed that the first pass had counted
crashes as catches (21 closed rows had exactly 51 changed lines, the whole output lost) and had let screened candidates decide
closures in shared builds. Every row that had closed was re-surveyed one at a time (no cross-row contamination, no screening) with
8 mutations, crashes not counted, and closes only with at least one confirmed catch and at most `min(2, tried / 4)` survivors.

**Result.** 636 vendored rows: **41 closed** (all qhull: 38 C functions and 3 listing transcriptions; no opcode row held up), 69 of the first-pass
closures reopened and deferred with their survey counts (53 caught but with too many survivors, 16 nothing caught: their first-pass
catches were crashes), and 526 carry a deferral note from the first pass (214 surveyed with too many survivors, nothing caught
or nothing to mutate; 312 not surveyed: function not found 48, source file missing 29, candidate function larger than 2x the row
and so shared 95, no source file 21, no unique map symbol 119). The 95 "shared" are labelled `homeless_shared_code` on a size
heuristic alone; inlining by the compiler gives the same result.

Evidence: `evidence/raw/falsify-vendored-20261103/` (`survey.log` and `deferrals*.json` for the first pass; `solo/survey.log`,
`solo/caught.log`, `solo/manifest.json` for the re-survey that the closures stand on). Mutation texts name the file the
mutation actually landed in (the overlay copy where one shadows the upstream file).

**Caveats.** A family shared by many rows can be moved by any of them; candidates are confirmed alone. A family line that prints
identically under a mutation leaves the row a survivor. `parse_lines` ignores lines without `name=` and keeps the last of two
lines with one name (only ever adds survivors).
