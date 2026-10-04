// Focused internal/object-model harnesses compile private units without the
// full collision translation units. These link-only definitions keep those
// harnesses isolated; product and differential targets bind the real bodies.
#include "ContactGeneration.h"
#include "NarrowPhase.h"

void __cdecl NxContactCompoundShape(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

void __cdecl NxContactCompoundCompound(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

// NxPhysicsObjectLayoutTests constructs the dispatch matrix but does not drive
// any contact pair. Keep its private collision-unit stubs link-only as the
// production and differential targets bind the real ContactGeneration.cpp.
void __cdecl NxContactPlaneSphere(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactPlaneCapsule(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactPlaneBox(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactSphereSphere(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactSphereCapsule(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactSphereBox(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactCapsuleCapsule(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactBoxBox(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}
void __cdecl NxContactBoxCapsule(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*) {}

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

