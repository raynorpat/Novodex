#ifndef NX_PHYSICS_ICEMESHTOOLS_H
#define NX_PHYSICS_ICEMESHTOOLS_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Sub-unit D of units/convex-mesh-gap-contract.md, the mesh utilities of
// Physics/src/IceMeshTools.cpp (a file name chosen by the contract: the rows
// carry no string). The valencies (001663, 001665, 001667) are written by
// convex-mesh gap Task 2c; the vertex reduction (001645, 001647, 001659) by
// Task 2d, for MeshBuilder2.

#include "EdgeList.h"

// The create block 001667 reads (0x00032610): +0x00 vertex count, +0x04 face
// count, +0x08 32-bit faces, +0x0c 16-bit faces, +0x10 a flag byte (build the
// adjacent-vertex lists).
struct VALENCESCREATE
	{
	NxU32			NbVerts;
	NxU32			NbFaces;
	const NxU32*	DFaces;
	const NxU16*	WFaces;
	bool			AdjacentList;
	};

// Per-vertex valence (0x14 bytes; 001663 zeroes all five words).
class Valencies
	{
	public:
				Valencies();
				~Valencies();

	bool		Compute(const VALENCESCREATE& create);

	NxU32		mNbVerts;		// +0x00
	NxU32		mNbAdjVerts;	// +0x04
	NxU32*		mValencies;		// +0x08
	NxU32*		mOffsets;		// +0x0c
	NxU32*		mAdjVerts;		// +0x10
	};

// What 001647 hands back (0x0003185e..0x00031876): +0x00 the reduced vertices,
// +0x04 their count, +0x08 the cross-reference (old vertex -> reduced vertex).
struct REDUCEDCLOUD
	{
	IceMaths::Point*	RVerts;
	NxU32				NbRVerts;
	NxU32*				XRef;
	};

// The vertex reduction (0x14 bytes; 001645 sets +0x00/+0x04 and zeroes the
// rest). The three owned blocks come from the 004803 getter (type 0) and are
// released by 001659 (+0x10, then +0x0c).
class ReducedVertices
	{
	public:
				ReducedVertices(const IceMaths::Point* verts, NxU32 nb_verts);
				~ReducedVertices();

	bool		Reduce(REDUCEDCLOUD* rc);

	NxU32					mNbVerts;		// +0x00
	const IceMaths::Point*	mVerts;			// +0x04
	NxU32					mNbRVerts;		// +0x08
	IceMaths::Point*		mRVerts;		// +0x0c
	NxU32*					mXRef;			// +0x10
	};

#endif
