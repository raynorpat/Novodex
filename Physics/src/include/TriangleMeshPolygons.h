#ifndef NX_PHYSICS_TRIANGLEMESHPOLYGONS_H
#define NX_PHYSICS_TRIANGLEMESHPOLYGONS_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The TriangleMesh polygon interface (the table at +0x04, 0x101085d4) and its
// scratch-stamp helper, prerequisite P-Mesh of units/convex-mesh-gap-contract.md
// (Physics/src/TriangleMeshPolygons.cpp, convex-mesh gap Task 2g). Every slot is
// thiscall on the mesh plus four in the oracle and __fastcall with an unused
// edx here (see the .cpp).

#include "ConvexHull.h"
#include "IceSupportMaps.h"

#if NX_PHYSICS_USE_X87
NxU32 __fastcall nxScratchStamp(void* scratch);																	// 000505
const IceMaths::Point* __fastcall nxMeshHullCentre(const void* iface);											// 002211, slot 0
NxU32 __fastcall nxMeshHullVertexCount(const void* iface);														// 002213, slot 1
const IceMaths::Point* __fastcall nxMeshHullVertices(const void* iface);										// 002215, slot 2
NxU32 __fastcall nxMeshHullPolygonCount(const void* iface);														// 002221, slot 3
const HullPolygon* __fastcall nxMeshHullPolygon(const void* iface, NxU32 edx, NxU32 index);						// 002223, slot 4
IceCore::Container* __fastcall nxMeshHullEdgeAxes(const void* iface);											// 002225, slot 5
const HullEdge* __fastcall nxMeshHullEdges(const void* iface);													// 002227, slot 6
const EdgeDesc* __fastcall nxMeshHullEdgeToPolygons(const void* iface);											// 002229, slot 7
const NxU32* __fastcall nxMeshHullEdgePolygons(const void* iface);												// 002231, slot 8
NxU32 __fastcall nxMeshHullSupportPolygon(const void* iface, NxU32 edx, const IceMaths::Point* dir,
	const float* pose);																							// 002217, slot 9
NxU32 __fastcall nxMeshHullSupportFace(const void* iface, NxU32 edx, const IceMaths::Point* dir,
	const float* pose, NxU32* kind);																			// 002219, slot 10
void __fastcall nxMeshHullProject(const void* iface, NxU32 edx, void* scratch, float* least, float* greatest,
	const IceMaths::Point* dir, const float* pose, const IceSupportMap* map);									// 002249, slot 11

#else
#include "portable/NxConvexInterfaces.h"
#endif

// The table at 0x101085d4: the twelve slots above in the image's order.
extern const void* const gTriangleMeshPolygonTable[12];

#endif
