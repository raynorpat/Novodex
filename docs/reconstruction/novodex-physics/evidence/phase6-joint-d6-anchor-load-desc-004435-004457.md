# Phase 6 closure: D6 anchor and descriptor loading

The D6 staged-pair case now reads back a second descriptor after `loadFromDesc`, exposing the public wrapper dispatch as well as the row’s existing setGlobalAnchor readback. Each row was mutated independently in the production source, rebuilt, and detected by the registered `NxPhysicsJointStagedPairTests` differential.

For `phys_fn_004435` at 0x000b0610, omitted only `NpD6Joint::setGlobalAnchor` forwarding; the public post-set anchor readback changed. The oracle and candidate both exited 0, stderr matched exactly, and `stdout_delta=4`. Mutant candidate SHA-256: `9ade2bdeb1c3937efc7bef40a561b9c945b29c1117d5c16c8ccd969687804d96`. After restoration, the candidate SHA-256 was `e654381efff414238cb72965b4fecc3b3b4f70374521ef03c0b57494668c2e8b` and the clean staged-pair differential returned `stdout_delta=0` with exact stderr.

For `phys_fn_004457` at 0x000b0990, omitted only `NpD6Joint::loadFromDesc` forwarding; the reloaded local anchors and axes stayed at their previous values. The oracle and candidate both exited 0, stderr matched exactly, and `stdout_delta=2`. Mutant candidate SHA-256: `7d8fb11972266ddcd40579fb695ee1a9f83749ee219b09ede6aa7a82b52b8b0c`. After restoration, the candidate SHA-256 was `df7afbf6512cddf46d1151dd3d2009f9f47525422f91b332bfbcce155ac1922b` and the clean staged-pair differential returned `stdout_delta=0` with exact stderr.

Build, mutation, and restored-control logs are retained in ignored `build/phase6-004435-*` and `build/phase6-004457-*` files.
