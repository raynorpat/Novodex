# NpFluidEmitter world-pose setters (`phys_fn_003798`, `003800`, `003802`)

The oracle bodies at RVAs `0x8c380`, `0x8c430`, and `0x8c4e0` implement the write-locked `setGlobalPose`, `setGlobalPosition`, and `setGlobalOrientation` methods. The candidate CMake map resolves the corresponding methods in `NpFluidEmitter.obj` at `0x1003ed00`, `0x1003edd0`, and `0x1003ecb0`. The existing registered component-setter fixture compares oracle, candidate-source, and candidate-DLL callback records, resulting pose words, and x86 stack balance.

Each method was mutation-tested separately by replacing its candidate entry with `ret 4`. The `setGlobalPose` mutation is detected in both world-position and world-orientation cases (`mismatches=2`); the position and orientation mutations each report one mismatch. The restored candidate DLL hash is exact. Fresh Phase 5/7 gates pass at 2,842/2,842 and 2,975/2,975 assertions; all 817 reconstruction-tool tests pass. Raw logs are under `evidence/raw/phase7-fluid-emitter-world-pose-setters-003798-003800-003802-20261017/`.


## Full-state cache refresh continuation

The original component fixture checked callback records and local pose words but did not include the cached backend world pose. Extending it to compare the full 24-word local/cache region found that all three world setters omitted the oracle's final `EmitterGetGlobalPose` cache refresh. The candidate now calls `nxNpFluidEmitterStoreGlobalPose` after publishing the pose. The stronger fixture still rejects row-specific no-op mutations for `003798`, `003800`, and `003802` with 2, 1, and 1 mismatches, and the restored oracle/source/DLL comparison is exact. The same correction and test expansion also close adjacent local setter rows `003810/003812/003814`; see `phase7-fluid-emitter-local-pose-setters-003810-003812-003814.md`.
