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
// carry no string). Only the valencies (001663, 001665, 001667) are written so
// far (convex-mesh gap Task 2c).

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

#endif
