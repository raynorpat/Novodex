// NxPhysicsInternalTests checks isolated object/layout invariants and never
// submits scene work. Scene.cpp and NpScene.cpp still reference these worker
// and pair-manager entry points, so keep link-only definitions local to that
// static-proof harness. Product and differential targets link the real bodies.
#include "BodyStep.h"
#include "ContactPairManager.h"

void Row000720Fixture::row000720() {}
void Row000724Fixture::row000724(void*, void*) {}
void Row000730Fixture::row000730(NxReal, NxReal) {}
void Row000762Fixture::row000762(void*) {}
void Row000764Fixture::row000764() {}
void Row000710Fixture::row000710(const NxVec3*) {}
void Row000726Fixture::row000726(NxReal, NxReal) {}
void Row000732Fixture::row000732(NxReal, NxReal) {}
void Row000770Fixture::row000770(NxReal, NxReal) {}

void nxSceneRefreshPairs(NxSceneInternal*) {}
void __cdecl cpmBufferContactReports0917(NxSceneInternal*, CpmPairHash*) {}
void cpmSetActorPairFlags(NxSceneInternal*, void*, void*, NxU32) {}
NxU32 cpmGetActorPairFlags(const NxSceneInternal*, const void*, const void*) { return 0; }
