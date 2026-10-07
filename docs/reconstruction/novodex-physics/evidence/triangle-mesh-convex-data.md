# TriangleMesh convex-data reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `0b88c7b22f8e1b73bbdc8429679ed21552e7b9f3a2e07f68d855d624e3602efd`

IDA decompiled RVA `0x00053910` (preferred VA `0x10053910`) as a ConvexHull builder. It
allocates a 0x98-byte object through the Foundation allocator, initializes its base, passes
mesh vertex/face data and tolerance to the virtual build slot, and destroys the object with
an error report if construction fails. The candidate reconstruction is
`nxTriangleMeshBuildConvexData` in `Physics/src/TriangleMesh.cpp`; it builds the measured
ConvexHull layout, polygon records, normals, vertex adjacency, and bounds.

The clean staged-pair differential passed `NxPhysicsTriangleMeshApiTests` and
`NxPhysicsConvexMeshTests`: both sides exited zero, `stdout_delta=0`, and stderr matched
exactly. The covered mesh cases include a computed cube point cloud, computed and
precomputed tetrahedra, padded input points, flipped winding, padded 16-bit indices and
materials, hull arrays/polygons, and static/dynamic triangle-mesh actors.

Mutation check: temporarily made the candidate convex-data builder reject every valid cooked
mesh. The API descriptor fixture then failed on the candidate while the oracle passed.
Restoring the builder returned both targets to exact output. Degenerate hull inputs and
allocation-failure branches remain open.
