// Focused internal/object-model harnesses compile private units without the
// full collision translation units. These link-only definitions keep those
// harnesses isolated; product and differential targets bind the real bodies.
#include "ContactGeneration.h"
#include "NarrowPhase.h"

void __cdecl NxContactCompoundShape(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

void __cdecl NxContactCompoundCompound(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

// The focused object-layout and shape-vtable harnesses link PhysicsInternal.cpp
// to inspect the table object, but do not execute primitive collision rows.
// Keep these link-only definitions local to those harnesses; production and
// collision-differential targets bind the real implementations.
void __cdecl NxContactPlaneSphere(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactPlaneBox(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactPlaneCapsule(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactSphereSphere(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactSphereMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactSphereBox(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactSphereCapsule(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactBoxBox(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactBoxCapsule(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}
void __cdecl NxContactCapsuleCapsule(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*) {}

bool __cdecl NxOverlapPlaneSphere(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapPlaneBox(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapPlaneCapsule(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapSphereSphere(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapSphereMesh(const NxCollisionShape*, const NxCollisionShape*, void*) { return false; }
bool __cdecl NxOverlapSphereBox(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapSphereCapsule(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapBoxBox(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapBoxCapsule(const NxCollisionShape*, const NxCollisionShape*) { return false; }
bool __cdecl NxOverlapCapsuleCapsule(const NxCollisionShape*, const NxCollisionShape*) { return false; }

// NxPhysicsInternalTests links Scene.cpp without NarrowPhase.cpp. That target
// never drives scene pair-map teardown; the product and differential targets
// link the real 004157 implementation.
bool NxRemoveCollisionPairRecord(void*, NxU16, NxU16)
	{
	return false;
	}

// The internal-layout target compiles Scene.cpp without the contact generator;
// its focused fixtures do not exercise pruner sink reset.
void __fastcall NxContactSinkResetState(NxU32*)
	{
	}

