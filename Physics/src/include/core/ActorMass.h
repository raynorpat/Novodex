#ifndef NX_PHYSICS_CORE_ACTOR_MASS
#define NX_PHYSICS_CORE_ACTOR_MASS
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Mass from shapes (actor-mass Task 1): the internal actor's mass computation
// phys_fn_000008 and the compound shape's slot 4, phys_fn_001024, which it
// reaches through the actor's root shape. The per-shape slot-4 rows and the
// 0x34-byte MassFrame they accumulate into are in ObjectModel.h/.cpp.

#include "Nxp.h"
#include "NxVec3.h"
#include "NxMat34.h"

class MassFrame;

// The root shape at actor body +0x10, as 000008 and 001024 call it: slot 4
// (`call [eax+0x10]`), __thiscall with three stack arguments and `ret 0xc` --
// the frame to merge into, the density, and a third word the listing passes
// as a pointer to a zeroed NxVec3 (000008) and forwards unchanged (001024).
// Every final shape table's slot 4 has this shape (box 000947, sphere 001371,
// capsule 001008, compound 001024, the base stub 001249 on the plane).
// Declared as an interface so the call is a real virtual dispatch; nothing
// implements it here.
struct ActorMassShape
	{
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual void slot2() = 0;
	virtual void slot3() = 0;
	virtual bool accumulateMass(MassFrame* destination, NxReal density,
		const NxVec3* reserved) = 0;
	};

// phys_fn_000008 (0x000010a0, 751 B; owner gap <start>..Actor.cpp).
// __thiscall on the 0x50-byte internal actor (the candidate's actor body at
// NxActor+0x14), four stack arguments, `ret 0x10`. Returns 0 on success, 1
// when the root shape's slot 4 fails, 2 when the shapes' mass is not
// positive (no non-trigger shape). Called by 000026 (the body creation the
// candidate models in nxActorComputeMass) and by 000164 (updateMassFromShapes,
// not reconstructed). The receiver is a fixture (the JointSupport.h
// convention); it is never constructed.
struct Row000008Fixture
	{
	NxU32 row000008(NxReal density, NxReal* totalMass, NxMat34* massLocalPose,
		NxVec3* massSpaceInertia);
	};

// phys_fn_001024 (0x000229b0, 87 B): the compound shape's slot 4. The shape
// array is at +0xe0 (begin) and +0xe4 (end).
struct CompoundShapeMass
	{
	bool row001024(MassFrame* destination, NxReal density, const NxVec3* reserved);
	};

// The compound shape's final table (.rdata 0x10106c2c, stored by the compound
// constructor 001033 at 0x22d76), with the slots the candidate has rows for:
// 1 phys_fn_001347, 2 phys_fn_001277, 4 phys_fn_001024, 7 phys_fn_001035,
// 12-14 phys_fn_001391. The group-specific rows 0 (0x22e50), 3 (0x22970),
// 5 (0x22a10), 6 (0x227d0), 8/10 (0xa0f60), 9 (0x22bf0) and 11 (0x22810) are
// not reconstructed and stay null.
void** nxCompoundShapeInternalVtable();

#endif
