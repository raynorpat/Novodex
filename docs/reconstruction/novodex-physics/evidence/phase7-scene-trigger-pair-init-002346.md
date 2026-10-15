# Phase 7: Scene trigger-pair list initialization (`phys_fn_002346`)

IDA identifies RVA `0x0005ab50` as the initializer for the two embedded Scene trigger-pair vectors. It clears the six begin/end/capacity words at `self+0x08` and `self+0x18`, then stores pointers to those headers at `self` and `self+0x04`. The implementation is `nxSceneListInit` in `Physics/src/Scene.cpp`, called while constructing `NxSceneInternal`.

The registered `NxPhysicsPopulatedSceneTeardownTests`, `NxPhysicsTriggerSimulationTests`, and `NxPhysicsSceneConstructorTests` staged-pair fixtures cover construction and subsequent trigger-list use. All three oracle/restored-candidate controls exit zero with identical transcripts.

For row-specific falsification, only the second header pointer was changed from `self+0x18` to `self+0x14`, and the CMake `NxPhysics` target was explicitly rebuilt. The minimal constructor target still matches, but use of the trigger lists crashes the candidate: `NxPhysicsPopulatedSceneTeardownTests` exits `-1073741819` with `stdout_delta=38`, and `NxPhysicsTriggerSimulationTests` exits `-1073741819` with `stdout_delta=32`; both oracle processes exit zero and stderr matches. Restoring `self+0x18`, rebuilding, and rerunning all three targets returns exact controls. Raw baseline, mutant, restored transcripts, and the exact mutation are retained in `evidence/raw/phase7-scene-trigger-pair-init-002346-20261015/`.

This closes only the initializer row. The full Phase 7 gate passes against the restored DLL at 2,884/2,884 assertions; its compressed transcript and the three focused differential logs are recorded in the raw evidence folder. No public header changed.
