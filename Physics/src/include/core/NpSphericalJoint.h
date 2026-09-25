#ifndef NX_PHYSICS_CORE_NPSPHERICALJOINT
#define NX_PHYSICS_CORE_NPSPHERICALJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public spherical joint object: 0x1c bytes, vtable 0x1011b028
// (phys_data_002721, 37 slots) and the one-slot secondary table 0x1011b0bc.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Spherical" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxSphericalJoint, SphericalJoint>
// (core/NpJointShared.h): NxSphericalJoint primary, the hook base secondary,
// the internal SphericalJoint* at +0x18. NxSphericalJoint adds
// loadFromDesc/saveToDesc (slots 31/32) and setFlags/getFlags/
// setProjectionMode/getProjectionMode (slots 33-36) to NxJoint.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxSphericalJoint.h"
#include "ObjectModel.h"
#include "core/SphericalJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpSphericalJoint : public NpJointShared<NxSphericalJoint, SphericalJoint>
	{
	public:
	//! phys_fn_004649 (0x000b2840, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxSphericalJoint table 0x1011af58, hook
	//! base (phys_fn_002404), secondary table 0x1011b0bc, `internal` at +0x18
	//! and +0x08, final table 0x1011b028.
	explicit NpSphericalJoint(SphericalJoint* internal);

	//! phys_fn_004651 (0x000b2880, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpSphericalJoint>") for the second base.
	//! phys_fn_004653 (0x000b2890, 55 B). Slot 0: reinstalls
	//! 0x1011b028/0x1011b0bc, then the abstract NxJoint table 0x1011a680.
	virtual ~NpSphericalJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0xf in every write-lock report) ---

	//! phys_fn_004623 (0x000b2390, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004625 (0x000b23f0, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004627 (0x000b2450, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004629 (0x000b24b0, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004631 (0x000b2510, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004639 (0x000b2670, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004633 (0x000b2580, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004637 (0x000b2610, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxSphericalJoint pure virtuals ---

	//! phys_fn_004641 (0x000b26c0, 84 B). Slot 31: write lock (line 0x13),
	//! internal slot 9 (SphericalJoint::loadFromDesc, phys_fn_004304).
	virtual void loadFromDesc(const NxSphericalJointDesc&);

	//! phys_fn_004643 (0x000b2720, 84 B). Slot 32: write lock (line 0x1e),
	//! internal slot 10 (SphericalJoint::saveToDesc, phys_fn_004286).
	virtual void saveToDesc(NxSphericalJointDesc&);

	//! phys_fn_004645 (0x000b2780, 84 B). Slot 33: write lock (line 0x27),
	//! internal slot 11 (SphericalJoint::setFlags, phys_fn_004288).
	virtual void setFlags(NxU32 flags);

	//! Slot 34: the oracle's table names NpRevoluteJoint's getFlags body
	//! (phys_fn_004703, folded; claimed by core/NpRevoluteJoint.cpp). Read
	//! lock, internal slot 12 (SphericalJoint::getFlags, phys_fn_004290).
	virtual NxU32 getFlags();

	//! phys_fn_004647 (0x000b27e0, 84 B). Slot 35: write lock (line 0x34),
	//! internal slot 13 (SphericalJoint::setProjectionMode, phys_fn_004292).
	virtual void setProjectionMode(NxJointProjectionMode projectionMode);

	//! Slot 36: the oracle's table names NpRevoluteJoint's
	//! getProjectionMode body (phys_fn_004707, folded; claimed by
	//! core/NpRevoluteJoint.cpp). Read lock, internal slot 14, unlock.
	virtual NxJointProjectionMode getProjectionMode();
	};

static_assert(sizeof(NxSphericalJoint) == sizeof(NxJoint), "NxSphericalJoint adds no data members");
static_assert(sizeof(NpSphericalJoint) == 0x1c, "NpSphericalJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpSphericalJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpSphericalJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpSphericalJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpSphericalJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpSphericalJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
