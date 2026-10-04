#include "NxUserAllocator.h"

// TriangleMeshTopology's reconstructed call sequence reads the Foundation
// allocator through the import-address slot at this image location. This
// focused executable supplies the same indirection to its local test allocator.
extern "C" void* nxTriangleMeshFoundationAllocatorSlot = &nxFoundationSDKAllocator;
