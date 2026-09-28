#ifndef NX_PHYSICS_CONVEXHULL_H
#define NX_PHYSICS_CONVEXHULL_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The convex hull the ConvexHull.cpp rows work on (prerequisite P-Hull of
// units/convex-mesh-gap-contract.md). Convex-mesh gap Task 2e wrote 001461 (the
// vertex normals, P-Small) and the fields +0x04..+0x14 it reads; Task 2f writes
// the rest of P-Hull (001441..001502) and the fields those rows establish: the
// centroid, the polygons and the edges. Only fields a listing site reads or
// writes are named; +0x00 and +0x34 are read by none of these rows.

#include "IceAdjacencies.h"
#include "IceMeshTools.h"

// One hull polygon, 0x24 bytes: `new[]` of these with a count cookie through
// the 004803 getter (type 0, 001472 at 0x0002b76b..0x0002b794), each built by
// 000925 (0x00020440), which zeroes +0x00..+0x08.
struct HullPolygon
	{
	NxU32				mNbVerts;		// +0x00, 0x0002b80c
	const NxU32*		mVRefs;			// +0x04, into the hull's +0x2c (0x0002b803)
	const NxU32*		mERefs;			// +0x08, into the hull's +0x30 (001502, 0x0002cf91)
	IceMaths::Plane		mPlane;			// +0x0c..+0x18, set by 001463 (Plane::Set)
	float				mMin;			// +0x1c, least projection of a hull vertex (0x0002b90c)
	float				mMax;			// +0x20, greatest (0x0002b917)
	};

// One hull edge, 8 bytes: `new[]` with a count cookie, built by the identity
// row 001391 (0x00027f00), which writes nothing (001502 at 0x0002cd39..0x0002cd68).
struct HullEdge
	{
	NxU32				mRef0;			// +0x00, the smaller vertex reference
	NxU32				mRef1;			// +0x04
	};

class ConvexHull
	{
	public:
	bool				ComputeVertexNormals();

	NxU32				mWord00;			// +0x00, read by none of these rows (a table: TriangleMesh.h,
										// 002164 releases the hull at mesh +0xa0 through its slot 0)
	NxU32				mNbFaces;			// +0x04, 0x0002aedc
	const NxU16*		mFaces;				// +0x08, 16-bit triangles, 0x0002aee3
	NxU32				mNbVerts;			// +0x0c, 0x0002ae84 / 0x0002aeb0
	const IceMaths::Point*	mVerts;			// +0x10, 0x0002aed5
	IceMaths::Point*	mVertexNormals;		// +0x14, 0x0002ae66..0x0002aea4
	IceMaths::Point		mCentroid;			// +0x18, 001459 through 001472 (0x0002b7be)
	NxU32				mNbPolygons;		// +0x24, 0x0002b6ff / 0x0002b768
	HullPolygon*		mPolygons;			// +0x28, `new[]` (cookie), 0x0002b79f
	NxU32*				mPolygonVRefs;		// +0x2c, every polygon's references, 0x0002b7db
	NxU32*				mPolygonERefs;		// +0x30, every polygon's edge numbers, 0x0002cf5e
	NxU32				mWord34;			// +0x34, read by none of these rows
	NxU32				mNbEdges;			// +0x38, 0x0002cd32 / 0x0002ce4f
	HullEdge*			mEdges;				// +0x3c, `new[]` (cookie), 0x0002ced1
	IceMaths::Point*	mEdgeNormals;		// +0x40, 0x0002d1f2
	EdgeDesc*			mEdgeToPolygons;	// +0x44, no cookie, built by 001439 (0x0002d035)
	NxU32*				mEdgePolygons;		// +0x48, 0x0002d0e3
	};

// The P-Hull rows other than 001461, in ConvexHull.cpp. The oracle's methods are
// thiscall; the float rows are the listing's instructions, naked, and a naked
// function cannot be a member, so those are __fastcall with the object in ecx
// and an unused edx (the stack arguments popped by the callee, as thiscall's
// are). Each comment gives the oracle's convention.

// 001441: the area of a 16-bit indexed triangle (thiscall on the triangle,
// `ret 4`); 0.0f when the vertices are null. Result in st(0).
float __fastcall nxHullTriangleArea(const NxU16* triangle, NxU32 edx, const IceMaths::Point* verts);

// 001445: the centre of a 16-bit indexed triangle (thiscall, `ret 8`); nothing
// written when the vertices are null.
void __fastcall nxHullTriangleCenter(const NxU16* triangle, NxU32 edx, const IceMaths::Point* verts,
	IceMaths::Point* center);

// 001449: the faces reachable from `face` across inactive edges, appended to
// `faces` and marked (cdecl, recursive).
void nxHullGatherFaces(IceCore::Container* faces, const AdjTriangle* adj, NxU32 face, NxU8* marks);

// 001459: the area-weighted centre of the faces (thiscall, `ret 4`).
bool __fastcall nxHullComputeCentroid(const ConvexHull* hull, NxU32 edx, IceMaths::Point* center);

// 001463: a polygon's plane, through the largest of its spread triangles (cdecl).
bool nxHullPolygonPlane(IceMaths::Plane* plane, NxU32 nbVerts, const NxU32* vrefs,
	const IceMaths::Point* verts);

// 001465: the polygons of a closed mesh of 16-bit faces: per polygon its vertex
// count and its references, appended to `data` (cdecl).
bool nxHullExtractPolygons(NxU32* nbPolygons, IceCore::Container* data, const ConvexHull* hull);

// 001472: the polygons, their planes and extents and the centroid (thiscall).
bool __fastcall nxHullComputePolygons(ConvexHull* hull);

// 001496: the polygon whose normal is furthest along a direction, the direction
// first rotated by the 4x4 pose when one is given (thiscall, `ret 8`).
NxU32 __fastcall nxHullSupportPolygon(ConvexHull* hull, NxU32 edx, const IceMaths::Point* dir,
	const float* pose);

// 001502: the edges, the polygons' edge numbers, the edge-to-polygon table and
// the edge normals (thiscall).
bool __fastcall nxHullComputeEdges(ConvexHull* hull);

// The element constructors the `new[]` sites run through 000001, as their
// product forms (thiscall, no argument).
void* __fastcall nxHullPolygonConstruct(void* polygon);		// 000925
void* __fastcall nxIceIdentityConstruct(void* object);		// 001391
void* __fastcall nxEdgeDescConstruct(void* desc);			// 001439

// 000001: the `vector constructor iterator` MSVC generates for `new[]` of a
// class with a constructor (stdcall: array, element size, count, constructor).
void __stdcall nxIceVectorConstruct(void* array, NxU32 size, NxU32 count, void* (__fastcall* ctor)(void*));

#endif
