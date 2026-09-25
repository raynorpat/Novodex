#ifndef NX_PHYSICS_CORE_NPCYLINDRICALJOINT
#define NX_PHYSICS_CORE_NPCYLINDRICALJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public cylindrical joint object: 0x1c bytes, vtable 0x1011b198
// (phys_data_002724, 33 slots) and the one-slot secondary table 0x1011b21c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Cylindrical" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxCylindricalJoint, CylindricalJoint>
// (core/NpJointShared.h): NxCylindricalJoint primary, the hook base
// secondary, the internal CylindricalJoint* at +0x18. NxCylindricalJoint adds
// only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxCylindricalJoint.h"
#include "ObjectModel.h"
#include "core/CylindricalJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpCylindricalJoint : public NpJointShared<NxCylindricalJoint, CylindricalJoint>
	{
	public:
	//! phys_fn_004675 (0x000b2c80, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxCylindricalJoint table 0x1011b0d8, hook
	//! base (phys_fn_002404), secondary table 0x1011b21c, `internal` at +0x18
	//! and +0x08, final table 0x1011b198.
	explicit NpCylindricalJoint(CylindricalJoint* internal);

	//! phys_fn_004677 (0x000b2cc0, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpCylindricalJoint>") for the second base.
	//! phys_fn_004679 (0x000b2cd0, 55 B). Slot 0: reinstalls
	//! 0x1011b198/0x1011b21c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpCylindricalJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x10 in every write-lock report) ---

	//! phys_fn_004655 (0x000b28d0, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004657 (0x000b2930, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004659 (0x000b2990, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004661 (0x000b29f0, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004663 (0x000b2a50, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004669 (0x000b2b70, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004665 (0x000b2ac0, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004667 (0x000b2b10, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxCylindricalJoint pure virtuals ---

	//! phys_fn_004671 (0x000b2bc0, 84 B). Slot 31: write lock (line 0x15),
	//! internal slot 9 (CylindricalJoint::loadFromDesc, phys_fn_004324).
	virtual void loadFromDesc(const NxCylindricalJointDesc&);

	//! phys_fn_004673 (0x000b2c20, 84 B). Slot 32: write lock (line 0x20),
	//! internal slot 10 (CylindricalJoint::saveToDesc, phys_fn_004316).
	virtual void saveToDesc(NxCylindricalJointDesc&);
	};

static_assert(sizeof(NxCylindricalJoint) == sizeof(NxJoint), "NxCylindricalJoint adds no data members");
static_assert(sizeof(NpCylindricalJoint) == 0x1c, "NpCylindricalJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpCylindricalJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpCylindricalJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpCylindricalJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpCylindricalJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpCylindricalJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
