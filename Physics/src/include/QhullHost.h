#ifndef NX_PHYSICS_QHULLHOST
#define NX_PHYSICS_QHULLHOST
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// NovodeX's hull library around qhull: the qhull host object, the hull driver
// and its descriptor/result. The layouts, the row assignment and the call chain
// are units/convex-cooking-contract.md (docs/reconstruction/novodex-physics).
//
// EVERY class, member and file name here is DESCRIPTIVE. The image carries no
// symbol for any of these rows; the interface shape is that of Stan Melax's and
// John Ratcliff's HullLibrary (HullDesc, HullResult, CreateConvexHull,
// ReleaseResult, the "JOHNRAT" block header), so the Ratcliff names are used
// where the shape matches, and names from the contract's role column elsewhere.
// No original identifier is evidenced.

#include "Nxf.h"

#include <stdio.h>
#include <stddef.h>
#include <setjmp.h>

// The user allocator the library takes (HullLibrary +0x00, QhullHost +0x4048):
// slot 0 malloc(size), slot 1 free(p), both thiscall. In NovodeX's only call
// (phys_fn_002233) it is the TriangleMesh itself, whose vtable
// .rdata:0x00108608 starts with 002235 (malloc) and 002237 (free). NULL means
// the CRT's malloc/free.
class HullAllocator
	{
	public:
	virtual	void*			malloc(size_t size) = 0;		// slot 0 (+0x00)
	virtual	void			free(void* memory) = 0;			// slot 1 (+0x04)
	};

// The descriptor phys_fn_002233 fills (0x1c bytes). Flag bits: contract,
// "Object layouts" (NovodeX passes 0xb7).
enum HullFlag
	{
	QF_WELD				= 0x01,	// weld duplicates in cleanupVertices
	QF_NORMALIZE		= 0x02,	// normalise by the extents, rescale the result
	QF_REDUCE			= 0x04,	// reduce to mMaxVertices through the quantizer
	QF_POLYGONIZER		= 0x08,	// hand the result to HullLibrary::mPolygonizer
	QF_TRIANGLES		= 0x10,	// triangulate (off: polygons)
	QF_REVERSE_ORDER	= 0x20,	// reverse the winding
	QF_WRITE_OK_OBJ		= 0x40,	// QHULL_OK_%04d.obj on success
	QF_WRITE_FAIL_OBJ	= 0x80	// QHULL_FAIL_%04d.obj on a qhull failure
	};

struct HullDesc
	{
	NxU32			mFlags;			// +0x00
	NxU32			mVcount;		// +0x04
	const NxReal*	mVertices;		// +0x08
	NxU32			mVertexStride;	// +0x0c
	NxReal			mNormalEpsilon;	// +0x10  the weld epsilon
	NxU32			mMaxVertices;	// +0x14
	NxReal			mUnknown18;		// +0x18  read only on the QF_POLYGONIZER path; meaning unestablished
	};

// 0x1c bytes. mPolygons is 0 in triangle mode.
struct HullResult
	{
	bool			mPolygons;				// +0x00
	NxU32			mNumOutputVertices;		// +0x04
	NxReal*			mOutputVertices;		// +0x08  float[3] each
	NxU32			mNumFaces;				// +0x0c
	NxU32			mNumTriangles;			// +0x10  sum(n-2)
	NxU32			mNumIndices;			// +0x14
	NxU32*			mIndices;				// +0x18
	};

enum HullError
	{
	QE_OK	= 0,
	QE_FAIL	= 1
	};

// The local CreateConvexHull hands the +4 interface (0x18 bytes at the
// driver's ebp+0x38). Only the call sites describe it.
struct HullPolygonizerResult
	{
	bool			mFlag;			// +0x00
	NxU32			mVcount;		// +0x04
	NxReal*			mVertices;		// +0x08
	NxU32			mFaceCount;		// +0x0c
	NxU32			mIndexCount;	// +0x10
	NxU32*			mIndices;		// +0x14
	};

// HullLibrary +0x04. NULL in NovodeX's only call, so its slots are known only
// from 003279's call sites (0x0007ebe6-0x0007ecd5); +0x04 and +0x0c are never
// called and are placeholders for the slot numbering.
class HullPolygonizer
	{
	public:
	virtual	bool			fromPolygons(HullPolygonizerResult& out, NxReal unknown18, NxU32 vcount,
								const NxReal* vertices, NxU32 stride, NxU32 faceCount, const NxU32* indices) = 0;	// +0x00
	virtual	void			unknown04() = 0;																		// +0x04
	virtual	bool			fromTriangles(HullPolygonizerResult& out, NxReal unknown18, NxU32 vcount,
								const NxReal* vertices, NxU32 stride, NxU32 faceCount, const NxU32* indices) = 0;	// +0x08
	virtual	void			unknown0c() = 0;																		// +0x0c
	virtual	void			release(HullPolygonizerResult& out) = 0;												// +0x10
	virtual	bool			finishPolygons(HullPolygonizerResult& out) = 0;											// +0x14
	};

