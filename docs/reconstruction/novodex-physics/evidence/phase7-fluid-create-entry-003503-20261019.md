# Phase 7 FluidCreate entry (`phys_fn_003503`)

On 2026-10-19, the existing enabled-fluid fixture was given a row-specific marker for the FluidCreate callback path at oracle RVA `0x00086080`. It records exactly one invocation and hashes each of the 14 non-manager callback arguments. Oracle and candidate both report `digest=1d2e206d` with zero mismatches. The user-facing public headers remain unchanged.

A temporary candidate mutation substituted callback argument 14 with argument 13 in `NxSceneInternal::createFluid`. The marker changed to `digest=de1a8fc9 mismatches=1`; `NxPhysicsSimulationTests` rejected the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=78`). Restoring the argument produced an exact staged differential (`stdout_delta=0`, `stderr_exact=True`). This row-specific mutation closes the callback argument mapping gap alongside existing coverage for descriptor state, callback ordering, emitters, manager refresh, initial-particle copying, and edge/failure paths.

Fresh gates passed with the registered marker on both staged DLLs:

- Phase 5: 2,970 / 2,970 coverage assertions.
- Phase 6: 1,820 / 1,820 coverage assertions.
- Phase 7: 3,112 / 3,112 coverage assertions.

All 837 reconstruction-tool tests pass, and inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained rows. The three phase gates each ran both public-header checks. The global static-proof audit still reports 8,411 findings, largely outside this row; its output is retained for the project-wide backlog.

Raw compressed transcripts and their checksums are in `evidence/raw/phase7-fluid-create-entry-003503-20261019/`.
