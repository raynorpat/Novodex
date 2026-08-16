#ifndef NX_PHYSICS_MEMORYSTREAM
#define NX_PHYSICS_MEMORYSTREAM
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"

/**
The SDK-private block stream the penetration-map loader reads through. It is NOT
NxStream: NxStream is the public abstract interface with a virtual destructor and
thirteen pure virtuals, and every read here is a direct call with no vtable in
sight -- phys_fn_002047 reaches phys_fn_004772 and phys_fn_004774 by
`call rel32`, not through `[eax+n]`.

The class has no `__FILE__` literal anywhere in the image, so it has no recovered
name. It is called MemoryStream here for what it does.

The stream walks a singly linked list of blocks and keeps a one-byte MSB-first
bit accumulator beside the read cursor, and PenetrationMap's payload decoder
drives both. sizeof is 0x1c, from `push 0x1c` at 0x00050184 in phys_fn_002035.

Block layout, off phys_fn_004772 at 0x000b3aa0 (`mov edx,[eax]; add edx,[eax+4]`
then `inc dword ptr [eax+4]`) and phys_fn_004772's allocator at 0x000b3b6e
(`[esi]=0`, `[esi+0xc]=0`, `[esi+8]=size`, then `[esi+4]=0`):

	+0x00 data      +0x04 offset      +0x08 size      +0x0c next

Stream layout, only as far as the read path establishes it:

	+0x00 current block     read by phys_fn_004772/004774 as `[ecx]`
	+0x04 head block        `mov [ebx+4],[ebx]` at 0x000b3ba4
	+0x08 owned allocation  freed through the SDK allocator at 0x000b3bfb
	+0x0c owned allocation  freed through the SDK allocator at 0x000b3c19
	+0x10 last read address stored by both readers
	+0x14 two words         zeroed by the constructor at 0x000b3cf1/0x000b3cf5
	+0x18 bit mask          zeroed by EVERY raw read, 0x000b3aa2 and 0x000b3ac2
	+0x19 bit accumulator

WHAT IS NOT RECONSTRUCTED. The constructor at 0x000b3ce0 and the block allocator
at 0x000b3b50 take their blocks from the SDK allocator and the destructor at
0x000b3bf0 gives them back; the growable write side, the multi-block walk in
phys_fn_004768 (0x000b3a30) and the file-backed construction at 0x000b3d20 are
all outside what the asset differential drives. This reconstruction covers the
two read primitives and a stream over one caller-owned block, which is what the
penetration-map load path uses, and nothing else. The fields above whose only
established property is that the constructor zeroes them are named for their
offsets rather than guessed at.

NEITHER READER BOUNDS-CHECKS. phys_fn_004772 reads `data[offset]` and increments
`offset` with no comparison against `size` anywhere in its 23 bytes, so a
truncated payload does not fail, it reads past the block. That is reproduced
here: `mSize` is carried because the constructor stores it, not because anything
tests it.
*/

struct MemoryStreamBlock
	{
	const NxU8*			mData;
	NxU32				mOffset;
	NxU32				mSize;
	MemoryStreamBlock*	mNext;
	};

class MemoryStream
	{
	public:
	/**
	A stream over one caller-owned block. This is not phys_fn_004788: that
	constructor allocates its block through the SDK allocator. It is the shape
	the penetration-map load path needs and it is marked as such.
	*/
						MemoryStream(const void* buffer, NxU32 size);

	// phys_fn_004772 (0x000b3aa0), 23 bytes.
	NxU8				readByte();
	// phys_fn_004774 (0x000b3ac0), 24 bytes. `mov eax,[eax]` at 0x000b3ad5 --
	// a native load, so the format is little-endian by omission.
	NxU32				readDword();

	// The MSB-first bit stream phys_fn_002035 and phys_fn_002008 drive. The
	// refill, the test and the shift are inlined at 0x000501b2, 0x000501ca and
	// 0x000501d0 rather than being a row of their own; they are one function
	// here because the same eight instructions appear four times in the image.
	NxU32				readBit();
	NxU32				readBitsMsbFirst(NxU32 count);

	private:
	MemoryStreamBlock*	mCurrent;
	MemoryStreamBlock*	mHead;
	void*				mOwned08;
	void*				mOwned0C;
	const NxU8*			mLastAddress;
	NxU16				mWord14;
	NxU16				mWord16;
	NxU8				mBitMask;
	NxU8				mBitAccumulator;
	NxU8				mPad1A;
	NxU8				mPad1B;

	MemoryStreamBlock	mBlock;
	};

#endif
