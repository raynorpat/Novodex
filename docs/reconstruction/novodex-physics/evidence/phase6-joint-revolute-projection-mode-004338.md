# Phase 6 closure: revolute projection mode setter (`phys_fn_004338`)

The registered index-0 revolute staged-pair fixture now calls `NxRevoluteJoint::setProjectionMode(NX_JPM_POINT_MINDIST)` and prints the public `getProjectionMode()` result. The Phase 6 registry requires `projection_mode=1`.

Built the Win32 Release `NxPhysics` and staged-pair targets. A temporary mutation omitted only `mProjectionMode = mode` from `RevoluteJoint::setProjectionMode`; the getter returned 0 instead of 1. The oracle/candidate differential caught it with both exits 0, `stdout_delta=2`, and exact stderr. Mutant candidate SHA-256: `51a40d5bb7bf99dcd25244786c5a9153d2d2b91b8f747a88042ff82df17272fe`.

Restored the assignment and rebuilt. The restored differential passed with both exits 0, `stdout_delta=0`, and exact stderr; restored candidate SHA-256: `e43597e018141d55a32b7e04ef8c6b07e30c16720952c699f88ec9dcb7bc4466`. Full logs are retained in the ignored `build/phase6-004338-*` files.
