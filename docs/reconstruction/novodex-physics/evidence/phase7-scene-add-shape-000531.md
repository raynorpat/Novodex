# Phase 7 row 000531: scene shape registration

`nxSceneAddShape` (`phys_fn_000531`, oracle RVA `0x00010600`) performs the scene-side registration for a newly added shape. It calls the shape's slot 6 with `1`, adds the shape to the pruning structure using its owner and `hasRecord`, recomputes the scene actor count, and notifies the fluid manager when present.

The registered `NxPhysicsActorShapeMutationTests` suite drives this row. In a source mutation, the function was replaced with a no-op and `NxPhysics.dll` was rebuilt. The staged-pair differential reported `oracle_exit=0`, `candidate_exit=0`, and `stdout_delta=142`. After restoring the source and rebuilding, the staged-pair differential returned `stdout_delta=0` with exact stderr. The broad `NxPhysicsSimulationTests` target alone did not catch the mutation and is not the proof.
