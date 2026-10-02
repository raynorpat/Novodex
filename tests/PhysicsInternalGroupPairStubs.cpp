// NxPhysicsInternalTests links PhysicsSDK.cpp without the production
// ContactPairManager.cpp translation unit. Its static layout checks do not call
// actor-group pair APIs, so supply link-only definitions for the process-wide
// pair map in this harness. Public SDK and simulation targets link the real map.
#include "ContactPairManager.h"

void cpmSetActorGroupPairFlags(NxU16, NxU16, NxU32) {}
NxU32 cpmGetActorGroupPairFlags(NxU16, NxU16) { return 0; }
void cpmResetActorGroupPairFlags() {}
