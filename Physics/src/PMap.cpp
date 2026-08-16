/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PMap.h"

#include "NxUserOutputStream.h"

// The pinned public NxPMap.h names both of these in NxCreatePMap's signature and
// includes neither; NxPhysics.h is what supplies them in a consumer build.
class NxTriangleMesh;
#include "NxPMap.h"

#include "Opcode.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// The reported file name is the oracle's, for the same reason PhysicsSDK.cpp's
// is: the two rejection sites pass __FILE__ and reproducing what the SDK reports
// means reproducing that string. Measured at .rdata 0x0010802c.
#define NX_PENETRATION_MAP_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\PenetrationMap.cpp"

// The two rejection sites report NovodeX's own __LINE__, pushed as immediates at
// 0x0005070c and 0x000506d7. This file is not that file and its own __LINE__ is
// its own, so the two numbers are named constants: recorded from the image, not
// manufactured by padding this file out to 986 lines.
static const NxI32	kPMapBadHeaderLine		= 0x3d3;	// 979
static const NxI32	kPMapBadVersionLine		= 0x3da;	// 986

// .rdata 0x00107f78, 0x00107fb8 and 0x00108000.
static const char* const kPMapBadHeaderMessage =
	"PenetrationMap::Create: the pmap file is invalid (bad header)";
static const char* const kPMapBadVersionMessage =
	"PenetrationMap::Create: the pmap file is invalid (bad version number)";
static const char* const kPMapNoMeshMessage =
	"PenetrationMap::Create: no triangle mesh";
static const NxI32	kPMapNoMeshLine			= 0x3c8;	// 968, pushed at 0x00050661

// The tag is four SEPARATE byte reads compared one at a time -- 0x0005069a,
// 0x000506a8, 0x000506b3 and 0x000506be -- so it has no byte order to get wrong.
static const NxU8	kPMapTag[4]				= { 'P', 'M', 'A', 'P' };
// `cmp eax,4` at 0x000506c9. Exactly four; 3 and 5 are both rejected.
static const NxU32	kPMapVersion			= 4;

// 0x00050110's terminator test is `cmp ebp,-1` at 0x0005020d.
static const NxU32	kPMapEndOfValues		= 0xffffffffu;

// The bit the sign block ORs in at 0x00050290 and the bit the corner pass ORs in
// at 0x00050466.
static const NxU32	kPMapCellFilled			= 0x80000000u;
static const NxU32	kPMapCellInterior		= 0x40000000u;

static_assert(sizeof(NxU32) == 4, "the grid is dwords");

// ---------------------------------------------------------------------------

// phys_fn_002045 at 0x000505f0. The AABB is seeded with FLT_MAX/-FLT_MAX and
// every word from +0x5c to +0x74 is cleared; +0x04 is NOT cleared here, it is
// written by the table build the constructor tail-calls at 0x0005062c.
PenetrationMap::PenetrationMap()
	{
	mMin[0] = mMin[1] = mMin[2] = 3.402823466e+38F;
	mMax[0] = mMax[1] = mMax[2] = -3.402823466e+38F;

	mResolution = 0;
	mResolutionSquared = 0;
	mLastIndex = 0.0f;
	mInvLastIndex = 0.0f;
	mCellCount = 0;
	mGrid = 0;
	mMesh = 0;

	// The constructor leaves the centre, extents and per-cell scales alone --
	// nothing between 0x000505f3 and 0x0005062c touches +0x20..+0x58 -- and so
	// does this one. They are written by setup() before anything reads them.
	mSpread = 0;
	buildSpreadTable();
	}

// phys_fn_001986 at 0x0004cb20. 256 dwords from the CRT heap, each entry the
// eight bits of its index spread three apart: bit i of the index lands at bit
// 3i. The seven `shl eax,2` and eight `and` pairs at 0x0004cb37..0x0004cb80 are
// that and nothing else, and the sort in finish() is the only reader.
void PenetrationMap::buildSpreadTable()
	{
	mSpread = static_cast<NxU32*>(malloc(0x400));
	if(!mSpread)
		return;

	for(NxU32 i = 0; i < 0x100; ++i)
		{
		NxU32 spread = 0;
		for(NxU32 bit = 0; bit < 8; ++bit)
			if(i & (1u << bit))
				spread |= 1u << (bit * 3);
		mSpread[i] = spread;
		}
	}

