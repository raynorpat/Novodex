# NpScene fluid contact-report wrappers

`phys_fn_000362` (`NpScene::setUserFluidContactReport`) and `phys_fn_000364`
(`NpScene::getUserFluidContactReport`) preserve the pinned SDK's disabled-fluid
behavior. The setter ignores its callback and reports the unavailable-feature
warning; the getter reports the same warning and returns null.

The registered `NxPhysicsSimulationTests` fixture catches an isolated message
mutation in each row. Changing only the setter warning to “Feature still
available!” produces `stdout_delta=2`; changing only the getter warning to the
same text also produces `stdout_delta=2`. For both mutants, oracle and candidate
exit 0 and stderr is exact. Rebuilding each restored source and rerunning the
differential yields `stdout_delta=0`, exact stderr, and `differential=pass`.
The fixture captures the original code, line, file, and message for both calls,
as well as the getter's null return.

The fresh full Phase 7 gate passed with 1,707/1,707 registered coverage
assertions; its transcript is archived alongside the mutation runs.

Compressed raw transcripts are in
`evidence/raw/phase7-npscene-fluid-contact-report-000362-000364-20261012/`.
