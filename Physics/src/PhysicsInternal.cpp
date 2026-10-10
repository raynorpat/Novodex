/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"
#include "ContactGeneration.h"
#include "NarrowPhase.h"
#include "NxArray.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>

void __cdecl NxContactSphereMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
void __cdecl NxContactMeshMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
void __cdecl NxContactCapsuleMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
bool __cdecl NxOverlapPlaneMesh(const NxCollisionShape*, const NxCollisionShape*, void*);
extern "C" bool __cdecl nxOverlapMeshMesh(const NxCollisionShape*, const NxCollisionShape*, void*);
bool __cdecl NxOverlapCapsuleMesh(const NxCollisionShape*, const NxCollisionShape*, void*);

#if NX_PHYSICS_USE_X87
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

#include "ReadWriteLockMethods.inl"
#endif

// 0x0000e733 (in the SDK constructor) pushes 0x124 as the allocation size, and
// phys_fn_002338 writes words 1..0x48 of the block.
static_assert(sizeof(ShapePairFunctionTable) == 0x124, "ShapePairFunctionTable is 292 bytes in the oracle");
static_assert(offsetof(ShapePairFunctionTable, mFunction) == 4, "the table follows the vtable pointer");

ShapePairFunctionTable::ShapePairFunctionTable()
	{
	// phys_fn_002338 clears both 36 word blocks and then writes the entries
	// that have a handler.
	for(unsigned block = 0; block < 2; block++)
		for(unsigned i = 0; i < 6; i++)
			for(unsigned j = 0; j < 6; j++)
				mFunction[block][i][j] = 0;
	// Task 2m: the shared [*][COMPOUND] matrix-A expander and the
	// [COMPOUND][COMPOUND] child-pair walk.
	for(unsigned type = 0; type < 5; ++type)
		mFunction[0][type][5] = (void*) NxContactCompoundShape;
	mFunction[0][5][5] = (void*) NxContactCompoundCompound;
	// phys_fn_002338's primitive contact slots are the kernels reconstructed in
	// ContactGeneration.cpp. Keep the upper-triangle indexing used by 002348.
	mFunction[0][0][1] = (void*) NxContactPlaneSphere;
	mFunction[0][0][2] = (void*) NxContactPlaneBox;
	mFunction[0][0][3] = (void*) NxContactPlaneCapsule;
	mFunction[0][0][4] = (void*) NxContactPlaneMesh;
	mFunction[0][1][1] = (void*) NxContactSphereSphere;
	mFunction[0][1][2] = (void*) NxContactSphereBox;
	mFunction[0][1][3] = (void*) NxContactSphereCapsule;
	mFunction[0][1][4] = (void*) NxContactSphereMesh;
	mFunction[0][2][2] = (void*) NxContactBoxBox;
	mFunction[0][2][3] = (void*) NxContactBoxCapsule;
	mFunction[0][3][3] = (void*) NxContactCapsuleCapsule;
	mFunction[0][3][4] = (void*) NxContactCapsuleMesh;
	mFunction[0][4][4] = (void*) NxContactMeshMesh;
	// Trigger interactions use the second matrix. The sphere/mesh overlap row
	// discovers mesh trigger pairs during simulation; plane/mesh uses the
	// recovered boolean overlap kernel as well. Primitive pairs share the same
	// dispatch path when either shape is a trigger; keep these slots populated
	// so a box trigger and moving sphere can produce pair transitions.
	mFunction[1][1][1] = (void*) NxOverlapSphereSphere;
	mFunction[1][1][2] = (void*) NxOverlapSphereBox;
	mFunction[1][1][3] = (void*) NxOverlapSphereCapsule;
	mFunction[1][2][2] = (void*) NxOverlapBoxBox;
	mFunction[1][2][3] = (void*) NxOverlapBoxCapsule;
	mFunction[1][3][3] = (void*) NxOverlapCapsuleCapsule;
	mFunction[1][3][4] = (void*) NxOverlapCapsuleMesh;
	mFunction[1][0][4] = (void*) NxOverlapPlaneMesh;
	mFunction[1][1][4] = (void*) NxOverlapSphereMesh;
	mFunction[1][4][4] = (void*) nxOverlapMeshMesh;
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

#include "NxSdkAllocatorAccess.inl"

// .data 0x00123c0c. The oracle allocates 0x10 bytes and clears three words,
// which is exactly what NX_NEW of an NxArraySDK does, and releases it with the
// destructor plus operator delete that phys_fn_000474 is.
static NxArraySDK<SdkPointerPair>* gPointerBindings = 0;

// phys_fn_000454 (0x0000df90). A linear scan; a miss and a value of zero are the
// same answer, and neither is reported.
void* nxGetSdkPointerBinding(void* key)
	{
	if(gPointerBindings && key)
		{
		NxU32 count = gPointerBindings->size();
		SdkPointerPair* pair = gPointerBindings->begin();
		for(NxU32 i = 0; i < count; i++, pair++)
			if(pair->key == key)
				return pair->value;
		}
	return 0;
	}

// phys_fn_000480 (0x0000edc0). A null key is the only rejection. A null value
// removes the binding, and removing the last one destroys the table outright
// rather than leaving an empty one -- 0x0000ee98 tests the remaining byte count
// and 0x0000eeac clears the pointer.
//
// The table is created only when a value is being stored, and the creation is
// NOT checked: if the allocation fails the oracle carries straight on into the
// scan and faults on the null. That is reproduced.
bool nxSetSdkPointerBinding(void* key, void* value)
	{
	if(!key)
		return false;

	if(!value)
		{
		if(!gPointerBindings)
			return true;
		}
	else if(!gPointerBindings)
		gPointerBindings = NX_NEW(NxArraySDK<SdkPointerPair>)();

	NxU32 count = gPointerBindings->size();
	for(NxU32 i = 0; i < count; i++)
		{
		if((*gPointerBindings)[i].key != key)
			continue;

		if(value)
			{
			(*gPointerBindings)[i].value = value;
			return true;
			}

		gPointerBindings->replaceWithLast(i);
		if(gPointerBindings->size() == 0)
			{
			NX_DELETE_SINGLE(gPointerBindings);
			gPointerBindings = 0;
			}
		return true;
		}

	SdkPointerPair pair;
	pair.key = key;
	pair.value = value;
	gPointerBindings->pushBack(pair);
	return true;
	}

void nxReleaseSdkPointerBindings()
	{
	if(!gPointerBindings)
		return;
	NX_DELETE_SINGLE(gPointerBindings);
	gPointerBindings = 0;
	}
