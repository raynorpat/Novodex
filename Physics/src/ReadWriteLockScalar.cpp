#include "PhysicsInternal.h"
#include "ReadWriteLockLifetime.h"
#include "NpSceneGuard.h"
#include <new>
#include <stddef.h>
#if NX_PHYSICS_USE_X87
#error Scalar lock lifetime must not enter the x87 source selection
#endif

// Actual original 5b6a0 state. Only owned is initialized before Windows init;
// the owner ID remains uninitialized until an acquisition stores it.
struct ReadWriteLockData
{
    CRITICAL_SECTION mCriticalSection;
    LONG mOwned;
    DWORD mOwnerThreadId;
    ReadWriteLockData():mOwned(0) { InitializeCriticalSection(&mCriticalSection); }
    ~ReadWriteLockData() { DeleteCriticalSection(&mCriticalSection); }
};
static_assert(sizeof(void*)==4,"original lock link pointer is fixed32");
static_assert(sizeof(ReadWriteLock)==4 && alignof(ReadWriteLock)==4,"actual four-byte aligned lock link");
static_assert(sizeof(ReadWriteLockData)==0x20 && alignof(ReadWriteLockData)==4,"original lock state size and alignment");
static_assert(offsetof(ReadWriteLockData,mCriticalSection)==0,"original critical section at zero");
static_assert(offsetof(ReadWriteLockData,mOwned)==0x18,"original owned flag offset");
static_assert(offsetof(ReadWriteLockData,mOwnerThreadId)==0x1c,"original owner ID offset");

ReadWriteLock::ReadWriteLock()
{
    static_assert(offsetof(ReadWriteLock,mData)==0,"original backing state link at zero");
    void* memory=nxFoundationSDKAllocator->malloc(sizeof(ReadWriteLockData),NX_MEMORY_PERSISTENT);
    mData=new (memory) ReadWriteLockData;
}
ReadWriteLock::~ReadWriteLock()
{
    static_cast<ReadWriteLockData*>(mData)->~ReadWriteLockData();
    if(mData) nxFoundationSDKAllocator->free(mData);
    mData=0;
}
#include "ReadWriteLockMethods.inl"
ReadWriteLock* nxSceneLockCreate()
{
    void* memory=nxFoundationSDKAllocator->malloc(sizeof(ReadWriteLock),NX_MEMORY_PERSISTENT);
    return memory ? new (memory) ReadWriteLock : 0;
}
void nxSceneLockDestroy(void* link)
{
    if(!link) return;
    static_cast<ReadWriteLock*>(link)->~ReadWriteLock();
    nxFoundationSDKAllocator->free(link);
}
void nxNpSceneGuardEnter(void* link) { static_cast<ReadWriteLock*>(link)->lock(); }
bool nxNpSceneGuardWriteTry(void* link) { return static_cast<ReadWriteLock*>(link)->tryLock(); }
void nxNpSceneGuardLeave(void* link) { static_cast<ReadWriteLock*>(link)->unlock(); }
