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
The SDK-private block stream. It is NOT NxStream: NxStream is the public
abstract interface with a virtual destructor and thirteen pure virtuals, and
every access here is a direct call with no vtable in sight -- phys_fn_002047
reaches phys_fn_004772 and phys_fn_004774 by `call rel32`, and phys_fn_002162
reaches phys_fn_004797 the same way.

The class has no `__FILE__` literal anywhere in the image, so it has no
recovered name. It is called MemoryStream here for what it does.

The stream walks a singly linked list of blocks and keeps a one-byte MSB-first
bit accumulator beside the cursor, and PenetrationMap's payload decoder drives
the read half while the triangle-mesh writer drives the store half through the
same rows. sizeof is 0x1c, from `push 0x1c` at 0x00050184 in phys_fn_002035.

Block layout, off phys_fn_004772 at 0x000b3aa0 (`mov edx,[eax]; add edx,[eax+4]`
then `inc dword ptr [eax+4]`) and the block allocator phys_fn_004782 at
0x000b3b6e (`[esi]=0`, `[esi+0xc]=0`, `[esi+8]=size`, then `[esi+4]=0`):

	+0x00 data      +0x04 offset      +0x08 size      +0x0c next

Stream layout:

	+0x00 current block    written by the allocator 0x000b3b9c and appendBlock
	                       0x000b3a1c, read by every accessor as `[ecx]`
	+0x04 head block       `mov eax,[ebx]; mov [ebx+4],eax` at 0x000b3ba4/06
	+0x08 collapsed buffer installed by phys_fn_004795 at 0x000b3e97 and freed
	                       by the destructor at 0x000b3bf3..0x000b3c0b
	+0x0c second owned allocation, freed beside it at 0x000b3c12..0x000b3c29;
	                       nothing in this image is seen writing it
	+0x10 last address     stored by every raw read and raw store
	+0x14 two words        zeroed by both constructors (0x000b3cf1/0x000b3cf5,
	                       0x000b3d34/0x000b3d38); phys_fn_004795 clears the
	                       first again at 0x000b3eed
	+0x18 bit count        zeroed by EVERY raw read, 0x000b3aa2 and 0x000b3ac2
	+0x19 bit accumulator

THE CONSTRUCTOR IS ONE ROW AND ITS BLOCKS ARE SDK ALLOCATIONS.
phys_fn_004788 (0x000b3ce0) zeroes +0x08..+0x19 and calls the block allocator
phys_fn_004782 (0x000b3b50) -- thiscall, four stack args `(size, buffer, fill,
initialOffset)`, which is why it ends in `ret 0x10`. The allocator takes one
16-byte block and one `size`-byte data buffer from nxGetSdkAllocator, links
them as current and head, and then: a non-null `buffer` is copied in and the
block starts FULL (`offset = initialOffset`); a null `buffer` with non-zero
`fill` memsets the data to the BYTE 1 and also stores `initialOffset`; a null
`buffer` with zero `fill` starts empty at offset 0. Both known callers pass
`(size, buffer)` with fill 0 and initialOffset = size, so a stream over given
bytes is born full and is rewound with phys_fn_004780 seek(0), while a growable
store stream is born empty.

NEITHER READER BOUNDS-CHECKS. phys_fn_004772 reads `data[offset]` and increments
`offset` with no comparison against `size` anywhere in its 23 bytes, so a
truncated payload does not fail, it reads past the block. That is reproduced
here. seek is equally blunt: 0x000b3b38 compares the argument against the
CURRENT block's size only, so it rewinds within one block or does nothing.

THE STORE SIDE GROWS BY DOUBLING. phys_fn_004766 (0x000b39b0) takes the previous
tail block and appends one whose size is `prev->size << 1` (or the caller's size
when there is no previous), linking it through `prev->next` at 0x000b3a20 and
moving current to it. storeByte phys_fn_004770 and storeDword phys_fn_004797
flush pending bits first (phys_fn_004786), grow when `offset + n > size`, then
store through lastAddress. getLength phys_fn_004768 is the sum of every block's
OFFSET -- full blocks are exactly full because each grew only after the previous
one reached its size. collapse phys_fn_004795 frees the previous collapsed
buffer, sums the same walk into one allocation, copies each block's offset bytes
in order, and returns it; handed a non-null destination it copies into that
instead (0x000b3e9c onward) and never allocates. The writer's call passes 0 and
pushes the length as an unread second argument.
*/

struct MemoryStreamBlock
	{
	NxU8*				mData;
	NxU32				mOffset;
	NxU32				mSize;
	MemoryStreamBlock*	mNext;
	};

class MemoryStream
	{
	public:
	//! phys_fn_004788 (0x000b3ce0), over the block allocator phys_fn_004782.
	//! `fill` and `initialOffset` are the allocator's third and fourth
	//! arguments; every caller in this image leaves fill 0 and initialOffset
	//! = size, which are therefore the defaults here.
						MemoryStream(NxU32 size, const void* buffer,
							NxU32 fill = 0, NxU32 initialOffset = 0xffffffffu);
						~MemoryStream();	// phys_fn_004791 -> phys_fn_004784

	// phys_fn_004772 (0x000b3aa0), 23 bytes.
	NxU8				readByte();
	// phys_fn_004774 (0x000b3ac0), 24 bytes. A native load, so the format is
	// little-endian by omission.
	NxU32				readDword();

	// phys_fn_004780 (0x000b3b30), 29 bytes. Rewinds within the CURRENT block
	// only, and only strictly inside it.
	void				seek(NxU32 offset);

	// The MSB-first bit stream phys_fn_002035 and phys_fn_002008 drive on the
	// read side. The refill, the test and the shift are inlined at 0x000501b2,
	// 0x000501ca and 0x000501d0 rather than being a row of their own; they are
	// one function here because the same eight instructions appear four times
	// in the image.
	NxU32				readBit();
	NxU32				readBitsMsbFirst(NxU32 count);
	void				storeBit(NxU32 bit);
	void				storeBitsMsbFirst(NxU32 value, NxU32 count);

	// phys_fn_004770 (0x000b3a60), 63 bytes.
	void				storeByte(NxU8 value);
	// phys_fn_004797 (0x000b3f00), 66 bytes. Model::Save reaches this row by
	// direct call at 0x000e944e.
	void				storeDword(NxU32 value);
	// phys_fn_004786 (0x000b3c70), 105 bytes. Pads the partial byte with
	// zero bits by shifting the accumulator left until the count wraps.
	void				flushBits();

	// phys_fn_004768 (0x000b3a30), 37 bytes: the sum of every block's offset.
	NxU32				getLength() const;
	// phys_fn_004795 (0x000b3e30), 202 bytes. Null destination allocates the
	// collapsed copy and remembers it at +0x08; a non-null destination is
	// filled and returned, and owns nothing.
	void*				collapse(void* destination);

	private:
	bool				appendBlock(MemoryStreamBlock* prev, NxU32 size);
							// phys_fn_004766 (0x000b39b0)
	bool				initBlock(NxU32 size, const void* buffer, NxU32 fill,
							NxU32 initialOffset);
							// phys_fn_004782 (0x000b3b50)

	private:
	MemoryStreamBlock*	mCurrent;
	MemoryStreamBlock*	mHead;
	NxU8*				mOwned08;
	NxU8*				mOwned0C;
	NxU8*				mLastAddress;
	NxU16				mWord14;
	NxU16				mWord16;
	NxU8				mBitMask;
	NxU8				mBitAccumulator;
	NxU8				mPad1A;
	NxU8				mPad1B;
	};

#endif
