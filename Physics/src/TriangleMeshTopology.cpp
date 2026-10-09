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
//
// convex-mesh gap Task 2i adds 002081 (0x00052240), the internal mesh's vertex
// normals on demand, which also sits in TriangleMesh's span; naked, integer
// code (it allocates through the Foundation allocator and calls 002146).

#if defined(NX_PHYSICS_MESH_NORMALS_KERNEL_ONLY)
#include "NxInternalTriangleMesh.h"
#include "NxUserAllocator.h"
#else
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
	// As the listing (the call at 0x00054427): Init is called on the pointer without a
	// null test, so a failed allocation calls it on null. In C++ that call is
	// undefined behaviour, which lets a compiler drop the null test below; it is
	// kept by the build of 14c8ec7 (`test ecx, ecx` at 0x10037a13 in its NxPhysics.dll,
	// checked in its disassembly). A compiler change that drops it moves nothing
	// the families compare (the recording allocator never fails).
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
	// As the listing (the call at 0x000544c0): Init is called on the pointer without a
	// null test, so a failed allocation calls it on null. In C++ that call is
	// undefined behaviour, which lets a compiler drop the null test below; it is
	// kept by the build of 14c8ec7 (`test ecx, ecx` at 0x10037aaa in its NxPhysics.dll,
	// checked in its disassembly). A compiler change that drops it moves nothing
	// the families compare (the recording allocator never fails).
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

#endif // standalone normal closure excludes unchanged TriangleMesh topology methods

// phys_fn_002081 (0x00052240, 59 B)
// The internal mesh's vertex normals, built on demand (thiscall on the
// InternalTriangleMesh, TriangleMesh +0x08; 001844 calls it when +0x18 is null):
// a block of vertex count x 12 bytes from the Foundation allocator
// (nxFoundationSDKAllocator, the import slot 0x101041bc: its slot 2, malloc(size,
// NX_MEMORY_PERSISTENT)), stored at +0x18 BEFORE it is filled, then
// NxBuildSmoothNormals(triangle count, vertex count, vertices, triangles, no
// 16-bit triangles, the block, 1) (002146, cdecl). Neither the block nor the
// result is tested, as in the listing. The listing's instructions, naked (the
// allocator slot is read through its import, as the listing reads it).
#if NX_PHYSICS_USE_X87
extern "C" void* nxTriangleMeshFoundationAllocatorSlot;	// the import slot 0x101041bc
#pragma comment(linker, "/alternatename:_nxTriangleMeshFoundationAllocatorSlot=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
extern "C" void nxTriangleMeshCallBuildSmoothNormals();		// 002146, NxBuildSmoothNormals
#pragma comment(linker, "/alternatename:_nxTriangleMeshCallBuildSmoothNormals=_NxBuildSmoothNormals")

__declspec(naked) void nxMeshComputeVertexNormals()
	{
	__asm
		{
		mov	eax, dword ptr nxTriangleMeshFoundationAllocatorSlot		// 0x00052240
		push	esi		// 0x00052245
		mov	esi, ecx		// 0x00052246
		mov	ecx, dword ptr [eax]		// 0x00052248
		mov	eax, dword ptr [esi]		// 0x0005224a
		mov	edx, dword ptr [ecx]		// 0x0005224c
		lea	eax, [eax + eax*2]		// 0x0005224e
		push	0		// 0x00052251
		shl	eax, 2		// 0x00052253
		push	eax		// 0x00052256
		call	dword ptr [edx + 8]		// 0x00052257
		mov	ecx, dword ptr [esi + 0xc]		// 0x0005225a
		mov	edx, dword ptr [esi + 8]		// 0x0005225d
		push	1		// 0x00052260
		push	eax		// 0x00052262
		push	0		// 0x00052263
		push	ecx		// 0x00052265
		mov	ecx, dword ptr [esi + 4]		// 0x00052266
		mov	dword ptr [esi + 0x18], eax		// 0x00052269
		mov	eax, dword ptr [esi]		// 0x0005226c
		push	edx		// 0x0005226e
		push	eax		// 0x0005226f
		push	ecx		// 0x00052270
		call	nxTriangleMeshCallBuildSmoothNormals		// 0x00052271
		add	esp, 0x1c		// 0x00052276
		pop	esi		// 0x00052279
		ret		// 0x0005227a
		}
	}

#else
#include "NxSmoothNormals.h"
void nxMeshComputeVertexNormals(InternalTriangleMesh* mesh)
{
    mesh->mVertexNormals = nxFoundationSDKAllocator->malloc(
        mesh->mVertexCount * 12u, NX_MEMORY_PERSISTENT);
    NxBuildSmoothNormals(mesh->mTriangleCount, mesh->mVertexCount,
        static_cast<const NxVec3*>(mesh->mVertices),
        static_cast<const NxU32*>(mesh->mTriangles), 0,
        static_cast<NxVec3*>(mesh->mVertexNormals), true);
}
#endif
