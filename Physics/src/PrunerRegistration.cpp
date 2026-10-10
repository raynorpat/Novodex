#include "NxPhysicsBackend.h"
#if !NX_PHYSICS_USE_X87
#include "NxPrunerRegistration.h"
#include <new>
#include <string.h>
// Actual .data1012846c singleton, owned through the genuine SDK allocator.
static NxPrunerProcessPool* gNxPrunerProcessPool=nullptr;
template<class T> static T* nxRegistryAllocate(NxU32 count) {
    T* memory=static_cast<T*>(nxGetSdkAllocator()->malloc(sizeof(T)*count,NX_MEMORY_PERSISTENT));
    // Begin trivial element lifetimes without zeroing unobserved object tails.
    // Like the image, later buffer OOM is unchecked and unsupported.
    for(NxU32 i=0;i<count;++i) ::new(static_cast<void*>(memory+i)) T;
    return memory;
}
NxPrunerProcessPool::NxPrunerProcessPool() {
    mCount=0;mFreeCount=0;mCapacity=2;
    mObjects=nxRegistryAllocate<NxPrunerRegistration*>(mCapacity);
    mForward=nxRegistryAllocate<NxU16>(mCapacity);
    mReverse=nxRegistryAllocate<NxU16>(mCapacity);
    mGenerations=nxRegistryAllocate<NxU16>(mCapacity);
    memset(mForward,0xff,mCapacity*2);memset(mReverse,0xff,mCapacity*2);memset(mGenerations,0,mCapacity*2);
}
void NxPrunerProcessPool::replace(NxPrunerRegistration** objects,NxU16* forward,NxU16* reverse,NxU16* generations) {
    if(mGenerations) { nxGetSdkAllocator()->free(mGenerations);mGenerations=nullptr; }
    if(mReverse) { nxGetSdkAllocator()->free(mReverse);mReverse=nullptr; }
    if(mForward) { nxGetSdkAllocator()->free(mForward);mForward=nullptr; }
    if(mObjects) { nxGetSdkAllocator()->free(mObjects);mObjects=nullptr; }
    mObjects=objects;mForward=forward;mReverse=reverse;mGenerations=generations;
}
NxPrunerProcessPool::~NxPrunerProcessPool() { replace(nullptr,nullptr,nullptr,nullptr); }
NxU32 NxPrunerProcessPool::add(NxPrunerRegistration* member) {
    if(mFreeCount) {
        const NxU16 index=mReverse[mCount];mObjects[mCount]=member;mForward[index]=NxU16(mCount);
        ++mCount;--mFreeCount;return NxU32(index)|(NxU32(mGenerations[index])<<16);
    }
    if(mCount==mCapacity) {
        mCapacity+=mCapacity;if(mCapacity>0xffffu) mCapacity=0xffffu;
        NxPrunerRegistration** objects=nxRegistryAllocate<NxPrunerRegistration*>(mCapacity);
        NxU16* forward=nxRegistryAllocate<NxU16>(mCapacity);
        NxU16* reverse=nxRegistryAllocate<NxU16>(mCapacity);
        NxU16* generations=nxRegistryAllocate<NxU16>(mCapacity);
        memcpy(objects,mObjects,mCount*4);memcpy(forward,mForward,mCount*2);memcpy(reverse,mReverse,mCount*2);memcpy(generations,mGenerations,mCount*2);
        memset(forward+mCount,0xff,(mCapacity-mCount)*2);memset(reverse+mCount,0xff,(mCapacity-mCount)*2);memset(generations+mCount,0,(mCapacity-mCount)*2);
        replace(objects,forward,reverse,generations);
    }
    mObjects[mCount]=member;mForward[mCount]=NxU16(mCount);mReverse[mCount]=NxU16(mCount);
    const NxU32 index=mCount++;return index|(NxU32(mGenerations[index])<<16);
}
void NxPrunerProcessPool::remove(NxU32 handle) {
    const NxU32 index=NxU16(handle);if(index>=mCapacity) return;
    const NxU16 slot=mForward[index];if(slot==0xffffu || slot>=mCapacity || !mCount || mGenerations[index]!=NxU16(handle>>16)) return;
    --mCount;mObjects[slot]=mObjects[mCount];mForward[mReverse[mCount]]=slot;
    mReverse[slot]=mReverse[mCount];mReverse[mCount]=NxU16(handle);mForward[index]=0xffffu;
    ++mFreeCount;++mGenerations[index];
}
void nxPrunerRegister(NxPrunerRegistration& member) {
    member.mTimestamp=0;
    if(!gNxPrunerProcessPool) {
        void* memory=nxGetSdkAllocator()->malloc(sizeof(NxPrunerProcessPool),NX_MEMORY_PERSISTENT);
        if(!memory) return; // shipped004830 leaves handle bytes untouched
        gNxPrunerProcessPool=::new(memory) NxPrunerProcessPool;
    }
    member.mHandle=gNxPrunerProcessPool->add(&member);
}
void nxPrunerUnregister(NxPrunerRegistration& member) { if(gNxPrunerProcessPool) gNxPrunerProcessPool->remove(member.mHandle); }
NxPrunerProcessPool* nxPrunerProcessPool() { return gNxPrunerProcessPool; }
void nxPrunerProcessPoolDestroy() {
    if(!gNxPrunerProcessPool) return;
    gNxPrunerProcessPool->~NxPrunerProcessPool();nxGetSdkAllocator()->free(gNxPrunerProcessPool);gNxPrunerProcessPool=nullptr;
}
void nxOpcodeReleasePool() { nxPrunerProcessPoolDestroy(); }
#endif
