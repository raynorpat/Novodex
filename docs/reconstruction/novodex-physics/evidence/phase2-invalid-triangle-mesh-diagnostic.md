# Invalid triangle-mesh descriptor diagnostic

`NxPhysicsTriangleMeshApiTests` now creates an invalid default `NxTriangleMeshDesc` through the public `NxPhysicsSDK::createTriangleMesh` API while recording `NxUserOutputStream` callbacks. The oracle returns null and emits one `NXE_INVALID_PARAMETER` callback: source `\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp`, line 498, message `PhysicsSDK::createTriangleMesh: desc.isValid() is false!`. The candidate previously returned null without reporting the error.

`PhysicsSDK::createTriangleMesh` now reports the same callback before returning. The case is registered in the Phase 4 differential coverage lines. The focused paired target passed with `stdout_delta=0` and `stderr_exact=True` after rebuilding `NxPhysics.dll`; both DLLs also agree on the three descriptor cooking/readback cases.

The fix is committed as `1e33e4da`; the differential harness and coverage assertion were committed earlier in `08820366`. Public headers are unchanged.
