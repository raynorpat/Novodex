// The asset writer differential constructs TriangleMesh only to call save(),
// immediately zeroes its storage, and supplies all serialized fields itself.
// Its isolated target does not link the SDK lifecycle dependencies.
#include "TriangleMesh.h"

TriangleMesh::TriangleMesh() {}
TriangleMesh::~TriangleMesh() {}
