/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stddef.h>

// The heap block phys_fn_002358 allocates. The critical section starts at zero,
// the interlocked owner flag is the word at +0x18 the constructor clears, and
// the owning thread id is the word at +0x1c that only the lock entry points
// write, so the constructor leaves it alone.
struct ReadWriteLockData
	{
	CRITICAL_SECTION mCriticalSection;
	long mOwned;
	unsigned long mOwnerThreadId;
	};

// 0x0005b6ae pushes 0x20 as the allocation size, 0x0005b6b5 clears +0x18, and
// 0x0005b724 stores the thread id at +0x1c.
static_assert(sizeof(ReadWriteLockData) == 0x20, "ReadWriteLockData is 32 bytes in the oracle");
static_assert(offsetof(ReadWriteLockData, mOwned) == 0x18, "owner flag is at +0x18 in the oracle");
static_assert(offsetof(ReadWriteLockData, mOwnerThreadId) == 0x1c, "owner thread id is at +0x1c in the oracle");
// NpPhysicsSDK is 12 bytes with the lock as its third word, so the lock is one word.
static_assert(sizeof(ReadWriteLock) == 4, "ReadWriteLock is one word in the oracle");

ReadWriteLock::ReadWriteLock()
	{
	mData = NX_ALLOC(sizeof(ReadWriteLockData));
	ReadWriteLockData* data = static_cast<ReadWriteLockData*>(mData);
	data->mOwned = 0;
	InitializeCriticalSection(&data->mCriticalSection);
	}

ReadWriteLock::~ReadWriteLock()
	{
	ReadWriteLockData* data = static_cast<ReadWriteLockData*>(mData);
	DeleteCriticalSection(&data->mCriticalSection);
	NX_FREE(mData);
	}

// 0x0000e9a5 (in the SDK constructor) pushes 0x124 as the allocation size, and
// phys_fn_002338 writes words 1..0x48 of the block.
static_assert(sizeof(ShapePairFunctionTable) == 0x124, "ShapePairFunctionTable is 292 bytes in the oracle");
static_assert(offsetof(ShapePairFunctionTable, mFunction) == 4, "the table follows the vtable pointer");

ShapePairFunctionTable::ShapePairFunctionTable()
	{
	// phys_fn_002338 clears both 36 word blocks and then writes the entries
	// that have a handler. Every handler it writes is Phase 3 collision code,
	// so this component leaves the table cleared.
	for(unsigned block = 0; block < 2; block++)
		for(unsigned i = 0; i < 6; i++)
			for(unsigned j = 0; j < 6; j++)
				mFunction[block][i][j] = 0;
	}

ShapePairFunctionTable::~ShapePairFunctionTable()
	{
	}

// 0x0000fb14 stores the caller's allocator at .data 0x001220ec, four bytes past
// the object 0x0000fb0f passes to phys_fn_004805.
static_assert(sizeof(SdkAllocatorBridge) == 8, "SdkAllocatorBridge is two words in the oracle");
static_assert(offsetof(SdkAllocatorBridge, mAllocator) == 4, "the allocator follows the vtable pointer");

void* SdkAllocatorBridge::malloc(size_t size, NxMemoryType type)
	{
	return mAllocator->malloc(size, type ? NX_MEMORY_TEMP : NX_MEMORY_PERSISTENT);
	}

void* SdkAllocatorBridge::mallocDEBUG(size_t size, const char* fileName, int line, const char* className, NxMemoryType type)
	{
	return mAllocator->mallocDEBUG(size, fileName, line, className, type ? NX_MEMORY_TEMP : NX_MEMORY_PERSISTENT);
	}

void* SdkAllocatorBridge::realloc(void* memory, size_t size)
	{
	return mAllocator->realloc(memory, size);
	}

void SdkAllocatorBridge::free(void* memory)
	{
	mAllocator->free(memory);
	}

static SdkAllocatorBridge* gSdkAllocatorBridge = 0;

bool nxSetSdkAllocatorBridge(SdkAllocatorBridge* bridge)
	{
	gSdkAllocatorBridge = bridge;
	return true;
	}
