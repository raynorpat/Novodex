/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// IceMeshTools.cpp, sub-unit D of units/convex-mesh-gap-contract.md (a file
// name chosen by the contract; the rows carry no string and have no .rdata
// block). Convex-mesh gap Task 2c writes the valencies from the Capstone
// listing, 0x00032590..0x0003280f: ICE's Valencies shape, not vendored, built
// on EdgeList (Physics/src/EdgeList.cpp). Every allocation goes through the
// 004803 getter (nxIceAlloc, EdgeList.h), persistent, without a cookie.
//
// x87: on the /arch:IA32 list, for the float rows sub-unit D will add; the
// valencies themselves are integer code.

#include "IceMeshTools.h"

#include <string.h>

// phys_fn_001663 (0x00032590, 19 B)
Valencies::Valencies()
	{
	mNbVerts = 0;
	mNbAdjVerts = 0;
	mValencies = 0;
	mOffsets = 0;
	mAdjVerts = 0;
	}

// phys_fn_001665 (0x000325b0, 95 B)
// Releases +0x08, +0x0c and +0x10 in that order, each only when non-null and
// each cleared afterwards.
Valencies::~Valencies()
	{
	if(mValencies)
		{
		nxIceFree(mValencies);
		mValencies = 0;
		}
	if(mOffsets)
		{
		nxIceFree(mOffsets);
		mOffsets = 0;
		}
	if(mAdjVerts)
		{
		nxIceFree(mAdjVerts);
		mAdjVerts = 0;
		}
	}

// phys_fn_001667 (0x00032610, 512 B)
// Valencies::Compute. The valences are counted over the edges of an EdgeList
// built with FacesToEdges only (no vertices; epsilon 0.001f, 0x3a83126f, which
// nothing reads); with AdjacentList the offsets are their prefix sums, the
// adjacent vertices are written through the offsets (advancing them), and the
// offsets are formed again. Any failure after the EdgeList exists releases it
// (0x0003275f); a failed valence array returns before it is built. The vertex
// count is trusted: the references must be below it.
bool Valencies::Compute(const VALENCESCREATE& create)
	{
	mNbVerts = create.NbVerts;
	mValencies = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
	if(!mValencies)
		return false;
	memset(mValencies, 0, mNbVerts * 4);

	EdgeList EL;
	EDGELISTCREATE ELC;
	ELC.NbFaces = create.NbFaces;
	ELC.DFaces = create.DFaces;
	ELC.WFaces = create.WFaces;
	ELC.FacesToEdges = true;
	ELC.VerticesToEdges = false;
	ELC.Verts = 0;
	ELC.Epsilon = 0.001f;
	if(!EL.Init(ELC))
		return false;

	// 0x000326a4..0x000326da.
	for(NxU32 i = 0; i < EL.mNbEdges; i++)
		{
		mValencies[EL.mEdges[i].Ref0]++;
		mValencies[EL.mEdges[i].Ref1]++;
		}

	if(create.AdjacentList)
		{
		mOffsets = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
		if(!mOffsets)
			return false;

		// 0x00032701..0x0003272c.
		mOffsets[0] = 0;
		for(NxU32 i = 1; i < mNbVerts; i++)
			mOffsets[i] = mValencies[i - 1] + mOffsets[i - 1];

		// 0x0003272e..0x0003275d: the last vertex's run end.
		mNbAdjVerts = mValencies[mNbVerts - 1] + mOffsets[mNbVerts - 1];
		mAdjVerts = (NxU32*) nxIceAlloc(mNbAdjVerts * 4, NX_MEMORY_PERSISTENT);
		if(!mAdjVerts)
			return false;

		// 0x00032773..0x000327c4.
		for(NxU32 i = 0; i < EL.mNbEdges; i++)
			{
			const NxU32 Ref0 = EL.mEdges[i].Ref0;
			const NxU32 Ref1 = EL.mEdges[i].Ref1;
			mAdjVerts[mOffsets[Ref0]++] = Ref1;
			mAdjVerts[mOffsets[Ref1]++] = Ref0;
			}

		// 0x000327c6..0x000327fa.
		mOffsets[0] = 0;
		for(NxU32 i = 1; i < mNbVerts; i++)
			mOffsets[i] = mValencies[i - 1] + mOffsets[i - 1];
		}
	return true;
	}
