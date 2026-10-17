# Phase 7 row 000537: x87 control-word save

`phys_fn_000537` (`0x000106d0`) reads the caller's x87 control word before scene stepping changes precision and rounding modes. The candidate performs that row's operation as the inline `fnstcw` at the start of `NxSceneInternal::simulateFrame` in `Physics/src/Scene.cpp`, then restores the saved word after simulation. The current CMake Release map resolves the containing method `?simulateFrame@NxSceneInternal@@QAEXXZ` at VA `0x1006e4d0` in `Scene.obj`; the `fnstcw` operation is inline and has no standalone candidate symbol.

For mutation testing, the save was replaced with a zero control word and `NxPhysics.dll` rebuilt. The registered `NxPhysicsSimulationTests` staged-pair differential detected the change: oracle exit 0, candidate exit `-1073741681` from a floating-point exception, `stdout_delta=3997`, and exact stderr. Restoring the `fnstcw` path and rebuilding returned both exits to zero with `stdout_delta=0` and exact stderr.
