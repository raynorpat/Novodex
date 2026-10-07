# TriangleMesh constructor reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `667687693171587740fd5f1b93d780f43e6bbcd1cbdab52932a108970e630995`

IDA decompiled RVA `0x00055490` (preferred VA `0x10055490`) as the TriangleMesh
constructor. It installs the TriangleMesh and base vtables, initializes the internal mesh,
sets the bounds sentinels and descriptor defaults, clears owned pointers, and allocates and
constructs the 8-byte public mesh wrapper through the Foundation SDK allocator. The candidate
implementation is `TriangleMesh::TriangleMesh` in `Physics/src/TriangleMesh.cpp`, with the
stable row comment at the constructor.

The clean staged-pair differential passed `NxPhysicsTriangleMeshApiTests` and
`NxPhysicsConvexMeshTests`: both sides exited zero, `stdout_delta=0`, and stderr matched
exactly. Mutation check: temporarily cleared `mPublicObject` after allocating the wrapper.
The candidate then failed on the first authored triangle-mesh creation while the oracle
passed. Restoring the assignment returned both targets to exact output.

This records the constructor as reconstructed. Allocation-failure behavior and the mesh
destructor's full cleanup remain part of the open lifecycle work.
