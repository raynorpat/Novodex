// The internal static-proof, asset, object-layout, and shape-vtable harnesses
// link the real primitive contact table but do not exercise sphere/mesh pairs.
// Keep these dispatch-only targets independent of the full contact unit;
// product and collision-differential targets link ContactSphereMesh.cpp.
#include "ContactGeneration.h"
#include "NarrowPhase.h"

void __cdecl NxContactSphereMesh(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

bool __cdecl NxOverlapSphereMesh(const NxCollisionShape*, const NxCollisionShape*, void*)
	{
	return false;
	}
