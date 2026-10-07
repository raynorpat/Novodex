# TriangleMesh destructor reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `aac7df85d5125d511f4aa6929291d6c087c61082ca323a9b751a8cf154175a8a`

IDA decompiled RVA `0x00055570` (preferred VA `0x10055570`) as the TriangleMesh
destructor. It releases and clears the public wrapper, calls the convex-data/PMap cleanup
helper, then destroys the internal mesh arrays. The candidate implementation is
`TriangleMesh::~TriangleMesh` in `Physics/src/TriangleMesh.cpp`, with the stable row comment
at the destructor.

`NxPhysicsTriangleMeshApiTests` now tracks outstanding SDK-allocator blocks around one
triangle-mesh create/release pair and asserts that release returns to the starting level.
The staged-pair differential passed that target and `NxPhysicsConvexMeshTests` with both
processes exiting zero, `stdout_delta=0`, and exact stderr. The API tests also release
computed convex meshes and meshes carrying loaded PMaps.

Mutation check: temporarily omitted the vertex-array free in the candidate destructor. The
new fixture changed `returned_to_baseline` from 1 to 0 and failed while the oracle passed.
Restoring the free returned both targets to exact output. Temporary allocation totals during
cooking differ between the binaries, so the fixture checks only outstanding ownership after
release. Individual allocation-failure branches remain open.
