/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Containers.h"

#include <stddef.h>
#include <string.h>

// 0x000b4d7c stores 0x40000000 in the fourth word and the three before it are
// cleared, so the header is sixteen bytes with the factor last.
static_assert(sizeof(SdkContainer) == 0x10, "SdkContainer is 16 bytes in the oracle");
static_assert(offsetof(SdkContainer, mCount) == 4, "the count follows the capacity");
static_assert(offsetof(SdkContainer, mEntries) == 8, "the entries follow the count");
static_assert(offsetof(SdkContainer, mGrowthFactor) == 0xc, "the growth factor is the last word");

SdkContainer::SdkContainer()
	{
	mCapacity = 0;
	mCount = 0;
	mEntries = 0;
	mGrowthFactor = 2.0f;
	}

// 0x000b4de4 compares the factor against the 0.0f at .rdata 0x001041f0 and
// 0x000b4df4 returns false when it is not greater, before touching anything.
//
// The new capacity is capacity * factor through _ftol, or a literal 2 when the
// capacity is zero (0x000b4e1b), clamped up to count + needed. It is stored at
// 0x000b4e2b, BEFORE the allocation, so a failed allocation leaves the capacity
// already grown and the entries pointer untouched. That ordering is reproduced.
//
// The block comes from slot +0 of the SDK allocator with a memory type of 0, the
// live entries are copied with rep movsd, and the old block is released through
// slot +0xc. The pointer is cleared at 0x000b4e7c and rewritten at 0x000b4e83.
bool SdkContainer::resize(NxU32 needed)
	{
	if(!(mGrowthFactor > 0.0f))
		return false;

	NxU32 capacity = mCapacity ? static_cast<NxU32>(static_cast<NxF32>(mCapacity) * mGrowthFactor) : 2;
	if(capacity < mCount + needed)
		capacity = mCount + needed;
	mCapacity = capacity;

	NxU32* entries = static_cast<NxU32*>(nxGetSdkAllocator()->malloc(mCapacity * sizeof(NxU32), NX_MEMORY_PERSISTENT));
	if(!entries)
		return false;

	if(mCount)
		memcpy(entries, mEntries, mCount * sizeof(NxU32));

	if(mEntries)
		{
		nxGetSdkAllocator()->free(mEntries);
		mEntries = 0;
		}

	mEntries = entries;
	return true;
	}

// 0x000b4f5e tests only the sign of the factor, so a negative one -- the not
// owned marker -- keeps the buffer. The capacity and count are cleared either
// way, at 0x000b4f81 and 0x000b4f87, outside the branch.
void SdkContainer::empty()
	{
	if(mGrowthFactor >= 0.0f && mEntries)
		{
		nxGetSdkAllocator()->free(mEntries);
		mEntries = 0;
		}

	mCapacity = 0;
	mCount = 0;
	}

// The same release, then the arguments are installed in the order the oracle
// writes them -- count at 0x000b4fc9, capacity at 0x000b4fd0, entries at
// 0x000b4fd2 -- and the factor becomes the 0xbf800000 stored at 0x000b4fd5.
void SdkContainer::setExternalBuffer(NxU32 capacity, NxU32* entries)
	{
	if(mGrowthFactor >= 0.0f && mEntries)
		{
		nxGetSdkAllocator()->free(mEntries);
		mEntries = 0;
		}

	mCount = 0;
	mCapacity = capacity;
	mEntries = entries;
	mGrowthFactor = -1.0f;
	}
