
The full Phase 6 gate passed after registering NxPhysicsActorShapeMutationTests for this row: phase_gate=6 status=pass, 1,554/1,554 coverage assertions, all ten differential targets exact. Inventory validation reports 99 closed and 334 deferred Phase 6 functions.


Reproduction commands:

- `cmake --build build --config Release --target NxPhysics NxPhysicsActorShapeMutationTests`
- `powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsActorShapeMutationTests -RepoRoot <repo> -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot <repo>\build\pairs`
- Repeat the build with `--clean-first` after restoring `Physics/src/core/Joint.cpp`, then repeat the differential command.

No public headers or production behavior were changed; the source was restored before the final clean build.
