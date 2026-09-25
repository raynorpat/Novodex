#ifndef NX_PHYSICS_CORE_NPPRISMATICJOINT
#define NX_PHYSICS_CORE_NPPRISMATICJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public prismatic joint object: 0x1c bytes, vtable 0x1011b4b8
// (phys_data_002730, 33 slots) and the one-slot secondary table 0x1011b53c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Prismatic" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxPrismaticJoint, PrismaticJoint>
// (core/NpJointShared.h): NxPrismaticJoint primary, the hook base secondary,
// the internal PrismaticJoint* at +0x18. NxPrismaticJoint adds only
// loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class defines
// the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxPrismaticJoint.h"
#include "ObjectModel.h"
#include "core/PrismaticJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpPrismaticJoint : public NpJointShared<NxPrismaticJoint, PrismaticJoint>
	{
	public:
	//! phys_fn_004753 (0x000b3810, 57 B). The revolute constructor's shape
	//! (phys_fn_004725): transient NxPrismaticJoint table 0x1011b3f8, hook
	//! base (phys_fn_002404), secondary table 0x1011b53c, `internal` at +0x18
	//! and +0x08, final table 0x1011b4b8.
	explicit NpPrismaticJoint(PrismaticJoint* internal);

	//! phys_fn_004755 (0x000b3850, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpPrismaticJoint>") for the second base.
	//! phys_fn_004757 (0x000b3860, 55 B). Slot 0: reinstalls
	//! 0x1011b4b8/0x1011b53c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpPrismaticJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0xf in every write-lock report) ---

	//! phys_fn_004731 (0x000b3430, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004733 (0x000b3490, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004735 (0x000b34f0, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004737 (0x000b3550, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004739 (0x000b35b0, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004747 (0x000b3700, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004741 (0x000b3620, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004745 (0x000b36a0, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxPrismaticJoint pure virtuals ---

	//! phys_fn_004749 (0x000b3750, 84 B). Slot 31: write lock (line 0x13),
	//! internal slot 9 (PrismaticJoint::loadFromDesc, phys_fn_004384).
	virtual void loadFromDesc(const NxPrismaticJointDesc&);

	//! phys_fn_004751 (0x000b37b0, 84 B). Slot 32: write lock (line 0x1e),
	//! internal slot 10 (PrismaticJoint::saveToDesc, phys_fn_004376).
	virtual void saveToDesc(NxPrismaticJointDesc&);
	};

static_assert(sizeof(NxPrismaticJoint) == sizeof(NxJoint), "NxPrismaticJoint adds no data members");
static_assert(sizeof(NpPrismaticJoint) == 0x1c, "NpPrismaticJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpPrismaticJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpPrismaticJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpPrismaticJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpPrismaticJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpPrismaticJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
