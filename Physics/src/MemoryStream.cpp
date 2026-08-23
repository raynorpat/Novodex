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

// The block is four words and both halves index it as {data, offset, size,
// next}: 0x000b3aa6 loads [eax], 0x000b3aa8 adds [eax+4], 0x000b3aae
// increments [eax+4]; the allocator at 0x000b3b6e stores size into [esi+8]
// and clears [esi+0xc] before [esi+4].
static_assert(sizeof(MemoryStreamBlock) == 0x10, "the stream block is sixteen bytes");
static_assert(offsetof(MemoryStreamBlock, mOffset) == 4, "the offset follows the data pointer");
static_assert(offsetof(MemoryStreamBlock, mSize) == 8, "the size follows the offset");
static_assert(offsetof(MemoryStreamBlock, mNext) == 0xc, "the link is the last word");

// sizeof(MemoryStream) is 0x1c in the image -- `push 0x1c` at 0x00050184 is
// the allocation phys_fn_002035 makes when it builds its own stream. The two
// padding bytes after the accumulator are what make that come out.
static_assert(sizeof(MemoryStream) == 0x1c, "the stream object is twenty-eight bytes");

// ---------------------------------------------------------------------------
// Construction and destruction.

// phys_fn_004782 (0x000b3b50). One 16-byte block and one `size` data buffer,
// both through nxGetSdkAllocator -- slot 0 with (size, 0) at 0x000b3b62 and
// 0x000b3b8d. A non-null buffer is copied in (rep movsd / rep movsb at
// 0x000b3be1/0x000b3be8) and the block starts full; a null buffer with a
// non-zero fill memsets the BYTE 1 over the data at 0x000b3bb3..0x000b3bbf;
// otherwise the block starts empty. `initialOffset` reaches [block+4] only on
// those two arms (0x000b3bc8), so an empty stream starts at offset 0 whatever
// the caller passed.
bool MemoryStream::initBlock(NxU32 size, const void* buffer, NxU32 fill, NxU32 initialOffset)
	{
	SdkAllocator* allocator = nxGetSdkAllocator();
	MemoryStreamBlock* block = static_cast<MemoryStreamBlock*>(allocator->malloc(sizeof(MemoryStreamBlock), NX_MEMORY_PERSISTENT));
	if(!block)
		{
		// The image's failure path falls into its own tail with the object
		// fields uninitialised and would copy or fill through whatever was
		// there; that is unreachable undefined behaviour rather than a
		// behaviour to reproduce, so the reconstruction simply declines.
		return false;
		}
	block->mData = 0;							// 0x000b3b6e
	block->mNext = 0;							// 0x000b3b74
	block->mSize = size;						// 0x000b3b7b
	NxU8* data = static_cast<NxU8*>(allocator->malloc(size, NX_MEMORY_PERSISTENT));	// 0x000b3b8d
	block->mData = data;						// 0x000b3b91
	if(!data)
		return false;	// the block itself is not freed here in the image either
	block->mOffset = 0;							// 0x000b3b95
	mCurrent = block;							// 0x000b3b9c
	mHead = block;								// 0x000b3ba6
	if(buffer)
		{
		memcpy(data, buffer, size);				// 0x000b3bd8..0x000b3be8
		block->mOffset = initialOffset;			// 0x000b3bc8
		}
	else if(fill)
		{
		memset(data, 1, size);					// 0x000b3bb3..0x000b3bbf, value 1
		block->mOffset = initialOffset;			// 0x000b3bc8
		}
	mLastAddress = block->mData;				// 0x000b3bd1
	return true;
	}

// phys_fn_004788 (0x000b3ce0).
MemoryStream::MemoryStream(NxU32 size, const void* buffer, NxU32 fill, NxU32 initialOffset)
	{
	mOwned08 = 0;			// 0x000b3ceb
	mOwned0C = 0;			// 0x000b3cee
	mWord14 = 0;			// 0x000b3cf1
	mWord16 = 0;			// 0x000b3cf5
	mBitMask = 0;			// 0x000b3cf9
	mBitAccumulator = 0;	// 0x000b3cfc
	mPad1A = 0;
	mPad1B = 0;

	initBlock(size, buffer, fill, initialOffset);	// 0x000b3d07, four stack args
	}

// phys_fn_004791 is a five-byte jmp to phys_fn_004784 (0x000b3bf0): the two
// owned allocations first, then every block's data and the block itself.
MemoryStream::~MemoryStream()
	{
	SdkAllocator* allocator = nxGetSdkAllocator();
	if(mOwned08)								// 0x000b3bf3
		{
		allocator->free(mOwned08);				// 0x000b3c08
		mOwned08 = 0;							// 0x000b3c0b
		}
	if(mOwned0C)								// 0x000b3c12
		{
		allocator->free(mOwned0C);				// 0x000b3c26
		mOwned0C = 0;							// 0x000b3c29
		}
	for(MemoryStreamBlock* block = mHead; block;)
		{
		MemoryStreamBlock* next = block->mNext;	// 0x000b3c3d
		if(block->mData)						// 0x000b3c3a
			allocator->free(block->mData);		// 0x000b3c4e
		allocator->free(block);					// 0x000b3c61
		block = next;							// 0x000b3c66
		}
	}

// ---------------------------------------------------------------------------
// The read half.

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

