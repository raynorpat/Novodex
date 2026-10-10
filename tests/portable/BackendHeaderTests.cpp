#include "NxPhysicsBackend.h"

static_assert(NX_PHYSICS_USE_X87 == 0, "Standalone kernels require the scalar backend");
int main() { return 0; }