// phys_fn_001984 at 0x0004cae0. The grid first, the spread table second, both
// through the CRT free at 0x000f48bb, and both pointers nulled after.
PenetrationMap::~PenetrationMap()
	{
	if(mGrid)
		{
		free(mGrid);
		mGrid = 0;
		}
	if(mSpread)
		{
		free(mSpread);
		mSpread = 0;
		}
	}

// ---------------------------------------------------------------------------

// phys_fn_002033 at 0x0004ff80.
//
// NOTHING IS VALIDATED. There is no minimum, no maximum and no length field: a
// resolution of 0 allocates a zero-cell grid and a resolution of 0x400 asks for
// 4 GB, and both take the same path. The return value is `grid != 0`
// (`setne al` at 0x000500f2 and 0x00050104) and Create ignores it.
bool PenetrationMap::setup(NxU32 resolution, const NxF32* bounds)
	{
	mResolution = resolution;
	mResolutionSquared = resolution * resolution;

	// `lea eax,[ecx-1]` then `fild` at 0x0004ff9f, with the unsigned fixup at
	// 0x0004ffa5 that adds 2^32 when the value read as signed is negative --
	// which is `(NxF32)(NxU32)(resolution - 1)`.
	mLastIndex = static_cast<NxF32>(resolution - 1);
	mInvLastIndex = 1.0f / mLastIndex;

	mMin[0] = bounds[0]; mMin[1] = bounds[1]; mMin[2] = bounds[2];
	mMax[0] = bounds[3]; mMax[1] = bounds[4]; mMax[2] = bounds[5];

	for(int axis = 0; axis < 3; ++axis)
		{
		mCentre[axis] = (mMax[axis] + mMin[axis]) * 0.5f;
		mHalfExtents[axis] = (mMax[axis] - mMin[axis]) * 0.5f;
		mExtents[axis] = mMax[axis] - mMin[axis];
		}
	for(int axis = 0; axis < 3; ++axis)
		{
		mCellsPerUnit[axis] = mLastIndex / mExtents[axis];
		mUnitsPerCell[axis] = mInvLastIndex * mExtents[axis];
		}

	// `imul edi,ecx` at 0x000500b6 makes the third power out of the square, and
	// the store at 0x000500ca happens BEFORE the allocation, so a failed malloc
	// leaves the cell count set and the grid null.
	mCellCount = mResolutionSquared * resolution;
	mGrid = static_cast<NxU32*>(malloc(mCellCount * 4));
	if(mGrid)
		{
		// `rep stosd` with 0xffffffff at 0x000500ea, guarded by
		// `lea eax,[edi-1]; test eax,eax; jl` -- so a zero cell count writes
		// nothing rather than wrapping.
		for(NxU32 i = 0; i < mCellCount; ++i)
			mGrid[i] = 0xffffffffu;
		}
	return mGrid != 0;
	}

// ---------------------------------------------------------------------------

// phys_fn_002008 at 0x0004dba0, as far as the recorded fixtures establish it.
//
// The first three instructions are the ONE place in this format where the
// resolution is interpreted: `cmp eax,0x20` -> 5, `cmp eax,0x40` -> 6,
// `cmp eax,0x50` -> 7 at 0x0004dbab, 0x0004dbb8 and 0x0004dbc4, defaulting to 0
// for every other resolution. So the cell-run encoding is defined for
// resolutions 32, 64 and 80 and for nothing else, while the header above accepts
// any resolution at all.
//
// Then a 32-bit element count MSB first, then the container is emptied
// (`[eax+4] = 0` at 0x0004dc13), and a zero count returns 0 without entering the
// walk (0x0004dc1c to 0x0004e109).
//
// THE WALK ITSELF IS NOT RECONSTRUCTED. Each element is a 5-bit code dispatched
// through the 32-way jump table at 0x0004dc75 which moves three cursor globals,
// and the meaning of the codes is unestablished. The count read and the
// code-width selection above are reproduced because they are measurable; the
// walk refuses.
NxU32 PenetrationMap::decodeCellRun(MemoryStream& stream, IceCore::Container& cells, NxU32 resolution)
	{
	NxU32 codeWidth = 0;
	if(resolution == 0x20)		codeWidth = 5;
	else if(resolution == 0x40)	codeWidth = 6;
	else if(resolution == 0x50)	codeWidth = 7;
	(void) codeWidth;

	NxU32 count = stream.readBitsMsbFirst(32);
	cells.Reset();
	if(count == 0)
		return 0;

	// NOT RECONSTRUCTED -- see above. Reaching here means a fixture drove a
	// non-empty cell run, which nothing recorded does.
	NX_ASSERT(!"PenetrationMap cell-run walk is not reconstructed");
	return 0;
	}

