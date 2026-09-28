/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// NovodeX's hull library around qhull, band A of the qhull span
// (0x0007d420-0x0007ed43): the qhull host object and its tracked allocator
// (qhull-gap Task 4a), the hull driver, its result, the box fallback and the
// OBJ writers (Task 4b). Contract: units/convex-cooking-contract.md
// (docs/reconstruction/novodex-physics); the bundle is
// units/gap__Controller.cpp__to__fluids__Fluid.cpp.md.
//
// The file name is DESCRIPTIVE, like every name in QhullHost.h. The one
// constraint the image puts on it: band A links between qhull.c and qset.c,
// so if NovodeX's objects were linked in the same alphabetical order as
// qhull's, its name sorts between those two.
//
// Rows are in address order, each under its stable-ID line. Floating point
// follows core/Joint.cpp: this translation unit is x87 in the oracle and is
// built /arch:IA32 here; a value the listing keeps on the FPU stack is a
// `double`, a value it stores (fstp dword) is an `NxReal`. No row here touches
// the control word; cooking runs under the caller's.
//
// What the shipped object does that the earlier shims did not, reproduced on
// purpose:
//
//   * slot +0x10 (print) formats the message and then calls errexit(1): ANY
//     qhull print through the host ends the hull attempt;
//   * slot +0x20 (errexit) releases the arrays and longjmps to the driver's
//     jmp_buf at .data:0x00125040;
//   * every qhull allocation is a tracked block ("JOHNRAT" header, 4,096
//     slots), reclaimed by the destructor -- the driver never calls
//     qh_freeqhull;
//   * a full slot table frees the new block with the CRT's free even when the
//     user allocator made it (0x0007e8d4). An oracle defect, kept.

#define _CRT_SECURE_NO_WARNINGS

#include "QhullHost.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// .data:0x00125040, the only jmp_buf in the band: _setjmp3 in 003279
// (0x0007eae7) and longjmp in 003267 (0x0007e54f).
jmp_buf gQhullJump;
// .data:0x00125080. Written once, at 0x0007ea51, with the address of
// CreateConvexHull's stack object, and never cleared: after the driver returns
// it points at a dead frame, as the oracle's does.
QhullHost* gQhullHost = 0;

// phys_fn_003238 (0x0007d500, 106 B)
void QhullHost::releaseArrays()
	{
	if(mPoints)
		{
		trackedFree(mPoints);
		mPoints = 0;
		}
	if(mRemap)
		{
		trackedFree(mRemap);
		mRemap = 0;
		}
	if(mOutputVertices)
		{
		trackedFree(mOutputVertices);
		mOutputVertices = 0;
		mOutputCount = 0;
		}
	if(mIndices)
		{
		trackedFree(mIndices);
		mIndices = 0;
		mIndexCount = 0;
		}
	mNumPoints = 0;
	}

// phys_fn_003240 (0x0007d570, 32 B)
void* QhullHost::rawAlloc(size_t size)
	{
	if(mAllocator)
		return mAllocator->malloc(size);
	return ::malloc(size);
	}

// phys_fn_003241 (0x0007d590, 31 B)
void QhullHost::rawFree(void* memory)
	{
	if(mAllocator)
		{
		mAllocator->free(memory);
		return;
		}
	::free(memory);
	}

// phys_fn_003257 (0x0007e370, 95 B)
QhullHost::QhullHost(HullAllocator* allocator)
	{
	mAllocator = allocator;
	mPoints = 0;
	mRemap = 0;
	mLiveBlocks = 0;
	mLiveBytes = 0;
	mPeakBytes = 0;
	mSearchStart = 0;
	mIndices = 0;
	mOutputVertices = 0;
	mOutputCount = 0;
	mIndexCount = 0;
	memset(mLive, 0, sizeof(mLive));
	mArea = 0.0f;
	mVolume = 0.0f;
	}

