/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "MemoryStream.h"

#include <stddef.h>
#include <string.h>

// The block is four words and the two readers index it as {data, offset, size,
// next}: 0x000b3aa6 loads [eax], 0x000b3aa8 adds [eax+4], 0x000b3aae increments
// [eax+4]; 0x000b3b7b stores the size into [esi+8] and 0x000b3b74 clears
// [esi+0xc] before the list is linked.
static_assert(sizeof(MemoryStreamBlock) == 0x10, "the stream block is sixteen bytes");
static_assert(offsetof(MemoryStreamBlock, mOffset) == 4, "the offset follows the data pointer");
static_assert(offsetof(MemoryStreamBlock, mSize) == 8, "the size follows the offset");
static_assert(offsetof(MemoryStreamBlock, mNext) == 0xc, "the link is the last word");

// sizeof(MemoryStream) is 0x1c in the image -- `push 0x1c` at 0x00050184 is the
// allocation phys_fn_002035 makes when it has to build its own stream. This
// reconstruction carries the caller-owned block inline instead of taking one
// from the SDK allocator, so its own sizeof is larger and is not asserted
// against the oracle's; what the header records is where each field sits in the
// oracle, and nothing here reads the object at those offsets.

MemoryStream::MemoryStream(const void* buffer, NxU32 size)
	{
	mBlock.mData = static_cast<const NxU8*>(buffer);
	mBlock.mOffset = 0;
	mBlock.mSize = size;
	mBlock.mNext = 0;

	mCurrent = &mBlock;
	mHead = &mBlock;
	mOwned08 = 0;
	mOwned0C = 0;
	mLastAddress = 0;
	mWord14 = 0;
	mWord16 = 0;
	// The constructor at 0x000b3cf9/0x000b3cfc clears both accumulator bytes, so
	// a fresh stream starts on a byte boundary.
	mBitMask = 0;
	mBitAccumulator = 0;
	mPad1A = 0;
	mPad1B = 0;
	}

// 0x000b3aa2 is the SECOND instruction of the row: every raw byte read discards
// whatever bits were left in the accumulator. A bit stream and a byte stream
// therefore cannot be interleaved, and this line is the whole of that rule.
NxU8 MemoryStream::readByte()
	{
	mBitMask = 0;
	mLastAddress = mCurrent->mData + mCurrent->mOffset;
	mCurrent->mOffset += 1;
	return *mLastAddress;
	}

NxU32 MemoryStream::readDword()
	{
	mBitMask = 0;
	mLastAddress = mCurrent->mData + mCurrent->mOffset;
	mCurrent->mOffset += 4;
	NxU32 value;
	memcpy(&value, mLastAddress, sizeof(value));
	return value;
	}

// Refill when the mask is exhausted (`test [esi+0x18]` at 0x000501b2, the read
// at 0x000501bb and `mov byte ptr [esi+0x18],0x80` at 0x000501c3), test the top
// remaining bit (`test [esi+0x19],al` at 0x000501ca) and shift the mask down
// (`shr al,1` at 0x000501d0). MSB first inside each byte.
NxU32 MemoryStream::readBit()
	{
	if(mBitMask == 0)
		{
		mBitAccumulator = readByte();
		mBitMask = 0x80;
		}
	NxU32 bit = (mBitAccumulator & mBitMask) != 0 ? 1u : 0u;
	mBitMask = static_cast<NxU8>(mBitMask >> 1);
	return bit;
	}

// The 32-bit form at 0x000501e0..0x00050208 and the identical loop at
// 0x0004dbd5..0x0004dc00: shift the accumulator left, OR the next bit in.
NxU32 MemoryStream::readBitsMsbFirst(NxU32 count)
	{
	NxU32 value = 0;
	for(NxU32 i = 0; i < count; ++i)
		value = (value << 1) | readBit();
	return value;
	}
