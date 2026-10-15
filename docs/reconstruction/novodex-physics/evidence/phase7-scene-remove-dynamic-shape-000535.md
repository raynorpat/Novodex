# Phase 7 row 000535: dynamic shape removal

`nxSceneRemoveDynamicShape` (`phys_fn_000535`, oracle RVA `0x000106a0`) removes a dynamic shape from the scene's pruning structure and notifies an attached fluid manager that the dynamic shape changed.

The registered `NxPhysicsActorShapeMutationTests` differential reaches this row. Replacing it with a no-op that returns true, then rebuilding `NxPhysics.dll`, caused the candidate to exit with access violation `-1073741819`; the oracle exited 0 and the transcript delta was 220 lines. After restoring and rebuilding, both sides exited 0 with `stdout_delta=0` and exact stderr.