// ---------------------------------------------------------------------------

// phys_fn_002035 at 0x00050110, the arm the loader takes when the caller supplies
// a stream. The filename arm at 0x00050123 and the build-your-own-stream arm at
// 0x0005017b are not reconstructed; see PMap.h.
bool PenetrationMap::loadPayload(MemoryStream& stream)
{
	// 0x00050157: the grid is refilled with 0xffffffff before the payload runs,
	// even though setup() has just done it. Reproduced because it is what the
	// row does, not because anything depends on it.
	for(NxU32 i = 0; i < mCellCount; ++i)
		mGrid[i] = 0xffffffffu;

	// The value loop from 0x000501b0. `next` is the running previous-plus-one:
	// `mov ebp,edi; inc edi` at 0x0005020a, with edi zeroed once at 0x000501b0
	// and never reset, so a set flag bit means "the value before this one, plus
	// one" and the first record's implicit predecessor is -1.
	NxU32 next = 0;
	for(;;)
		{
		NxU32 value;
		if(stream.readBit())
			value = next;
		else
			value = stream.readBitsMsbFirst(32);
		next = value + 1;

		if(value == kPMapEndOfValues)
			break;

		IceCore::Container cells;
		NxU32 count = decodeCellRun(stream, cells, mResolution);
		const udword* entries = cells.GetEntries();
		for(NxU32 i = 0; i < count; ++i)
			mGrid[entries[i]] = value;
		}

	// The sign block from 0x00050260: one bit per cell, and a CLEAR bit sets the
	// top bit of that cell. `setne cl; test cl,cl; jne` at 0x0005027b skips the
	// OR when the bit is set, which is the inverse of what it reads like.
	for(NxU32 i = 0; i < mCellCount; ++i)
		if(stream.readBit() == 0)
			mGrid[i] |= kPMapCellFilled;

	return true;
	}

// ---------------------------------------------------------------------------

// phys_fn_002037 at 0x000502d0 and its three continuations. Two passes.
//
// PASS ONE, 0x000502f0-0x0005049f. For every cell it takes the EIGHT corners of
// the unit cube whose low corner it is -- a, a+1, a+n, a+n+1, a+n*n, a+n*n+1,
// a+n*n+n, a+n*n+n+1, assembled at 0x00050310-0x0005034d -- drops the ones that
// would leave the grid (0x0005035a for k, 0x00050379 for j, 0x0005038c for i,
// each writing -1 into the four indices that axis invalidates) and sets bit 30
// of the cell when EVERY surviving corner has bit 31 set.
//
// The sense is worth being exact about. Each test is
// `shr edx,0x1f; not edx; test dl,1; jne <skip>`: bit 31 clear gives ~0 and
// takes the jump, bit 31 set gives ~1 and falls through. So the OR at 0x00050466
// happens when all eight corners are SET, not when they are clear.
//
// PASS TWO, 0x000504a5-0x000505d9. Builds one key per cell out of the spread
// table -- `table[k] + 2*table[j] + 4*table[i]` at 0x000504dc/0x000504df, which
// interleaves the three indices into a Morton code -- radix-sorts them, and
// permutes the grid into that order: newGrid[i] = grid[ranks[i]] at 0x00050599,
// copied back at 0x000505b6.
bool PenetrationMap::finish()
	{
	const NxU32 n = mResolution;
	const NxU32 nn = mResolutionSquared;

	for(NxU32 i = 0; i < n; ++i)
		for(NxU32 j = 0; j < n; ++j)
			for(NxU32 k = 0; k < n; ++k)
				{
				const NxU32 a = i * nn + j * n + k;
				NxI32 corner[8];
				corner[0] = static_cast<NxI32>(a);
				corner[1] = static_cast<NxI32>(a + 1);
				corner[2] = static_cast<NxI32>(a + n);
				corner[3] = static_cast<NxI32>(a + n + 1);
				corner[4] = static_cast<NxI32>(a + nn);
				corner[5] = static_cast<NxI32>(a + nn + 1);
				corner[6] = static_cast<NxI32>(a + nn + n);
				corner[7] = static_cast<NxI32>(a + nn + n + 1);
				if(k == n - 1)
					corner[1] = corner[3] = corner[5] = corner[7] = -1;
				if(j == n - 1)
					corner[2] = corner[3] = corner[6] = corner[7] = -1;
				if(i == n - 1)
					corner[4] = corner[5] = corner[6] = corner[7] = -1;

				bool interior = true;
				for(int c = 0; c < 8 && interior; ++c)
					if(corner[c] != -1 && (mGrid[corner[c]] & kPMapCellFilled) == 0)
						interior = false;
				if(interior)
					mGrid[a] |= kPMapCellInterior;
				}

	IceCore::Container keys;
	for(NxU32 i = 0; i < n; ++i)
		for(NxU32 j = 0; j < n; ++j)
			for(NxU32 k = 0; k < n; ++k)
				keys.Add(mSpread[k] + 2 * mSpread[j] + 4 * mSpread[i]);

	RadixSort sorter;
	const udword* ranks = sorter.Sort(keys.GetEntries(), keys.GetNbEntries(), RADIX_SIGNED).GetRanks();

	NxU32* sorted = static_cast<NxU32*>(malloc(mCellCount * 4));
	if(sorted)
		{
		for(NxU32 i = 0; i < mCellCount; ++i)
			sorted[i] = 0xffffffffu;
		for(NxU32 i = 0; i < mCellCount; ++i)
			sorted[i] = mGrid[ranks[i]];
		memcpy(mGrid, sorted, mCellCount * 4);
		free(sorted);
		}
	return true;
	}

