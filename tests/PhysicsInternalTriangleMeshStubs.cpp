// NxPhysicsInternalTests links SDK construction/destruction code but its
// static-proof cases never create or use triangle meshes. Production and
// differential targets bind the real implementations from TriangleMesh.cpp.
#include "TriangleMesh.h"

TriangleMesh::TriangleMesh() {}
TriangleMesh::~TriangleMesh() {}
bool TriangleMesh::loadFromDesc(const NxTriangleMeshDesc&) { return false; }
NxTriangleMesh* TriangleMesh::publicHandle() const { return 0; }
