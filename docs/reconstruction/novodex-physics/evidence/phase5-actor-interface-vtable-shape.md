# Actor interface vtable shape

The private `NpActorVtable` had declared an extra `setGlobalPose(const NxVec3&, const NxMat33&)` virtual with an empty body. The frozen public `Physics/include/NxActor.h` declares only the `NxMat34` overload, and the oracle's actor interface table has 87 public slots. The extra candidate method had no oracle row and occupied a candidate-only slot at the end of its primary table.

Removed the private declaration and empty definition without changing any public header. The rebuilt `build/Release/NxPhysics.map` contains `?setGlobalPose@NpActorVtable@@UAEXABVNxMat34@@@Z` and no `?setGlobalPose@NpActorVtable@@UAEXABVNxVec3@@ABVNxMat33@@@Z` symbol. The latter is the candidate-only overload removed by this change.

Verification on the rebuilt Win32 Release DLL:

- `cmake --build build --config Release --target NxPhysics NxPhysicsActorLifecycleTests NxPhysicsObjectLayoutTests --parallel 8` succeeded.
- The staged `NxPhysicsActorLifecycleTests` differential passed with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -PairsRoot D:\github\Novodex\build\actor-vtable-cleanup-phase5-pairs` passed; 2,563/2,563 required coverage assertions ran, including the object-layout oracle checks.
- The Phase 5 runner's immutable-header checks passed for all 80 Physics headers in both the repository and UE3 tree.

This removes one incorrect candidate-only virtual. It does not prove all actor slots or close the remaining Phase 5 rows.