// Band B's entry, reduceVertices (phys_fn_003369, 0x00080e90, thiscall,
// `ret 0x18`): Wu's colour quantizer reducing the cleaned cloud to
// maxVertices points (Physics/src/Quantizer.cpp, qhull-gap piece 4d).
// cleanupVertices calls it on an object with no fields, built in a dead
// argument slot of its own frame (`lea ecx,[esp+0x6c]` at 0x0007da34, the
// `weld` slot), passing the host's user allocator and the same buffer as input
// and output. It returns `this` (0x000814c3); cleanupVertices does not read it
// (it returns true after the call, 0x0007da40).
class HullVertexReducer
	{
	public:
	HullVertexReducer*		reduceVertices(HullAllocator* allocator, NxU32 svcount, const NxReal* svertices,
								NxU32& vcount, NxReal* vertices, NxU32 maxVertices);
	};

// 8 bytes, on phys_fn_002233's stack.
class HullLibrary
	{
	public:
	HullError				CreateConvexHull(const HullDesc& desc, HullResult& result);
	HullError				ReleaseResult(HullResult& result);

	HullAllocator*			mAllocator;		// +0x00
	HullPolygonizer*		mPolygonizer;	// +0x04
	};

// The 16-byte header in front of every tracked block.
struct BlockHeader
	{
	void					init(size_t size, NxU32 slot);

	char					mTag[8];		// "JOHNRAT\0"
	size_t					mSize;
	NxU32					mSlot;
	};

// The qhull host object: the stack arena CreateConvexHull publishes at
// .data:0x00125080. 0x4054 bytes; vtable .rdata:0x00113614, nine slots.
// The slots named here trackedMalloc/trackedFree/print are the contract's
// malloc/free/fprintf: renamed so a member cannot shadow the CRT function a
// row calls directly.
class QhullHost
	{
	public:
							QhullHost(HullAllocator* allocator);
							~QhullHost();

	virtual	void			offBegin(int dim, NxU32 numpoints, NxU32 numfacets, int numridges);	// +0x00
	virtual	void			point3(NxReal x, NxReal y, NxReal z);								// +0x04
	virtual	void			facet(NxU32 count, const NxU32* ids);								// +0x08
	virtual	void			size(NxReal area, NxReal volume);									// +0x0c
	virtual	int				print(FILE* stream, const char* format, ...);						// +0x10
	virtual	void*			trackedMalloc(size_t size);											// +0x14
	virtual	void			trackedFree(void* memory);											// +0x18
	virtual	void			narrowHull();														// +0x1c
	virtual	void			errexit(int code);													// +0x20

	void					releaseArrays();
	void*					rawAlloc(size_t size);
	void					rawFree(void* memory);
	bool					cleanupVertices(NxU32 svcount, const NxReal* svertices, NxU32 stride,
								NxU32& vcount, NxReal* vertices, NxReal normalepsilon, NxReal* scale,
								bool weld, bool reduce, NxU32 maxVertices);
	void					writeOkObj(const HullResult& result);
	void					writeFailObj(NxU32 vcount, const NxReal* vertices, NxU32 stride);
	void					boxFallback(NxU32& vcount, NxReal* vertices);
	bool					buildResult(HullResult& result, bool triangles, bool reverse);

	int						mDim;				// +0x04
	NxU32*					mRemap;				// +0x08  1-based output id per qhull point
	NxReal*					mPoints;			// +0x0c  float3[mNumPoints]
	NxU32					mNumPoints;			// +0x10
	NxU32					mPointCount;		// +0x14
	NxU32					mFacetCount;		// +0x18
	NxU32					mNumFacets;			// +0x1c
	NxU32					mTriangleCount;		// +0x20
	NxU32					mLiveBlocks;		// +0x24
	NxU32					mLiveBytes;			// +0x28
	NxU32					mPeakBytes;			// +0x2c
	NxU32					mSearchStart;		// +0x30
	BlockHeader*			mLive[0x1000];		// +0x34
	NxU32					mOutputCount;		// +0x4034
	NxReal*					mOutputVertices;	// +0x4038
	NxU32					mIndexCount;		// +0x403c
	NxU32					mIndexCapacity;		// +0x4040
	NxU32*					mIndices;			// +0x4044
	HullAllocator*			mAllocator;			// +0x4048
	NxReal					mArea;				// +0x404c
	NxReal					mVolume;			// +0x4050
	};

// The process globals of the band (contract, "Error and degenerate paths":
// cooking is single-threaded and not reentrant).
extern jmp_buf		gQhullJump;			// .data:0x00125040
extern QhullHost*	gQhullHost;			// .data:0x00125080
extern int			gQhullObjCounter;	// .data:0x00125084

int runQhull(int argc, char** argv, int n, const NxReal* points);

#endif
