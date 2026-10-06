# `NpScene::fetchResults` contact retention

`phys_fn_000398` waits for simulation completion, dispatches scene callbacks,
refreshes simulation state, and resets the completion event. The oracle's
callback dispatcher leaves buffered contact records queued when no
`NxUserContactReport` is installed and clears them after delivery when one is
available.

The Phase 7 regression constructs a buffered contact record with no listener,
fetches once, installs a listener, and fetches again. It records both queue
retention and the callback result. Before the fix, the candidate's extra
`cpmDeliverBufferedContactReports` call cleared the queue on the first fetch;
the fixture showed `pending=0`, `calls=0`, and the staged differential was RED
with `stdout_delta=4`. Removing that candidate-only drain makes both fetches
match the oracle exactly (`stdout_delta=0`, `stderr_exact=True`; see
`build/scene-fetch-deferred-contact-green.log`). The new exact result line is
required by the Phase 7 coverage registry, whose floor is now 1,338 assertions.

Mutation falsification used a throwaway `git archive` of HEAD `1bb2955d`. The
mutation replaced `mScene->processSimulationCallbacks()` in `NpScene::fetchResults`
with a no-op. The archive was rebuilt for Release with NxPhysics and
NxPhysicsSimulationTests and run through the registered staged-pair differential.
The deferred-report assertion failed, the candidate exited 1 while the oracle
exited 0, stderr matched, and the normalized transcript differed by 944 lines:
`build/scene-fetch-results-callback-archive-mutation.log`.

This closes the fetch-results row's callback-dispatch behavior. Actor-contact
dispatch inside `NxSceneInternal::processSimulationCallbacks` has a separate
row and mutation proof in `evidence/fetch-contact-callback-mutation.md`.
