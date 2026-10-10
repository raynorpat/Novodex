// Reference-only IAT slot points at the genuine Foundation factory binding.
#include "NxUserAllocator.h"
#include "NxPhysicsBackend.h"
#if NX_PHYSICS_USE_X87
extern "C" void *nxTriangleMeshFoundationAllocatorSlot = &nxFoundationSDKAllocator;
#endif
