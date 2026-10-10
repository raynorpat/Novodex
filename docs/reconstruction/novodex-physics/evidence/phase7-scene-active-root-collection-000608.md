# Phase 7 active-root collection row `phys_fn_000608`

`phys_fn_000608` (`NxPhysics.dll+0x11190`, 124 bytes) is represented in `NxSceneInternal::simulateFrame` in `Physics/src/Scene.cpp`. Its active-root pass walks the Scene body-record range `[+0x56c,+0x570)`, resolves each record's sleep-group root through `phys_fn_000713`, retains only self-parented awake roots, and appends them to `[+0x57c,+0x580)` for the following effector and solver passes. The same reconstructed step also rebuilds contact-pair auxiliary links before root collection.

Mutation falsification: changed the root-retention condition to always continue, so no active root reaches the solver. The registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential rejected the candidate: oracle exit 0, candidate exit 1, `stdout_delta=6726`; the candidate failed `joint-break notify callback count was not one`. This observable depends on the scene's active roots being simulated. Restoring the condition and rebuilding returned both exits to 0, `stdout_delta=0`, and exact stderr.

Retained command transcripts: `build/phase7-608-mutation-build.log`, `build/phase7-608-mutation-diff.log`, `build/phase7-608-control-build.log`, and `build/phase7-608-control-diff.log`. The fixture is part of the existing registered simulation suite; no public Physics header changed.
