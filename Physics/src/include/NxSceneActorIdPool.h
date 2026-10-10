#ifndef NX_SCENE_ACTOR_ID_POOL_H
#define NX_SCENE_ACTOR_ID_POOL_H
#include "NxSimpleTypes.h"
#include <cstddef>

// The real Scene +6d0 member (1430 take; 1b90 return). Successful coherent
// buffers only: the original has no allocation-failure/overflow recovery.
class NxSceneActorIdPool
{
public:
    NxSceneActorIdPool();
    ~NxSceneActorIdPool();
    NxU32 take();
    void returnId(NxU32 id);
    void releaseBuffer();

    NxU32 mNext;
    NxU32* mBegin;
    NxU32* mEnd;
    NxU32* mCapacity;
private:
    NxSceneActorIdPool(const NxSceneActorIdPool&);
    NxSceneActorIdPool& operator=(const NxSceneActorIdPool&);
};
static_assert(sizeof(void*)==4,"Scene ID member requires measured fixed32 pointers");
static_assert(sizeof(NxSceneActorIdPool)==0x10,"original Scene ID member extent");
static_assert(alignof(NxSceneActorIdPool)==4,"original Scene ID member alignment");
static_assert(offsetof(NxSceneActorIdPool,mNext)==0 && offsetof(NxSceneActorIdPool,mBegin)==4 &&
    offsetof(NxSceneActorIdPool,mEnd)==8 && offsetof(NxSceneActorIdPool,mCapacity)==12,
    "original Scene ID fields");
NxSceneActorIdPool* nxSceneActorIdPoolConstruct(void* storage);
void nxSceneActorIdPoolDestroy(NxSceneActorIdPool* pool);
class NxSceneInternal;
NxSceneActorIdPool& nxSceneActorIds(NxSceneInternal& scene);
#endif
