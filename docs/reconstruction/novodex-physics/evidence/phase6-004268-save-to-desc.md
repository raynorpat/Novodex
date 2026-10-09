# Phase 6 closure: PointOnLineJoint::saveToDesc (`phys_fn_004268`)

`phys_fn_004268` is `PointOnLineJoint::saveToDesc` in `Physics/src/core/PointOnLineJoint.cpp`. The registered `NxPhysicsJointStagedPairTests` staged-pair fixture creates and saves this joint family through its public descriptor, then records the saved base anchors, axes, normals, flags, and actors.

In the isolated worktree at mainline `a54922c8`, built `NxPhysics` and `NxPhysicsJointStagedPairTests` as Win32 Release with CMake. Temporarily replaced only this row's `saveToDescBase(desc)` call with `return` and rebuilt. The registered differential caught the mutation: the first saved `anchor1` changed from `c0800000.00000000.00000000` to zero, along with other saved descriptor fields. Both processes exited 0, `stdout_delta=92`, and stderr was exact. Mutated candidate DLL SHA-256: `26e341a7193cf2a01481fba932638644e556f56e1982d96bbef72c8a4880cf8b`.

Restored the source byte-for-byte, rebuilt, and reran the same staged-pair differential: `stdout_delta=0`, both exits zero, and exact stderr. The mutation log and restored-control log are retained under the ignored `build/effector-hook-probe/` directory.