// ---------------------------------------------------------------------------

// phys_fn_002047 at 0x00050640, the LOAD arm.
bool PenetrationMap::create(const void* mesh, NxU32 resolution, const char* filename,
	MemoryStream* stream, bool load, NxUserOutputStream* outputStream)
	{
	// 0x00050652. A null mesh is refused before anything else, and the refusal
	// is silent when there is no output stream to report through: every one of
	// the three sites tests it first.
	if(!mesh)
		{
		if(outputStream)
			outputStream->reportError(NXE_INVALID_PARAMETER, kPMapNoMeshMessage,
				NX_PENETRATION_MAP_CPP, kPMapNoMeshLine);
		return false;
		}

	// 0x0005067f and 0x0005068a: the header is only read when BOTH a stream and
	// the load flag are present. Otherwise the resolution argument stands and
	// the COMPUTE arm runs.
	if(stream && load)
		{
		for(int i = 0; i < 4; ++i)
			if(stream->readByte() != kPMapTag[i])
				{
				if(outputStream)
					outputStream->reportError(NXE_INTERNAL_ERROR, kPMapBadHeaderMessage,
						NX_PENETRATION_MAP_CPP, kPMapBadHeaderLine);
				return false;
				}

		if(stream->readDword() != kPMapVersion)
			{
			if(outputStream)
				outputStream->reportError(NXE_INTERNAL_ERROR, kPMapBadVersionMessage,
					NX_PENETRATION_MAP_CPP, kPMapBadVersionLine);
			return false;
			}

		// 0x000506f7. The resolution comes out of the file and replaces the
		// argument -- `mov [ebp+0xc],ebx` at 0x000506fe.
		resolution = stream->readDword();
		}

	// 0x0005072d. The six floats at mesh+0x44 are the mesh's AABB.
	setup(resolution, reinterpret_cast<const NxF32*>(static_cast<const NxU8*>(mesh) + 0x44));
	mMesh = mesh;

	if(load && loadPayload(*stream))
		return finish();

	// 0x00050768. The COMPUTE arm, reached both when the caller asked for one
	// and when a load failed. NOT RECONSTRUCTED -- see PMap.h. Refusing is not
	// what the oracle does here and this return is a hole, not a behaviour.
	NX_ASSERT(!"PenetrationMap compute path is not reconstructed");
	return false;
	}

// ---------------------------------------------------------------------------
// The exports.

// phys_fn_002051 at 0x00051040, 32 bytes, every one of them driven.
//
// Three things a reimplementation gets wrong by default, and all three are
// measured by the asset differential:
//
//   * it frees `data` through the CRT free at 0x000f48bb, not through the SDK
//     allocator, because NxCreatePMap malloc'd it;
//   * it leaves `dataSize` ALONE. Nothing between 0x00051040 and 0x0005105f
//     writes [esi];
//   * it returns true unconditionally -- `mov al,1` at 0x0005105c is outside
//     the `test eax,eax` at 0x00051048 -- so it reports success for a PMap it
//     did not release.
NX_C_EXPORT NXP_DLL_EXPORT bool NX_CALL_CONV NxReleasePMap(NxPMap& pmap)
	{
	if(pmap.data)
		{
		free(pmap.data);
		pmap.data = 0;
		}
	return true;
	}
