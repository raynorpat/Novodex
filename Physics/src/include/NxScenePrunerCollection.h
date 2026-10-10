#ifndef NX_SCENE_PRUNER_COLLECTION_H
#define NX_SCENE_PRUNER_COLLECTION_H
#include "IcePrunable.h"
#include <new>
#include <stddef.h>

// Actual four-pointer member of the production engine at +1c (Scene+640).
// The engine retains ownership of pruners; this aggregate owns no allocation.
struct NxScenePrunerCollection
{
    Pruner* mPruners[4];
    void setExternalBuffer(udword capacity, udword* entries)
    {
        // 004861: four slots in ascending order; genuine virtual slot 4.
        for(unsigned i=0;i<4;++i)
            if(mPruners[i]) mPruners[i]->SetExternalBuffer(capacity,entries);
    }
};
static_assert(sizeof(void*)==4,"collection requires recovered Win32 layout; native remains Task9");
static_assert(sizeof(NxScenePrunerCollection)==0x10,"four original pointer slots");
static_assert(alignof(NxScenePrunerCollection)==4,"actual collection alignment");
static_assert(offsetof(NxScenePrunerCollection,mPruners)==0,"collection starts at engine+1c");
inline NxScenePrunerCollection* nxScenePrunerCollectionConstruct(void* storage)
{ return ::new(storage) NxScenePrunerCollection{}; }
#endif
