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
// valencies and the vertex reduction are integer code.
//
// Task 2d adds the vertex reduction (0x00031680..0x0003188c and 0x000324a0),
// which MeshBuilder2 (IceMeshBuilder2.cpp) calls: ICE's reduced-vertex shape,
// vertices radix-sorted on the bits of x, then y, then z (RadixSort 005157,
// 005163 with the unsigned hint, 005159), kept once per run of equal bits.

#include "IceMeshTools.h"

#include <string.h>

// phys_fn_001645 (0x00031680, 29 B)
// +0x04 = verts, +0x00 = nb_verts (the arguments in that order, `ret 8`), and
// +0x08, +0x0c, +0x10 zeroed.
ReducedVertices::ReducedVertices(const IceMaths::Point* verts, NxU32 nb_verts)
	{
	mNbRVerts = 0;
	mRVerts = 0;
	mXRef = 0;
	mVerts = verts;
	mNbVerts = nb_verts;
	}

// phys_fn_001659 (0x000324a0, 65 B)
// Releases +0x10, then +0x0c, each through the 004803 getter's slot 3 when
// non-null, each cleared.
ReducedVertices::~ReducedVertices()
	{
	if(mXRef)
		{
		nxIceFree(mXRef);
		mXRef = 0;
		}
	if(mRVerts)
		{
		nxIceFree(mRVerts);
		mRVerts = 0;
		}
	}

// phys_fn_001647 (0x000316a0, 492 B)
// ReducedVertices::Reduce. The two previous outputs are released first, as
// the destructor releases them (0x000316a7..0x000316d8). The cross-reference is
// persistent (type 0) and checked; the key buffer temporary (type 1), checked,
// and released after the third sort (0x000317b9); the reduced array
// persistent and NOT checked (0x000317dc..0x000317f1). A vertex is kept when
// any of its three words differs from the previous sorted vertex's -- the
// first is compared with three 0xffffffff words on the stack (0x000317c6), so
// a first vertex of those bits is not kept and its cross-reference is
// 0xffffffff. The comparison is on bits (`cmp` of dwords), not on values.
bool ReducedVertices::Reduce(REDUCEDCLOUD* rc)
	{
	if(mXRef)
		{
		nxIceFree(mXRef);
		mXRef = 0;
		}
	if(mRVerts)
		{
		nxIceFree(mRVerts);
		mRVerts = 0;
		}

	mXRef = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
	if(!mXRef)
		return false;

	NxU32* Keys = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_TEMP);
	if(!Keys)
		return false;

	const NxU32* Words = (const NxU32*) mVerts;
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 0];
	IceCore::RadixSort Radix;
	Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED);
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 1];
	Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED);
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 2];
	const NxU32* Sorted = Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED).GetRanks();
	nxIceFree(Keys);

	mNbRVerts = 0;
	const NxU32 Junk[3] = { 0xffffffff, 0xffffffff, 0xffffffff };
	const NxU32* Previous = Junk;
	mRVerts = (IceMaths::Point*) nxIceAlloc(mNbVerts * 12, NX_MEMORY_PERSISTENT);
	NxU32* Reduced = (NxU32*) mRVerts;
	for(NxU32 NbToGo = mNbVerts; NbToGo; NbToGo--)
		{
		const NxU32 Index = *Sorted++;
		const NxU32* Current = Words + Index * 3;
		if(Current[0] != Previous[0] || Current[1] != Previous[1] || Current[2] != Previous[2])
			{
			Reduced[mNbRVerts * 3 + 0] = Current[0];
			Reduced[mNbRVerts * 3 + 1] = Current[1];
			Reduced[mNbRVerts * 3 + 2] = Current[2];
			mNbRVerts++;
			}
		Previous = Current;
		mXRef[Index] = mNbRVerts - 1;
		}

	if(rc)
		{
		rc->XRef = mXRef;
		rc->NbRVerts = mNbRVerts;
		rc->RVerts = mRVerts;
		}
	return true;
	}


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
