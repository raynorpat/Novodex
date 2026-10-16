# Phase 7 `phys_fn_003609`: fluid emitter initialization

IDA places the emitter wrapper metadata, rest-particle-distance callback, backend emitter creation callback, descriptor flags, optional frame actor, local pose, and global-pose update/store sequence in the 377-byte initializer at RVA `0x00089290`. The candidate now mirrors that boundary in `nxFluidEmitterInitialize` in `Physics/src/Scene.cpp`; its direct helpers map to the already-gated name/registry and emitter global-pose rows `phys_fn_000480`, `phys_fn_003559`, and `phys_fn_003605`.

The registered `NxPhysicsSimulationTests` staged-pair differential exercises descriptor emitters, metadata, callback arguments, flags, frame-actor binding, and resulting world pose. A no-op mutation of the optional frame-actor store was rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=70`); restoring and rebuilding returned the differential to `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, with exact stderr. The fresh full Phase 7 CMake/Win32 Release gate passed at 2,913/2,913 coverage assertions.

The source mapping and callback/pose behavior are verified through the public creation path; indirect FluidModel callback targets remain supplied by the test fixture. Public Physics headers are unchanged.
