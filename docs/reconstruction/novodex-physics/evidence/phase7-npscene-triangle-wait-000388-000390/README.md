# Phase 7 NpScene triangle overlap and wait mutation closures (2026-10-12)

Two reconstructed scene APIs were already exercised by registered Phase 7 fixtures but still lacked row-specific mutations. `phys_fn_000388` is the public AABB-triangle collection wrapper; `phys_fn_000390` is the deprecated `wait` fence forwarding wrapper.

Each mutant changed only its result path, then rebuilt `NxPhysics.dll` and the corresponding test target before running that registered staged-pair differential. Both oracle and candidate processes exited zero, stderr remained exact, and each mutation changed the transcript. Rebuilding the restored source returned both targets to exact output.

| Census row | Targeted mutation | Oracle differential |
| --- | --- | --- |
| `phys_fn_000388` (`0x0000d5c0`) | `overlapAABBTriangles` returns zero | `stdout_delta=6` |
| `phys_fn_000390` (`0x0000d600`) | `wait` returns false instead of forwarding to `checkResults` | `stdout_delta=2` |

The row patches and full staged-pair transcripts are stored beside this README and under `evidence/raw/phase7-npscene-triangle-wait-000388-000390-20261012/` as losslessly compressed `.log.gz` files. No public header changed.
