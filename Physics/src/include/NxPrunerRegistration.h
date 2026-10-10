#ifndef NX_PRUNER_REGISTRATION_H
#define NX_PRUNER_REGISTRATION_H
#include "NxPhysicsBackend.h"
#include "NxSdkAllocator.h"
#include <stddef.h>
#if NX_PHYSICS_USE_X87
#error Pruner registration is the scalar fixed32 lifecycle implementation
#endif
// Ordinary non-owning member at Pruner+34..3c, never a placement overlay.
struct NxPrunerRegistration { NxU32 mHandle; NxU32 mTimestamp; };
// Shipped005438/005440/005442/005444/005446, successful-buffer domain.
class NxPrunerProcessPool {
public:
    NxPrunerProcessPool();
    ~NxPrunerProcessPool();
    NxU32 add(NxPrunerRegistration* member);
    void remove(NxU32 handle);
    NxPrunerRegistration** mObjects;
    NxU32 mCount;
    NxU32 mCapacity;
    NxU16* mForward;
    NxU16* mReverse;
    NxU16* mGenerations;
    NxU32 mFreeCount;
private:
    void replace(NxPrunerRegistration** objects,NxU16* forward,NxU16* reverse,NxU16* generations);
    NxPrunerProcessPool(const NxPrunerProcessPool&);
    NxPrunerProcessPool& operator=(const NxPrunerProcessPool&);
};
static_assert(sizeof(void*)==4,"process registration requires recovered fixed32 pointer layout");
static_assert(sizeof(NxPrunerRegistration)==8 && alignof(NxPrunerRegistration)==4,"actual eight-byte registration member");
static_assert(offsetof(NxPrunerRegistration,mHandle)==0 && offsetof(NxPrunerRegistration,mTimestamp)==4,"registration handle/timestamp layout");
static_assert(sizeof(NxPrunerProcessPool)==0x1c && alignof(NxPrunerProcessPool)==4,"actual fixed32 process owner");
static_assert(offsetof(NxPrunerProcessPool,mObjects)==0 && offsetof(NxPrunerProcessPool,mCount)==4 && offsetof(NxPrunerProcessPool,mCapacity)==8 && offsetof(NxPrunerProcessPool,mForward)==12 && offsetof(NxPrunerProcessPool,mReverse)==16 && offsetof(NxPrunerProcessPool,mGenerations)==20 && offsetof(NxPrunerProcessPool,mFreeCount)==24,"actual process owner offsets");
void nxPrunerRegister(NxPrunerRegistration& member);
void nxPrunerUnregister(NxPrunerRegistration& member);
NxPrunerProcessPool* nxPrunerProcessPool();
void nxPrunerProcessPoolDestroy();
void nxOpcodeReleasePool();
#endif
