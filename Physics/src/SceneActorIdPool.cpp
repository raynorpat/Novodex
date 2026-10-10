#include "NxSceneActorIdPool.h"
#include "FoundationSDK.h"
#include <new>

NxSceneActorIdPool::NxSceneActorIdPool()
    : mNext(0),mBegin(nullptr),mEnd(nullptr),mCapacity(nullptr) {}
NxSceneActorIdPool::~NxSceneActorIdPool() { releaseBuffer(); }

NxU32 NxSceneActorIdPool::take()
{
    if(mEnd!=mBegin) return *--mEnd;
    return mNext++; // The helper's unsigned word wraps; no uniqueness guarantee.
}

void NxSceneActorIdPool::returnId(NxU32 id)
{
    if(mEnd==mCapacity) {
        const NxU32 count=mBegin?static_cast<NxU32>(mEnd-mBegin):0;
        const NxU32 next=count*2u+2u;
        NxU32* grown=static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(
            next*sizeof(NxU32),NX_MEMORY_PERSISTENT));
        // Default initialization starts actual scalar element lifetimes but
        // leaves the original unobserved capacity tail unchanged.
        ::new (static_cast<void*>(grown)) NxU32[next];
        for(NxU32 i=0;i<count;++i) grown[i]=mBegin[i];
        if(mBegin) nxFoundationSDKAllocator->free(mBegin);
        mBegin=grown;
        mCapacity=grown+next;
        mEnd=grown+count;
    }
    *mEnd++=id;
}

void NxSceneActorIdPool::releaseBuffer()
{
    if(mBegin) nxFoundationSDKAllocator->free(mBegin);
    mBegin=nullptr;
    mEnd=nullptr;
    mCapacity=nullptr;
}

NxSceneActorIdPool* nxSceneActorIdPoolConstruct(void* storage)
{ return ::new (storage) NxSceneActorIdPool; }
void nxSceneActorIdPoolDestroy(NxSceneActorIdPool* pool)
{ pool->~NxSceneActorIdPool(); }
