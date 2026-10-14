# Actor-group pair insertion and lookup (`phys_fn_000433`, `phys_fn_000435`)

The existing `NxPhysicsSDKTests` fixture exercises actor-group pair flag insertion, forward/reverse lookup, clearing, and defaults against the pinned DLL. A clean baseline reported `stdout_delta=0` and exact stderr.

Two row-specific mutations were rebuilt and run through the registered staged-pair differential:

- `phys_fn_000433` (`PhysicsSDK::setActorGroupPairFlags`): removing the `cpmSetActorGroupPairFlags` call changed the transcript (`stdout_delta=6`).
- `phys_fn_000435` (`PhysicsSDK::getActorGroupPairFlags`): forcing the valid-key path to return zero changed the transcript (`stdout_delta=2`).

After restoring the original source and rebuilding, the differential returned to `stdout_delta=0` with exact stderr. The fresh Phase 3 gate passed at 945/945 coverage assertions and ran `NxPhysicsSDKTests` with exact oracle/candidate output. The two Phase 2 rows are therefore recorded as discharged by Phase 3. Phase 2 totals are 76 closed / 67 deferred. Public headers and CMake are unchanged.

Raw build, mutation, restored-control, and Phase 3 gate transcripts are compressed under `evidence/raw/phase2-actor-group-pair-core-000433-000435-20261014/`.
