// Actual Foundation binding, with the existing fixture IAT host boundary.
#include "NxUserAllocator.h"
#include "NxFoundationAllocatorAccess.inl"
#if NX_PHYSICS_USE_X87
extern "C" void* nxTriangleMeshFoundationAllocatorSlot = &nxFoundationSDKAllocator;
#endif
