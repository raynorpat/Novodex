# TriangleMesh convex reload reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `49141d0209e504f1e27676c699e2cb521d387870795bdf5b7e16d83c9d2328b1`

IDA decompiled RVA `0x00053b70` (preferred VA `0x10053b70`) as the convex-hull state
transition. It destroys and clears an existing convex object when called directly, then
either computes a hull or initializes convex data from the current mesh. In the loader,
the oracle calls this transition only if the mesh has no convex object and the new descriptor
requests convex data.

Added a public reload sequence to `NxPhysicsTriangleMeshApiTests`: load a computed convex
tetrahedron, reload a precomputed tetrahedron with different vertices, then reload a plain
mesh. The oracle retains the first convex hull and its convex flag through all three steps.
Before the fix, the candidate rebuilt the hull for the second descriptor and removed it for
the plain descriptor; the differential went RED. `TriangleMesh::loadFromDesc` now preserves
an existing convex object and initializes one only when absent. The staged-pair run of
`NxPhysicsTriangleMeshApiTests` and `NxPhysicsConvexMeshTests` is exact: both processes exit
zero, `stdout_delta=0`, and stderr matches.
