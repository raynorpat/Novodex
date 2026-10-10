#ifndef NX_PHYSICS_ICESUPPORTMAPS_H
#define NX_PHYSICS_ICESUPPORTMAPS_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The support-vertex maps, sub-unit B of units/convex-mesh-gap-contract.md
// (Physics/src/IceSupportMaps.cpp, a file name chosen by the contract; convex-
// mesh gap Task 2f). A map samples 6 * n * n directions on the faces of a cube
// and keeps one byte per direction; the lookup (001556) turns a direction back
// into its sample. Three kinds fill the bytes: A (table 0x1010785c) with the
// hull polygon furthest along the direction (001496), B (0x1010786c) with the
// polygon the ray from the hull's centre along the direction leaves through,
// and C (0x10107890) with the least and greatest vertex along it, in two maps.
//
// The objects are C++ classes in the oracle, a base (0x10107848) with three
// derived tables of four slots: 0 the scalar deleting destructor, 1 allocate,
// 2 compute(sample, direction), 3 a no-op. The float rows are the listing's
// instructions, naked, and a naked function cannot be a member or a virtual
// slot of a C++ class; so the tables are written as the arrays of function
// pointers the image holds, and every slot is __fastcall with the object in ecx
// and an unused edx, which is thiscall's convention for the callee (the stack
// arguments popped by it).

#include "ConvexHull.h"

// The object: 0x14 bytes for A and B, 0x18 for C.
struct IceSupportMap
	{
	const void* const*	mVtable;		// +0x00
	NxU32				mSubdiv;		// +0x04, n (001558 at 0x0002e30b)
	NxU32				mNbSamples;		// +0x08, 6 * n * n (0x0002e30e)
	NxU8*				mSamples;		// +0x0c, A and B: the polygon per sample; C: the least vertex
	union
		{
		ConvexHull*		mHull;			// +0x10, A and B (001565 / 001571)
		NxU8*			mSamples2;		// +0x10, C: the greatest vertex
		};
	const ConvexHull*	mVertexSource;	// +0x14, C only (001575): +0x0c count, +0x10 vertices
	};

// The four tables (0x10107848, 0x1010785c, 0x1010786c, 0x10107890).
extern const void* const gIceSupportMapBaseTable[4];
extern const void* const gIceSupportMapHullTable[4];
extern const void* const gIceSupportMapPlaneTable[4];
extern const void* const gIceSupportMapVertexTable[4];

// 001550: the cube face of a direction (dominant |component| by the bits, ties
// to the lower axis) as axis*2 + sign bit, and the two other components divided
// by the dominant one's magnitude (cdecl).
#if NX_PHYSICS_USE_X87
NxU32 nxSupportMapCubeFace(const IceMaths::Point* dir, float* u, float* v);

// 001552 / 001554: the base constructor and the base table store.
void* __fastcall nxSupportMapBaseConstruct(IceSupportMap* map);
void __fastcall nxSupportMapBaseTable(IceSupportMap* map);

// 001556: the sample index of a direction (thiscall, `ret 4`).
NxU32 __fastcall nxSupportMapLookup(const IceSupportMap* map, NxU32 edx, const IceMaths::Point* dir);

// 001558 (with 001560): n, the sample count, slot 1, then slot 2 per sample,
// then slot 3 (thiscall, `ret 4`).
bool __fastcall nxSupportMapInit(IceSupportMap* map, NxU32 edx, NxU32 subdiv);

// The constructors 001565 (A), 001571 (B) and 001575 (C) (thiscall, `ret 4`).
IceSupportMap* __fastcall nxSupportMapHullConstruct(IceSupportMap* map, NxU32 edx, ConvexHull* hull);
IceSupportMap* __fastcall nxSupportMapPlaneConstruct(IceSupportMap* map, NxU32 edx, ConvexHull* hull);
IceSupportMap* __fastcall nxSupportMapVertexConstruct(IceSupportMap* map, NxU32 edx, const ConvexHull* source);

// The slots.
IceSupportMap* __fastcall nxSupportMapBaseDelete(IceSupportMap* map, NxU32 edx, NxU32 flags);		// 001563
bool __fastcall nxSupportMapHullAllocate(IceSupportMap* map);										// 001567 (A, B)
bool __fastcall nxSupportMapHullCompute(IceSupportMap* map, NxU32 edx, NxU32 sample,
	const IceMaths::Point* dir);																	// 001569 (A)
bool __fastcall nxSupportMapPlaneCompute(IceSupportMap* map, NxU32 edx, NxU32 sample,
	const IceMaths::Point* dir);																	// 001573 (B)
void __fastcall nxSupportMapVertexRelease(IceSupportMap* map);										// 001577 (C's body)
bool __fastcall nxSupportMapVertexAllocate(IceSupportMap* map);									// 001579 (C)
bool __fastcall nxSupportMapVertexCompute(IceSupportMap* map, NxU32 edx, NxU32 sample,
	const IceMaths::Point* dir);																	// 001581 (C)
void __fastcall nxSupportMapNoop(IceSupportMap* map);												// 001583
IceSupportMap* __fastcall nxSupportMapHullDelete(IceSupportMap* map, NxU32 edx, NxU32 flags);		// 001585 (A)
IceSupportMap* __fastcall nxSupportMapPlaneDelete(IceSupportMap* map, NxU32 edx, NxU32 flags);	// 001587 (B)
IceSupportMap* __fastcall nxSupportMapVertexDelete(IceSupportMap* map, NxU32 edx, NxU32 flags);	// 001589 (C)
#else
#include "portable/NxConvexInterfaces.h"
#endif

#endif
