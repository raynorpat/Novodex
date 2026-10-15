# `PhysicsSDK::getScene` row proof (`phys_fn_000450`)

The isolated `NxPhysicsSDKSceneLookupTests` staged-pair fixture creates two scenes and checks that indexes 0 and 1 return the corresponding wrappers. It then releases index 0, checks that swap-removal preserves the remaining scene at index 0, and confirms the registry is empty after the final release.

A row-local candidate mutation made index 1 return the first scene. The oracle remained unchanged; the candidate reported `first=1, second=0` and failed (`stdout_delta=6`). The restored candidate matches exactly (both exit zero, `stdout_delta=0`, exact stderr).

The fixture runs in Phase 2 as well as its focused differential. Raw build and paired-run transcripts are in `evidence/raw/phase2-sdk-scene-lookup-000450-20261015/`.
