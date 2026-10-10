# Phase 7 coherent pruner-cache teardown — phys_fn_001974

IDA pseudocode for `NxPhysics.dll+0x0004c260` reads the coherent cache pointer at engine+0x2c, invokes `SweepAndPrune::~SweepAndPrune`, frees the object through the SDK allocator, and clears the field. `nxSceneEngineReleaseCoherent` in `Physics/src/opcode/IcePruningEngine.cpp` implements that sequence; Scene teardown calls it with `Scene+0x624`.

The registered `NxPhysicsPopulatedSceneTeardownTests` fixture selects coherent broad phase, steps a public scene, verifies that the retained cache is a tracked 44-byte allocation, then verifies it is freed on release. A temporary mutant that cleared the field without destroying/freeing the cache was rejected by the staged differential: the candidate retained 22 allocator blocks where the oracle retained 15 and exited 1 (`stdout_delta=17`). The restored oracle/candidate control passes exactly (`stdout_delta=0`, both exits 0, exact stderr).

Candidate symbol: `?nxSceneEngineReleaseCoherent@@YAXPAX@Z` at `0x1009e190` in `IcePruningEngine.obj` (`build/Release/NxPhysics.map`).