// phys_fn_003259 (0x0007e3d0, 210 B)
// The fourth argument is ignored (ret 0x10).
void QhullHost::offBegin(int dim, NxU32 numpoints, NxU32 numfacets, int /*numridges*/)
	{
	releaseArrays();
	mDim = dim;
	mNumFacets = numfacets;
	mNumPoints = numpoints;
	mTriangleCount = 0;
	mPointCount = 0;
	mFacetCount = 0;
	mOutputCount = 0;
	mIndexCount = 0;
	mIndexCapacity = 0;
	if(numpoints)
		{
		mPoints = (NxReal*) trackedMalloc(sizeof(NxReal) * 3 * numpoints);
		mRemap = (NxU32*) trackedMalloc(sizeof(NxU32) * mNumPoints);
		memset(mRemap, 0, sizeof(NxU32) * mNumPoints);
		memset(mPoints, 0, sizeof(NxReal) * 3 * mNumPoints);
		mOutputVertices = (NxReal*) trackedMalloc(sizeof(NxReal) * 3 * mNumPoints);
		mIndexCapacity = mNumPoints * 16;
		mIndices = (NxU32*) trackedMalloc(sizeof(NxU32) * mIndexCapacity);
		}
	}

// phys_fn_003261 (0x0007e4b0, 43 B)
void QhullHost::point3(NxReal x, NxReal y, NxReal z)
	{
	if(mPointCount < mNumPoints)
		{
		NxReal* dest = &mPoints[mPointCount * 3];
		dest[0] = x;
		dest[1] = y;
		dest[2] = z;
		mPointCount++;
		}
	}

// phys_fn_003263 (0x0007e4e0, 60 B)
// Variadic, so `this` is on the stack ([esp+4]). The 0x2000-byte buffer is
// formatted (the three-argument CRT call at 0x000f621b) and never read.
int QhullHost::print(FILE* /*stream*/, const char* format, ...)
	{
	char buffer[0x2000];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	errexit(1);
	return 0;
	}

// phys_fn_003265 (0x0007e520, 23 B)
void QhullHost::size(NxReal area, NxReal volume)
	{
	mArea = area;
	mVolume = volume;
	}

// phys_fn_003267 (0x0007e540, 32 B)
void QhullHost::errexit(int code)
	{
	releaseArrays();
	longjmp(gQhullJump, code);
	}

// phys_fn_003268 (0x0007e560, 210 B)
// The output vertices are qhull's point ids compacted in first-use order;
// mRemap holds each one's 1-based output id.
void QhullHost::facet(NxU32 count, const NxU32* ids)
	{
	if(mFacetCount < mNumFacets)
		{
		mTriangleCount += count - 2;
		if(mIndexCount < mIndexCapacity)
			mIndices[mIndexCount++] = count;
		for(NxU32 i = 0; i < count; i++)
			{
			NxU32 id = ids[i];
			if(id < mNumPoints)
				{
				if(mRemap[id] == 0)
					{
					const NxReal* source = &mPoints[id * 3];
					NxReal* dest = &mOutputVertices[mOutputCount * 3];
					dest[0] = source[0];
					dest[1] = source[1];
					dest[2] = source[2];
					mOutputCount++;
					mRemap[id] = mOutputCount;
					}
				if(mIndexCount < mIndexCapacity)
					mIndices[mIndexCount++] = mRemap[id] - 1;
				}
			}
		mFacetCount++;
		}
	}

// phys_fn_003272 (0x0007e810, 222 B)
// Scans mLive from mSearchStart (never advanced) for a free slot, at most
// 0x1000 entries. With none free the block goes back to the CRT's free even
// when mAllocator made it (0x0007e8d4).
void* QhullHost::trackedMalloc(size_t size)
	{
	BlockHeader* block;
	if(mAllocator)
		block = (BlockHeader*) mAllocator->malloc(size + sizeof(BlockHeader));
	else
		block = (BlockHeader*) ::malloc(size + sizeof(BlockHeader));
	if(!block)
		return block;
	NxU32 slot = mSearchStart;
	for(NxU32 i = 0; i < 0x1000; i++)
		{
		if(!mLive[slot])
			break;
		slot++;
		if(slot == 0x1000)
			slot = 0;
		}
	if(mLive[slot])
		{
		::free(block);
		return 0;
		}
	mLiveBlocks++;
	mLiveBytes += size;
	block->init(size, slot);
	mLive[slot] = block;
	if(mLiveBytes > mPeakBytes)
		mPeakBytes = mLiveBytes;
	return block + 1;
	}

