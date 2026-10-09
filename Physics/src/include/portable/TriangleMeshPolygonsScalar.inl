// Ordinary polygon callbacks on the genuine constructed mesh and convex owner.
#include "TriangleMesh.h"
#include "TriangleMeshConvexData.h"
#include <string.h>

using NxTriangleMeshPrivate::TriangleMeshConvexData;

static TriangleMeshConvexData *nxMeshConvexOwner(const TriangleMesh *mesh)
{
    return static_cast<TriangleMeshConvexData *>(mesh->mConvexMesh);
}
NxU32 nxScratchStamp(NxPolygonScratch *scratch)
{
    if (++scratch->mStamp == 0)
    {
        if (scratch->mVisited)
            memset(scratch->mVisited, 0, (scratch->mVisitedCount & 0x3fffffffu) * sizeof(NxU32));
        scratch->mStamp = scratch->mVisitedCount;
    }
    return scratch->mStamp;
}
const IceMaths::Point *nxMeshHullCentre(const TriangleMesh *mesh)
{
    return &nxMeshConvexOwner(mesh)->mCentroid;
}
NxU32 nxMeshHullVertexCount(const TriangleMesh *mesh)
{
    return nxMeshConvexOwner(mesh)->mNbVerts;
}
const IceMaths::Point *nxMeshHullVertices(const TriangleMesh *mesh)
{
    return nxMeshConvexOwner(mesh)->mVerts;
}
NxU32 nxMeshHullPolygonCount(const TriangleMesh *mesh)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mNbPolygons)
        nxHullComputePolygons(hull);
    return hull->mNbPolygons;
}
const HullPolygon *nxMeshHullPolygon(const TriangleMesh *mesh, NxU32 index)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mPolygons)
        nxHullComputePolygons(hull);
    return hull->mPolygons + index;
}
IceCore::Container *nxMeshHullEdgeAxes(const TriangleMesh *mesh)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mEdgeAxes)
        nxHullComputeEdgeAxes(hull);
    return hull->mEdgeAxes;
}
const HullEdge *nxMeshHullEdges(const TriangleMesh *mesh)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mEdges)
        nxHullComputeEdges(hull);
    return hull->mEdges;
}
const EdgeDesc *nxMeshHullEdgeToPolygons(const TriangleMesh *mesh)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mEdgeToPolygons)
        nxHullComputeEdges(hull);
    return hull->mEdgeToPolygons;
}
const NxU32 *nxMeshHullEdgePolygons(const TriangleMesh *mesh)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    if (!hull->mEdgePolygons)
        nxHullComputeEdges(hull);
    return hull->mEdgePolygons;
}
NxU32 nxMeshHullSupportPolygon(const TriangleMesh *mesh, const IceMaths::Point *direction, const float *pose)
{
    return nxHullSupportPolygon(nxMeshConvexOwner(mesh), direction, pose);
}
NxU32 nxMeshHullSupportFace(const TriangleMesh *mesh, const IceMaths::Point *direction, const float *pose,
                            NxU32 *kind)
{
    return nxHullSupportFace(nxMeshConvexOwner(mesh), direction, pose, kind);
}
void nxMeshHullProject(const TriangleMesh *mesh, NxPolygonScratch *scratch, float *least, float *greatest,
                       const IceMaths::Point *direction, const float *pose, const IceSupportMap *map)
{
    TriangleMeshConvexData *hull = nxMeshConvexOwner(mesh);
    IceMaths::Point local;
    for (unsigned row = 0; row < 3; ++row)
        local[row] =
            float((double(pose[row * 4 + 1]) * direction->y + double(pose[row * 4 + 2]) * direction->z) +
                  double(pose[row * 4]) * direction->x);
    NxU32 first = 0, second = 0;
    if (map)
    {
        const NxU32 sample = nxSupportMapLookup(map, &local);
        first = map->mSamples[sample];
        second = map->mSamples2[sample];
    }
    else
    {
        // The cdecl climb borrows visited before stamp advances; wrap changes
        // its elements, never its pointer. A failed climb leaves vertex zero.
        NxU32 *visited = scratch->mVisited;
        const NxU32 firstStamp = nxScratchStamp(scratch);
        nxHullClimbSupportVertex(&first, &local, hull->mVerts, hull->mVertexGraph, firstStamp, visited);
        const IceMaths::Point opposite(-local.x, -local.y, -local.z);
        visited = scratch->mVisited;
        const NxU32 secondStamp = nxScratchStamp(scratch);
        nxHullClimbSupportVertex(&second, &opposite, hull->mVerts, hull->mVertexGraph, secondStamp, visited);
    }
    const double translation =
        (double(pose[12]) * direction->x + double(pose[14]) * direction->z) + double(pose[13]) * direction->y;
    const IceMaths::Point &a = hull->mVerts[first];
    const IceMaths::Point &b = hull->mVerts[second];
    *least = float(((double(local.z) * a.z + double(local.y) * a.y) + double(local.x) * a.x) + translation);
    const double upper =
        ((double(local.z) * b.z + double(local.y) * b.y) + double(local.x) * b.x) + translation;
    *greatest = float(upper);
    // Original compares the unrounded second projection with stored first.
    // Equality and unordered comparisons keep the original output order.
    if (upper < *least)
    {
        const float lower = *least;
        *least = float(upper);
        *greatest = lower;
    }
}
