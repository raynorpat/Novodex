#include "NxPhysicsBackend.h"
#include "NxUtilities.h"
#if NX_PHYSICS_USE_X87
// Reference-only genuine Foundation import target; no scalar register bridge.
extern "C" void* _imp__NxFindRotationMatrix = reinterpret_cast<void*>(&NxFindRotationMatrix);
#endif
