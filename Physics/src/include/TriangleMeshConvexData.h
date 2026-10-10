#ifndef NX_TRIANGLE_MESH_CONVEX_DATA_H
#define NX_TRIANGLE_MESH_CONVEX_DATA_H
#include "ConvexHull.h"
#include <stddef.h>
// Mechanically shared private owner declaration from TriangleMesh.cpp.
namespace NxTriangleMeshPrivate
{
struct TriangleMeshConvexData : ConvexHull
{
    float mBounds[6];        // +0x4c, min xyz then max xyz (001411)
    Valencies *mVertexGraph; // +0x64 (001411, 002249)
    NxU32 mObjectWords[12];  // +0x68..+0x97 (001524)
};

static_assert(sizeof(ConvexHull) == 0x4c, "the convex hull base has the measured 0x4c layout");
static_assert(offsetof(TriangleMeshConvexData, mVertexGraph) == 0x64, "the convex vertex graph is at +0x64");
static_assert(sizeof(TriangleMeshConvexData) == 0x98, "the convex mesh allocation is 0x98 bytes");

} // namespace NxTriangleMeshPrivate
#endif
