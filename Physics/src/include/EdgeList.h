#ifndef NX_PHYSICS_EDGELIST_H
#define NX_PHYSICS_EDGELIST_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The ICE-shaped edge list of `\Epic\Novodex\SDKs\Physics\src\EdgeList.cpp`
// (the oracle's own __FILE__, .rdata 0x101080cc), prerequisite P-EdgeList of
// units/convex-mesh-gap-contract.md, written by convex-mesh gap Task 2c. None of
// it has vendored source (OPCODE 1.3's Ice/ holds no EdgeList); every layout
// below is read off the listing sites cited.
//
// It also holds the allocation helpers the ICE-shaped rows of Task 2c share
// (EdgeList.cpp, IceAdjacencies.cpp, IceMeshTools.cpp): every allocation in them
// goes through the 004803 getter (nxGetSdkAllocator) -- slot 0 with (size,
// type), slot 3 to release -- and none through the imported
// nxFoundationSDKAllocator or the CRT.

#include "NxPhysicsBackend.h"
#if NX_PHYSICS_USE_X87
#include "PhysicsInternal.h"
#else
#include "NxSdkAllocator.h"
#endif
#include "Opcode.h"

// The allocator singleton's slot 0 and slot 3 (0x000b4000 -> [vtable+0x00] and
// [vtable+0x0c]). `type` is the listing's pushed word: 0 persistent, 1 temporary.
inline void* nxIceAlloc(unsigned size, NxMemoryType type)
	{
	return nxGetSdkAllocator()->malloc(size, type);
	}

inline void nxIceFree(void* memory)
	{
	nxGetSdkAllocator()->free(memory);
	}

// `new T[count]` as the listing compiles it for the ICE element types: one block
// of count * size + 4 bytes, the count stored in its first word, the array one
// word in; the element constructor is the identity row 0x10027f00 run by the
// vector constructor iterator (000001), which writes nothing. Released through
// slot 3 with the pointer minus four. The size is formed in 32 bits, as the
// listing's `lea` forms it.
inline void* nxIceNewArray(unsigned count, unsigned size, NxMemoryType type)
	{
	unsigned* block = (unsigned*) nxIceAlloc(count * size + 4, type);
	if(!block)
		return 0;
	block[0] = count;
	return block + 1;
	}

inline void nxIceDeleteArray(void* array)
	{
	nxIceFree((unsigned*) array - 1);
	}

// One edge, two vertex references, smaller first (8 bytes; 0x00051080 builds
// them in a `new[]` of 8-byte elements and 0x00051781 walks them by 8).
struct EdgeData
	{
	NxU32	Ref0;
	NxU32	Ref1;
	};

// Per edge: flags (bit 0 = active, set at 0x00051b0f), the number of faces that
// share it (+2, a word) and the offset of its run in the faces-by-edges array
// (+4). Constructed by 0x1002a610, which zeroes all three.
struct EdgeDesc
	{
	NxU16	Flags;
	NxU16	Count;
	NxU32	Offset;
	};

// Per face: the edge index of its edges (0,1), (1,2) and (2,0), in bits 0-27
// (0x0fffffff, 0x00051aa2); bit 31 marks an active edge and bit 30 an active
// vertex (0x00051aae, 0x00051d53).
struct EdgeTriangle
	{
	NxU32	mLink[3];
	};

// The create block 002063 reads (0x00051dd0): +0x00 face count, +0x04 32-bit
// faces, +0x08 16-bit faces, +0x0c and +0x0d two flag bytes, +0x10 vertices,
// +0x14 epsilon (passed on to 002061, which never reads it).
struct EDGELISTCREATE
	{
	NxU32			NbFaces;
	const NxU32*	DFaces;
	const NxU16*	WFaces;
	bool			FacesToEdges;
	bool			VerticesToEdges;
	const IceMaths::Point*	Verts;
	float			Epsilon;
	};

// The edge list (0x18 bytes: 002052 zeroes +0x00, +0x04, +0x0c, +0x10 and
// +0x14; +0x08 is written by 002054 before it is read).
class EdgeList
	{
	public:
				EdgeList();
				~EdgeList();

	bool		Init(const EDGELISTCREATE& create);
	bool		CreateFacesToEdges(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces);
	bool		CreateEdgesToFaces(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces);
	bool		ComputeActiveEdges(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces,
					const IceMaths::Point* verts, float epsilon);

	NxU32			mNbEdges;			// +0x00
	EdgeData*		mEdges;				// +0x04, new[] with a count cookie
	NxU32			mNbFaces;			// +0x08
	EdgeTriangle*	mEdgeFaces;			// +0x0c, new[] with a count cookie
	EdgeDesc*		mEdgeToTriangles;	// +0x10, no cookie
	NxU32*			mFacesByEdges;		// +0x14, no cookie
	};

#endif
