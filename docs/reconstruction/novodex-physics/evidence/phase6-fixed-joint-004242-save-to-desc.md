# Phase 6 closure: fixed-joint descriptor save (`phys_fn_004242`)

`phys_fn_004242` is `FixedJoint::saveToDesc`. For a live fixed joint it tail-calls the shared `Joint::saveToDescBase` path, which fills the base descriptor fields; a broken joint instead reports `NXE_INVALID_PARAMETER` at the row's source line and returns. The registered public `NxPhysicsJointStagedPairTests` fixture reads a fixed joint back through `NxFixedJointDesc` and records its saved anchors, axes, normals, flags, and actors.

In the isolated worktree at mainline `b70f564a`, built `NxPhysics` and `NxPhysicsJointStagedPairTests` as Win32 Release with CMake. Temporarily replaced the single `saveToDescBase(desc)` call in `FixedJoint::saveToDesc` with `return`. The staged-pair test caught the row-specific mutation: the first fixed case's saved `anchor1` changed from `c0800000.00000000.00000000` to zero, and other saved descriptor fields changed as well. The full differential reported both processes exit 0, `stdout_delta=92`, and exact stderr. Mutant candidate DLL SHA-256: `cc55d6644d57c4cfc8e5f2cfca96a8d729367aa6edff46dbf9e1065ad7f42d1d`.

Restored `FixedJoint.cpp` byte-for-byte, rebuilt, and reran the registered differential. It passed with both processes exiting 0, `stdout_delta=0`, and exact stderr. Restored candidate DLL SHA-256: `edaa4d06d17399a125a6a9a64120f3a2903f1138380ead5a2fbba52786cb9335`; Foundation SHA-256: `d97c3f14eacc4c7953ae2d131c3366c163c3854d2c61247082d79b07600e724e`.

The control and mutation transcripts are retained in the ignored `build/effector-hook-probe/` directory. The row is closed by its existing public differential; no public headers changed.
