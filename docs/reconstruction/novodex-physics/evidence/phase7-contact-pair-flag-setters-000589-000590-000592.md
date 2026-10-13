# Scene pair-flag setters: rows 000589, 000590, and 000592

The registered `NxPhysicsPairFlagTests` fixture now covers the three setter
rows in the Scene call chain. `phys_fn_000589` replaces or removes the hash
record and preserves the allocated notification state; `phys_fn_000590`
rejects an identical shape at `Scene.cpp` line 904 and forwards valid flags;
`phys_fn_000592` rejects an identical actor at line 930 and forwards its first
internal shapes through the shape setter. The Scene path explicitly selects
the recovered `const NxU8*` implementation of row 000589 rather than the
older `void*` helper overload.

The fixture asserts one error for a same-shape setter and one for a same-actor
setter, stores nonzero flags, clears an allocated notification record, and
checks the resulting getter values. Before each implementation fix, the
focused staged-pair differential exposed the missing same-actor diagnostic.

Each row was independently replaced with a no-op, rebuilt, and run through the
registered oracle/candidate differential:

- `phys_fn_000589`: rejected, `candidate_exit=1`, `stdout_delta=23`.
- `phys_fn_000590`: rejected, `candidate_exit=1`, `stdout_delta=26`.
- `phys_fn_000592`: rejected, `candidate_exit=1`, `stdout_delta=15`.

After restoring each row, the focused control is exact (`stdout_delta=0`,
`stderr_exact=True`). The complete Phase 7 gate is recorded in the raw evidence
directory alongside the mutation logs.

The fresh complete Phase 7 gate passes at 1,727/1,727 coverage assertions.
Fresh Phase 5 and Phase 6 gates also pass at 2,685/2,685 and 1,614/1,614.
Both pinned public-header manifests pass all 80 files, and inventory validation
passes with 6,338 functions, 5,138 data objects, and zero unexplained rows.

Raw logs: `evidence/raw/phase7-contact-pair-flag-setters-000589-000590-000592/`.
