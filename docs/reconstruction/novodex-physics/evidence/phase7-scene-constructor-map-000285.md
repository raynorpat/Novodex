# `NpScene` constructor map (`phys_fn_000285`)

IDA identifies oracle RVA `0x0000c310` (VA `0x1000c310`) as a 216-byte `__thiscall` constructor taking `NxSceneInternal*` and returning the complete `this` pointer with `ret 4`. The constructor installs the primary and embedded-hook vtables, constructs the embedded lock and two lock objects, initializes the event handles and fields at `+0x1c/+0x20`, stores the internal Scene pointer at `+0x24`, creates two 4-byte allocator-owned lock links, and creates the 0x18-byte condition object before starting its worker.

The rebuilt mainline map resolves `??0NpScene@@QAE@PAVNxSceneInternal@@@Z` to preferred VA `0x10041b70` in `NpScene.obj`. This maps the row to `Physics/src/NpScene.cpp#NpScene::NpScene`; `Physics/src/Scene.cpp:791` constructs this wrapper after the internal Scene is initialized. The candidate class layout is documented in `Physics/src/include/NpScene.h`.

The existing `NxPhysicsSceneConstructorTests` staged differential checks that the newly created public Scene points to its internal Scene at `+0x24`. Its prior `mScene = 0` mutation was rejected (`stdout_delta=2`), and the restored control matched exactly. This mapping record does not close the full constructor: detailed failure behavior for the nested lock and worker-condition allocations remains open.
