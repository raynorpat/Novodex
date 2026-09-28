/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The two on-demand topology builders of TriangleMesh, prerequisite P-Small of
// units/convex-mesh-gap-contract.md, written by convex-mesh gap Task 2e from the
// Capstone listing. In the oracle they sit in TriangleMesh's own span
// (0x000543d0 and 0x00054460, between the writer 0x000539d0 and the reader
// 0x00055cb0); they are written in a file of their own so that the asset
// harness, which links TriangleMesh.cpp, does not have to link the ICE-shaped
// rows they build (IceAdjacencies.cpp, EdgeList.cpp).
//
// Both are thiscall on the TriangleMesh with no argument and read its triangle
// count (+0x0c), vertices (+0x10) and 32-bit triangles (+0x14). Each allocates
// its object through the 004803 getter's slot 0 (type 0), constructs it only
// when the block is non-null, stores the pointer, and then calls Init on it
// WITHOUT testing it (a failed allocation calls Init on null, as the listing
// does). When Init fails the object is destroyed and released through slot 3
// and the pointer cleared. The listing returns nothing the callers read: on
// success eax holds Init's result, after a release the allocator's.
//
// Integer code; built /EHs-c- with the other ICE-shaped files (the listings
// are frameless).

#include "TriangleMesh.h"
#include "IceAdjacencies.h"

#include <new>

// phys_fn_002186 (0x000543d0, 143 B)
// The adjacencies: create block {triangle count, triangles, no 16-bit faces, no
// vertices (so no active-edge pass), epsilon 0.001f (0x3a83126f)}; an 8-byte
// Adjacencies (constructor 001536), Init 001546; on failure ~Adjacencies
// (001544) and the release.
__declspec(noinline) void TriangleMesh::createAdjacencies()
	{
	ADJACENCIESCREATE create;
	create.NbFaces = mInternal.mTriangleCount;
	create.DFaces = (const NxU32*) mInternal.mTriangles;
	create.WFaces = 0;
	create.Verts = 0;
	create.Epsilon = 0.001f;

	void* memory = nxIceAlloc(sizeof(Adjacencies), NX_MEMORY_PERSISTENT);
	mAdjacencies = memory ? new(memory) Adjacencies : 0;
	if(!mAdjacencies->Init(create))
		{
		if(mAdjacencies)
			{
			mAdjacencies->~Adjacencies();
			nxIceFree(mAdjacencies);
			mAdjacencies = 0;
			}
		}
	}

// phys_fn_002188 (0x00054460, 152 B)
// The edge list: create block {triangle count, triangles, no 16-bit faces,
// faces-to-edges and vertices-to-edges both set, the vertices, epsilon 0.001f};
// a 0x18-byte EdgeList (constructor 002052), Init 002063; on failure ~EdgeList
// (002060) and the release. The two bytes after the flags are not written.
__declspec(noinline) void TriangleMesh::createEdgeList()
	{
	EDGELISTCREATE create;
	create.NbFaces = mInternal.mTriangleCount;
	create.DFaces = (const NxU32*) mInternal.mTriangles;
	create.WFaces = 0;
	create.FacesToEdges = true;
	create.VerticesToEdges = true;
	create.Verts = (const IceMaths::Point*) mInternal.mVertices;
	create.Epsilon = 0.001f;

	void* memory = nxIceAlloc(sizeof(EdgeList), NX_MEMORY_PERSISTENT);
	mEdgeList = memory ? new(memory) EdgeList : 0;
	if(!mEdgeList->Init(create))
		{
		if(mEdgeList)
			{
			mEdgeList->~EdgeList();
			nxIceFree(mEdgeList);
			mEdgeList = 0;
			}
		}
	}
