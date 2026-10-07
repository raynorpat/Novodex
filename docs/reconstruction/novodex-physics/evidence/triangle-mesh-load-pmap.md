# TriangleMesh::loadPMap reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `dc3f9ee0833351d362a64756a0a097939c0127496cbc90259d689c6147cc8afa`

IDA decompiled RVA `0x00053cf0` (preferred VA `0x10053cf0`) as
`TriangleMesh::loadPMap`. The function rejects missing or empty input, destroys and clears
the previous map, constructs and parses a replacement from the serialized bytes, and
cleans up and reports an error if parsing fails. The candidate implementation is in
`Physics/src/TriangleMesh.cpp`, with the stable row comment at the method.

The clean staged-pair differential passed `NxPhysicsTriangleMeshApiTests` and
`NxPhysicsConvexMeshTests`: both processes exited zero, `stdout_delta=0`, and stderr matched
exactly. The tests cover descriptor-supplied map data, valid load and export, a non-empty
cell-run encoding, and invalid-magic replacement of an existing map.

Mutation check: temporarily skipped destruction of the previous map. The invalid-magic
reload then left `hasPMap=1` and failed the candidate lifecycle fixture while the oracle
passed. Restoring the cleanup returned both targets to exact output. Allocation-failure and
all malformed-encoding boundaries remain open.
