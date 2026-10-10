#ifndef NX_SCENE_VISITED_BUFFERS_H
#define NX_SCENE_VISITED_BUFFERS_H
#include "NxPolygonScratch.h"
#include "Containers.h"
#include "NxScenePrunerCollection.h"
#include <new>
#include <string.h>

static_assert(sizeof(NxPolygonScratch)==0x18,"the sole actual Scene prefix ends at +18");
static_assert(alignof(NxPolygonScratch)==4,"actual scratch alignment");
inline NxPolygonScratch* nxSceneScratchConstruct(void* storage,NxU32 originalVptr)
{ return ::new(storage) NxPolygonScratch{originalVptr,0,nullptr,0,0,0}; }
inline void nxSceneScratchDestroy(NxPolygonScratch* scratch)
{ scratch->~NxPolygonScratch(); }
inline void nxSceneScratchReleaseBuffers(NxPolygonScratch& scratch)
{
    if(scratch.mVisited) { nxFoundationSDKAllocator->free(scratch.mVisited); scratch.mVisited=nullptr; }
    if(scratch.mOpaque0C) {
        nxFoundationSDKAllocator->free(reinterpret_cast<void*>(scratch.mOpaque0C));
        scratch.mOpaque0C=0;
    }
}
// Whole 000503's original successful-allocation domain, with explicit actual
// member receivers. Allocation failure/overflow are unchecked original states;
// this gate introduces no false success or undersized-state recovery policy.
inline void nxSceneVisitedBuffersUpdate(NxPolygonScratch& scratch,
    SdkContainer& first,SdkContainer& second,SdkContainer& third,
    NxScenePrunerCollection& pruners,NxU32 count)
{
    if(scratch.mVisitedCount>=count) return;
    scratch.mVisitedCount=(count+0x100u)&~0xffu;
    if(scratch.mVisited) { nxFoundationSDKAllocator->free(scratch.mVisited); scratch.mVisited=nullptr; }
    scratch.mVisited=static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(scratch.mVisitedCount*4,NX_MEMORY_PERSISTENT));
    if(scratch.mOpaque0C) { nxFoundationSDKAllocator->free(reinterpret_cast<void*>(scratch.mOpaque0C)); scratch.mOpaque0C=0; }
    NxU32* shared=static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(scratch.mVisitedCount*4,NX_MEMORY_PERSISTENT));
    scratch.mOpaque0C=reinterpret_cast<NxU32>(shared);
    memset(scratch.mVisited,0,scratch.mVisitedCount*4);
    scratch.mStamp=scratch.mVisitedCount;
    first.setExternalBuffer(scratch.mVisitedCount,shared);
    second.setExternalBuffer(scratch.mVisitedCount,shared);
    third.setExternalBuffer(scratch.mVisitedCount,shared);
    pruners.setExternalBuffer(scratch.mVisitedCount,shared);
}
#endif
