# TriangleMesh::loadFromDesc reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build-mut/Release/NxPhysics.dll`
Candidate SHA-256: `afb0a41730f880bcfaea29becbb752eb892f36b02d4b6875c3eee5b3916279ce`

IDA decompiled the oracle function at RVA `0x00055890` (preferred VA `0x10055890`),
the `TriangleMesh::loadFromDesc` vtable slot. The control flow validates the descriptor,
computes a hull when requested, packs/copies vertex and index data, builds the mesh model,
loads the optional PMap, builds convex data, frees temporary hull arrays, and returns the
mesh's final validity. The implementation is in `Physics/src/TriangleMesh.cpp`.

The clean staged-pair run of `NxPhysicsTriangleMeshApiTests` and
`NxPhysicsConvexMeshTests` completed with both processes exiting zero,
`stdout_delta=0`, and exact stderr. The exercised paths include padded points and 16-bit
indices, flipped winding, padded material indices, implicit topology, precomputed and
computed convex meshes, repeated creation, mesh array and save-to-descriptor output, PMap
load/export, and static/dynamic triangle-mesh actors with mass properties.

Mutation check: temporarily made `loadFromDesc` reject every four-vertex descriptor. The
candidate then failed at `authored PMap fixture mesh creation failed` while the oracle
continued successfully. Removing the mutation and rebuilding restored the exact two-target
differential. This confirms the public construction fixtures reach the row. The row is
recorded as reconstructed; less common malformed/allocation-failure paths and dependent
TriangleMesh rows are still open.
