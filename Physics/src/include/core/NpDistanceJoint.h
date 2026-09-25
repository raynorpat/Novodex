#ifndef NX_PHYSICS_CORE_NPDISTANCEJOINT
#define NX_PHYSICS_CORE_NPDISTANCEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public distance joint object: 0x1c bytes, vtable 0x1011aa98
// (phys_data_002709, 33 slots) and the one-slot secondary table 0x1011ab1c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Distance" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxDistanceJoint, DistanceJoint>
// (core/NpJointShared.h): NxDistanceJoint primary, the hook base
// secondary, the internal DistanceJoint* at +0x18. NxDistanceJoint
// adds only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxDistanceJoint.h"
#include "ObjectModel.h"
#include "core/DistanceJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpDistanceJoint : public NpJointShared<NxDistanceJoint, DistanceJoint>
	{
	public:
	//! phys_fn_004531 (0x000b1550, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxDistanceJoint table 0x1011a9d8, hook
	//! base (phys_fn_002404), secondary table 0x1011ab1c, `internal` at +0x18
	//! and +0x08, final table 0x1011aa98.
	explicit NpDistanceJoint(DistanceJoint* internal);

	//! phys_fn_004533 (0x000b1590, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpDistanceJoint>") for the second base.
	//! phys_fn_004535 (0x000b15a0, 55 B). Slot 0: reinstalls
	//! 0x1011aa98/0x1011ab1c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpDistanceJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x10 in every write-lock report) ---

	//! phys_fn_004511 (0x000b11a0, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004513 (0x000b1200, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004515 (0x000b1260, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004517 (0x000b12c0, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004519 (0x000b1320, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004525 (0x000b1440, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004521 (0x000b1390, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004523 (0x000b13e0, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxDistanceJoint pure virtuals ---

	//! phys_fn_004527 (0x000b1490, 84 B). Slot 31: write lock (line 0x14),
	//! internal slot 9 (DistanceJoint::loadFromDesc, phys_fn_004238).
	virtual void loadFromDesc(const NxDistanceJointDesc&);

	//! phys_fn_004529 (0x000b14f0, 84 B). Slot 32: write lock (line 0x1f),
	//! internal slot 10 (DistanceJoint::saveToDesc, phys_fn_004230).
	virtual void saveToDesc(NxDistanceJointDesc&);
	};

static_assert(sizeof(NxDistanceJoint) == sizeof(NxJoint), "NxDistanceJoint adds no data members");
static_assert(sizeof(NpDistanceJoint) == 0x1c, "NpDistanceJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpDistanceJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpDistanceJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpDistanceJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpDistanceJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpDistanceJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
