# Buffered actor-contact callback mutation

`phys_fn_000640` (`NxSceneInternal::processSimulationCallbacks`) dispatches
buffered actor-contact records through the configured `NxUserContactReport`.
The existing Phase 7 simulation test exercises both real contact generation
and the fetch callback fixture.

For a row-targeted mutation check, a throwaway `git archive` copy of HEAD
`0ac9f256` replaced
`contactReport->onContactNotify(pair, events);` with a no-op in
`Physics/src/Scene.cpp`. The copy was configured and rebuilt for Release with
`NxPhysics` and `NxPhysicsSimulationTests`, then run through
`run_differential.ps1 -Targets NxPhysicsSimulationTests` against the pinned
oracle pair. Oracle and candidate both exited 0, stderr matched, and the
candidate transcript diverged by 937 normalized lines. The runner rejected the
mutant as expected:
`build/scene-contact-callback-branch-archive-mutation.log`.

This closes the actor-contact dispatch branch for this row. Trigger queue
generation remains separate coverage. It does not close `NpScene::fetchResults`
(`phys_fn_000398`): that row still has a second contact-delivery helper whose
mutation has not been observed independently.