// phys_fn_004780 (0x000b3b30). `cmp edx,[ecx+8]; jae` at 0x000b3b38: only a
// strictly in-bounds offset moves anything, and only within the current block.
void MemoryStream::seek(NxU32 offset)
	{
	if(offset < mCurrent->mSize)
		{
		mCurrent->mOffset = offset;
		mLastAddress = mCurrent->mData + mCurrent->mOffset;
		}
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

// ---------------------------------------------------------------------------
// The store half.

// phys_fn_004766 (0x000b39b0). The new block doubles the previous one's size
// (`mov eax,[edi+8]; shl eax,1` at 0x000b3df..0x000b3e2) unless there is no
// previous block, in which case it is the caller's size argument.
bool MemoryStream::appendBlock(MemoryStreamBlock* prev, NxU32 size)
	{
	SdkAllocator* allocator = nxGetSdkAllocator();
	MemoryStreamBlock* block = static_cast<MemoryStreamBlock*>(allocator->malloc(sizeof(MemoryStreamBlock), NX_MEMORY_PERSISTENT));	// 0x000b39c1
	if(!block)
		return false;							// 0x000b39e6, al = 0
	block->mData = 0;							// 0x000b39d0
	block->mNext = 0;							// 0x000b39d6
	NxU32 newSize = prev ? prev->mSize << 1 : size;	// 0x000b39df..0x000b39f1
	block->mSize = newSize;
	NxU8* data = static_cast<NxU8*>(allocator->malloc(newSize, NX_MEMORY_PERSISTENT));	// 0x000b3a03
	block->mData = data;						// 0x000b3a07
	if(!data)
		return false;							// 0x000b3a09, al = 0
	block->mOffset = 0;							// 0x000b3a15
	mCurrent = block;							// 0x000b3a1c
	if(prev)
		prev->mNext = block;					// 0x000b3a20
	return true;
	}

// phys_fn_004786 (0x000b3c70). Shifts the accumulator left once per pending
// bit until the count wraps at eight, then stores the byte through the same
// grow-then-write shape the raw stores use. The self-call at 0x000b3c9e is in
// the image and re-enters with the count already cleared, which returns at
// once; the recursion is kept because the source had it.
void MemoryStream::flushBits()
	{
	if(mBitMask == 0)							// 0x000b3c73
		return;
	while(mBitMask != 0)						// loop head 0x000b3c80
		{
		mBitAccumulator = static_cast<NxU8>(mBitAccumulator << 1);	// shl dl,1
		mBitMask = static_cast<NxU8>(mBitMask + 1);					// inc cl
		if(mBitMask == 8)
			{
			mBitMask = 0;						// 0x000b3c9a
			flushBits();						// 0x000b3c9e, returns immediately
			if(mCurrent->mOffset + 1 > mCurrent->mSize)	// 0x000b3cac
				appendBlock(mCurrent, 0);		// 0x000b3cb5
			mLastAddress = mCurrent->mData + mCurrent->mOffset;	// 0x000b3cbf
			mCurrent->mOffset += 1;				// 0x000b3cc4
			*mLastAddress = mBitAccumulator;	// 0x000b3cca
			}
		}
	}

// phys_fn_004770 (0x000b3a60).
void MemoryStream::storeByte(NxU8 value)
	{
	flushBits();								// 0x000b3a64
	if(mCurrent->mOffset + 1 > mCurrent->mSize)	// 0x000b3a71
		appendBlock(mCurrent, 0);				// 0x000b3a7b, growth by doubling
	mLastAddress = mCurrent->mData + mCurrent->mOffset;	// 0x000b3a87
	mCurrent->mOffset += 1;						// 0x000b3a8c
	*mLastAddress = value;						// 0x000b3a96
	}

// phys_fn_004797 (0x000b3f00).
void MemoryStream::storeDword(NxU32 value)
	{
	flushBits();								// 0x000b3f04
	if(mCurrent->mOffset + 4 > mCurrent->mSize)	// 0x000b3f11
		appendBlock(mCurrent, 0);				// 0x000b3f1d
	mLastAddress = mCurrent->mData + mCurrent->mOffset;	// 0x000b3f27
	mCurrent->mOffset += 4;						// 0x000b3f2e
	memcpy(mLastAddress, &value, sizeof(value));	// 0x000b3f39, a native dword store
	}

// phys_fn_004768 (0x000b3a30). Every non-last block is exactly full -- each
// grew only after the previous reached its size -- so the sum of the offsets
// is the number of bytes stored.
NxU32 MemoryStream::getLength() const
	{
	NxU32 total = 0;
	for(const MemoryStreamBlock* block = mHead; block; block = block->mNext)
		total += block->mOffset;				// 0x000b3a48 and 0x000b3a4f..52
	return total;
	}

// phys_fn_004795 (0x000b3e30). Null destination: free the previous collapsed
// buffer (0x000b3e46..0x000b3e5d), sum the walk, allocate it (0x000b3e91),
// remember it at +0x08 (0x000b3e97). Either way copy each block's OFFSET
// bytes in order (the loops at 0x000b3eb0..0x000b3eeb) and clear +0x14
// (0x000b3eed).
void* MemoryStream::collapse(void* destination)
	{
	if(!destination)
		{
		if(mOwned08)
			{
			nxGetSdkAllocator()->free(mOwned08);
			mOwned08 = 0;
			}
		NxU32 total = getLength();				// the identical walk, inlined at 0x000b3e64
		mOwned08 = total ? static_cast<NxU8*>(nxGetSdkAllocator()->malloc(total, NX_MEMORY_PERSISTENT)) : 0;
		destination = mOwned08;
		if(!destination)
			return 0;
		}
	NxU8* cursor = static_cast<NxU8*>(destination);
	for(MemoryStreamBlock* block = mHead; block; block = block->mNext)
		{
		memcpy(cursor, block->mData, block->mOffset);
		cursor += block->mOffset;
		}
	mWord14 = 0;
	return destination;
	}