// phys_fn_003274 (0x0007e8f0, 48 B)
void BlockHeader::init(size_t size, NxU32 slot)
	{
	mTag[0] = 'J';
	mTag[1] = 'O';
	mTag[2] = 'H';
	mTag[3] = 'N';
	mTag[4] = 'R';
	mTag[5] = 'A';
	mTag[6] = 'T';
	mTag[7] = 0;
	mSize = size;
	mSlot = slot;
	}

// phys_fn_003275 (0x0007e920, 92 B)
// A pointer without the header (first four bytes "JOHN", non-zero size) is
// ignored.
void QhullHost::trackedFree(void* memory)
	{
	if(!memory)
		return;
	BlockHeader* block = (BlockHeader*) memory - 1;
	if(block->mTag[0] == 'J' && block->mTag[1] == 'O' && block->mTag[2] == 'H' && block->mTag[3] == 'N'
		&& block->mSize)
		{
		mLive[block->mSlot] = 0;
		mLiveBlocks--;
		mLiveBytes -= block->mSize;
		if(mAllocator)
			mAllocator->free(block);
		else
			::free(block);
		}
	}

// phys_fn_003277 (0x0007e980, 130 B)
// Frees every live tracked block: the inlined trackedFree, including its null
// test on the user pointer (0x0007e99f).
QhullHost::~QhullHost()
	{
	releaseArrays();
	for(NxU32 i = 0; i < 0x1000; i++)
		{
		if(mLive[i])
			QhullHost::trackedFree(mLive[i] + 1);
		}
	}

// Slot +0x1c is phys_fn_001583 (0x0002ea70), a one-byte `ret` shared by
// folding: the narrow-hull hook does nothing. Not a row of this unit.
void QhullHost::narrowHull()
	{
	}

//////////////////////////////////////////////////////////////////////////////
// The seam. qhull's call sites reach the host as `mov ecx,[0x10125080];
// mov edx,[ecx]; call [edx+slot]`; the vendored tree calls these nine C hooks
// at the same sites (External/qhull/novodex/QhullNovodeXHost.h), and each
// makes that call on the published object. They are not rows.

extern "C" {
#include "..\..\External\qhull\novodex\QhullNovodeXHost.h"
}

void qhNovodeXOffBegin(int dim, int numpoints, int numfacets, int numridges)
	{
	gQhullHost->offBegin(dim, (NxU32) numpoints, (NxU32) numfacets, numridges);
	}

void qhNovodeXPoint3(float x, float y, float z)
	{
	gQhullHost->point3(x, y, z);
	}

void qhNovodeXFacet3Vertex(int count, int* pointids)
	{
	gQhullHost->facet((NxU32) count, (const NxU32*) pointids);
	}

void qhNovodeXSize(float totarea, float totvol)
	{
	gQhullHost->size(totarea, totvol);
	}

// Slot +0x10 is variadic and C cannot forward `...`, so the hook runs 003263's
// body against the published object: format into 0x2000 bytes, then
// errexit(1) through the vtable. It does not return.
int qhNovodeXFprintf(FILE* /*stream*/, const char* format, ...)
	{
	char buffer[0x2000];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	gQhullHost->errexit(1);
	return 0;
	}

void* qhNovodeXMalloc(size_t size)
	{
	return gQhullHost->trackedMalloc(size);
	}

void qhNovodeXFree(void* memory)
	{
	gQhullHost->trackedFree(memory);
	}

void qhNovodeXNarrowHull()
	{
	gQhullHost->narrowHull();
	}

// Reached from qh_errexit (phys_fn_003413): releases the arrays and longjmps
// to the driver. It does not return.
void qhNovodeXErrexit(int exitcode)
	{
	gQhullHost->errexit(exitcode);
	}
