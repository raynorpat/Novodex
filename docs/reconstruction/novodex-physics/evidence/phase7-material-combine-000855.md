# Material-combine row 000855 (`phys_fn_000855`)

The registered simulation target now resolves the candidate at its staged map symbol and the oracle at RVA `0x1ca00`. Its direct DLL probe checks the full combine selector: average returns `40800000`, minimum `40000000`, multiply `41400000`, maximum `40c00000`, and an unknown selector also returns maximum `40c00000`.

Replacing only `cpmCombine0855` with an immediate return of the first argument collapses those results and is rejected by the staged differential (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4000`). The restored candidate returns the same five values as the oracle and the retry differential is exact (`stdout_delta=0`, `stderr_exact=True`). An initial restored attempt had a transient oracle access violation; the identical retry and all three subsequent full phase gates passed.

The full Phase 5, Phase 6 and Phase 7 gates pass at `2730/2730`, `1655/1655` and `1804/1804` coverage assertions. After commit `a75c417d` was fast-forwarded to main, Phase 7 passed again from the main checkout at `1804/1804`; its transcript is `evidence/raw/phase7-material-combine-000855-20261014/post-merge-phase7-gate.log.gz`. Public headers and production implementation sources are unchanged. Raw mutation, restored-control and gate transcripts are recorded under `evidence/raw/phase7-material-combine-000855-20261014/`.
