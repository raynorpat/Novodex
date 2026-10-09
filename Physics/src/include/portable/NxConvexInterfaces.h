#ifndef NX_CONVEX_INTERFACES_H
#define NX_CONVEX_INTERFACES_H
#include "NxSimpleTypes.h"

// Ordinary contracts recovered from ConvexHull.cpp's listing and the actual
// TriangleMeshPolygons producers/consumers. Types are the real private types.
// These declarations intentionally omit the unused legacy edx placeholders.
class ConvexHull;
class Valencies;
struct HullPolygon;
struct EdgeDesc;
struct AdjTriangle;
namespace IceCore { class Container; }
namespace IceMaths { class Point; class Plane; }

// 001441/001445: ecx triangle, stack vertices then output; borrowed arrays.
float nxHullTriangleArea(const NxU16* triangle, const IceMaths::Point* vertices);
void nxHullTriangleCenter(const NxU16* triangle, const IceMaths::Point* vertices, IceMaths::Point* center);
// 001459: ecx hull, stack output. No writes if vertex count/array is absent.
bool nxHullComputeCentroid(const ConvexHull* hull, IceMaths::Point* center);
// 001463: cdecl output plane, count, vertex refs, vertices. No implicit scratch.
bool nxHullPolygonPlane(IceMaths::Plane* plane, NxU32 count, const NxU32* refs, const IceMaths::Point* vertices);
// 001449/001465: all cdecl inputs; output append containers borrow nothing.
void nxHullGatherFaces(IceCore::Container* faces, const AdjTriangle* adjacency, NxU32 first, NxU8* marks);
bool nxHullExtractPolygons(NxU32* count, IceCore::Container* data, const ConvexHull* hull);
// 001472/001502..12/001514: ecx hull owns SDK output arrays/count cookies;
// lazy reads call these directly from slots3..8. Edge axes own a CRT Container.
bool nxHullComputePolygons(ConvexHull* hull);
bool nxHullComputeEdges(ConvexHull* hull);
bool nxHullComputeEdgeAxes(ConvexHull* hull);
// 001496/001516..22: ecx hull, stack axis, optional row-major 4x4 pose,
// optional output kind. Only the upper 3x3 pose is consumed; translation is
// ignored. Return polygon index; kind0 face/kind1 better face of winning edge.
NxU32 nxHullSupportPolygon(ConvexHull* hull, const IceMaths::Point* axis, const float* pose);
NxU32 nxHullSupportFace(ConvexHull* hull, const IceMaths::Point* axis, const float* pose, NxU32* kind);
// 001530/32/34: cdecl index in/out, axis, vertices, actual Valencies graph,
// stamp, visited. Caller owns visited[graph.mNbVerts]; previous stamps persist.
bool nxHullClimbSupportVertex(NxU32* index, const IceMaths::Point* axis,
    const IceMaths::Point* vertices, const Valencies* graph, NxU32 stamp, NxU32* visited);
// Exact constructor write sets; no raw receiver/register calling convention.
void* nxHullPolygonConstruct(void* polygon);
void* nxIceIdentityConstruct(void* object);
void* nxEdgeDescConstruct(void* desc);
typedef void* (*NxHullElementConstructor)(void*);
void nxIceVectorConstruct(void* array, NxU32 size, NxU32 count, NxHullElementConstructor constructor);
// 001661: ecx axes, stack borrowed axis. 001591: ecx container, stack three
// binary32 words copied as integer data. No MeshBuilder2 dependency backedge.
bool nxIceAddUniqueAxis(IceCore::Container* axes, const IceMaths::Point* axis);
IceCore::Container& nxIceContainerAddPoint(IceCore::Container* container, const NxU32* words);

// Support-map001550..001589. Actual table slots: deleting destructor,
// allocate, compute(sample, direction), finish. No implicit scratch survives
// a call; samples are owned SDK allocations, the hull/source is borrowed.
struct IceSupportMap;
typedef IceSupportMap* (*NxSupportMapDeleteSlot)(IceSupportMap*, NxU32 flags);
typedef bool (*NxSupportMapAllocateSlot)(IceSupportMap*);
typedef bool (*NxSupportMapComputeSlot)(IceSupportMap*, NxU32 sample, const IceMaths::Point* direction);
typedef void (*NxSupportMapFinishSlot)(IceSupportMap*);
NxU32 nxSupportMapCubeFace(const IceMaths::Point* direction, float* u, float* v);
void* nxSupportMapBaseConstruct(IceSupportMap* map);
void nxSupportMapBaseTable(IceSupportMap* map);
NxU32 nxSupportMapLookup(const IceSupportMap* map, const IceMaths::Point* direction);
bool nxSupportMapInit(IceSupportMap* map, NxU32 subdiv);
IceSupportMap* nxSupportMapHullConstruct(IceSupportMap* map, ConvexHull* hull);
IceSupportMap* nxSupportMapPlaneConstruct(IceSupportMap* map, ConvexHull* hull);
IceSupportMap* nxSupportMapVertexConstruct(IceSupportMap* map, const ConvexHull* source);
IceSupportMap* nxSupportMapBaseDelete(IceSupportMap* map, NxU32 flags);
bool nxSupportMapHullAllocate(IceSupportMap* map);
bool nxSupportMapHullCompute(IceSupportMap* map, NxU32 sample, const IceMaths::Point* direction);
bool nxSupportMapPlaneCompute(IceSupportMap* map, NxU32 sample, const IceMaths::Point* direction);
void nxSupportMapVertexRelease(IceSupportMap* map);
bool nxSupportMapVertexAllocate(IceSupportMap* map);
bool nxSupportMapVertexCompute(IceSupportMap* map, NxU32 sample, const IceMaths::Point* direction);
// Capstone's authoritative2ea70 is one bytec3/ret; init calls slot3 with
// receiver inecx and no stack arguments. It reads/writes no receiver state.
void nxSupportMapNoop(IceSupportMap* map);
IceSupportMap* nxSupportMapHullDelete(IceSupportMap* map, NxU32 flags);
IceSupportMap* nxSupportMapPlaneDelete(IceSupportMap* map, NxU32 flags);
IceSupportMap* nxSupportMapVertexDelete(IceSupportMap* map, NxU32 flags);

#endif
