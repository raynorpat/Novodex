/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ObjectModel.h"

#include <string.h>

// phys_fn_002404 (0x0005ba70) is the shared member constructor; the oracle's
// collision-object ctor calls it at 0x000247d7 and then overwrites the vptr
// with the container's final table. The transcription constructs the member
// directly and lets C++ install this container's vptr for +0x00, which lands
// in the same post-construction state without replaying the intermediate
// base-vtable stores.
CollisionObject::CollisionObject(void* argument)
	{
	mWord04 = 0;							// 0x000247cb
	mArgument08 = argument;					// 0x000247e9
	mArgument18 = argument;					// 0x000247e6
	}

// phys_fn_001281 (0x000257a0): mov eax,[ecx+4]; ret. The whole row -- note
// the offset is FOUR bytes past the shape's vptr.
const void* nxShapeOwner(const void* shape)
	{
	return *reinterpret_cast<const void* const*>(
		static_cast<const unsigned char*>(const_cast<void*>(shape)) + 4);
	}

// ---------------------------------------------------------------------------
// The box hull facade. The static tables' CONTENTS are transcribed from the
// pinned image (.rdata 0x10122180/0x101221e0/0x10122240); this binary's
// copies live at different addresses, which is fine -- consumers get them
// through these getters, and the layout gate compares content, not pointers.

static const NxU32 gEdgeTable[12] =
	{ 0, 1, 1, 2, 2, 3, 3, 0, 7, 6, 6, 5 };			// 0x10122180..
static const NxU32 gFaceCornerTable[12] =
	{ 131073, 0, 131073, 2, 131073, 4, 131073, 6, 131073, 8, 131073, 10 };	// 0x101221e0..
static const NxU32 gAdjacencyTable[12] =
	{ 0, 5, 0, 1, 0, 4, 0, 3, 2, 4, 1, 2 };			// 0x10122240..

const NxU32* BoxHullFacade::vertices() const
	{
	return mVertices;							// lea eax,[ecx+0x10]
	}

const BoxFaceRecord* BoxHullFacade::face(unsigned index) const
	{
	// lea eax,[eax+eax*8]; lea eax,[ecx+eax*4+0x70] -- index*36 + (this+0x70).
	return &mFaces[index];
	}

const NxU32* BoxHullFacade::edgeTable()
	{
	return gEdgeTable;
	}

const NxU32* BoxHullFacade::faceCornerTable()
	{
	return gFaceCornerTable;
	}

const NxU32* BoxHullFacade::adjacencyTable()
	{
	return gAdjacencyTable;
	}
