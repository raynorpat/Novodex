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

// The dispatch-only harnesses compile PhysicsInternal.cpp without the mesh
// contact translation unit. The production DLL and collision differential
// bind the recovered implementations from ContactPlaneMesh.cpp.
void __cdecl NxContactPlaneMesh(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

bool __cdecl NxOverlapPlaneMesh(const NxCollisionShape*, const NxCollisionShape*, void*)
	{
	return false;
	}

// PhysicsInternal.cpp keeps the complete trigger-overlap dispatch table even
// in dispatch-only harnesses, which intentionally do not compile NarrowPhase.cpp.
// Those harnesses only construct the table; real simulation targets link the
// recovered overlap kernels from NarrowPhase.cpp.
bool __cdecl NxOverlapSphereSphere(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}

bool __cdecl NxOverlapSphereBox(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}

bool __cdecl NxOverlapSphereCapsule(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}

bool __cdecl NxOverlapBoxBox(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}

bool __cdecl NxOverlapBoxCapsule(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}

bool __cdecl NxOverlapCapsuleCapsule(const NxCollisionShape*, const NxCollisionShape*)
	{
	return false;
	}
