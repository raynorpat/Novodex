// The internal static-proof target links the real primitive contact table,
// but not the sphere/triangle-mesh implementations. Its layout checks never
// execute this pair; public/product targets link ContactSphereMesh.cpp.
#include "ContactGeneration.h"
#include "NarrowPhase.h"

void __cdecl NxContactSphereMesh(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

bool __cdecl NxOverlapSphereMesh(const NxCollisionShape*, const NxCollisionShape*, void*)
	{
	return false;
	}
