#ifndef NX_PHYSICS_CORE_NPD6JOINT
#define NX_PHYSICS_CORE_NPD6JOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public D6 joint object: 0x1c bytes, vtable 0x1011a7c8
// (phys_data_002703, 37 slots) and the one-slot secondary table 0x1011a85c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## D6" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxD6Joint, D6Joint> (core/NpJointShared.h):
// NxD6Joint primary, the hook base secondary, the internal D6Joint* at +0x18.
// NxD6Joint adds loadFromDesc/saveToDesc (slots 31/32) and the four drive
// setters (slots 33-36) to NxJoint, so this class defines the per-family
// NxJoint rows and those six.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxD6Joint.h"
#include "ObjectModel.h"
#include "core/D6Joint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpD6Joint : public NpJointShared<NxD6Joint, D6Joint>
	{
	public:
	//! phys_fn_004469 (0x000b0bd0, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxD6Joint table 0x1011a700, hook base
	//! (phys_fn_002404), secondary table 0x1011a85c, `internal` at +0x18 and
	//! +0x08, final table 0x1011a7c8.
	explicit NpD6Joint(D6Joint* internal);

	//! phys_fn_004471 (0x000b0c10, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpD6Joint>") for the second base.
	//! phys_fn_004473 (0x000b0c20, 55 B). Slot 0: reinstalls
	//! 0x1011a7c8/0x1011a85c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpD6Joint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x11 in every write-lock report) ---

	//! phys_fn_004435 (0x000b0610, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004439 (0x000b06a0, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004445 (0x000b0760, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004447 (0x000b07c0, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004449 (0x000b0820, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004455 (0x000b0940, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004451 (0x000b0890, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004453 (0x000b08e0, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxD6Joint pure virtuals ---

	//! phys_fn_004457 (0x000b0990, 84 B). Slot 31: write lock (line 0x15),
	//! internal slot 9 (D6Joint::loadFromDesc, phys_fn_004212).
	virtual void loadFromDesc(const NxD6JointDesc&);

	//! phys_fn_004459 (0x000b09f0, 84 B). Slot 32: write lock (line 0x20),
	//! internal slot 10 (D6Joint::saveToDesc, phys_fn_004182).
	virtual void saveToDesc(NxD6JointDesc&);

	//! phys_fn_004461 (0x000b0a50, 84 B). Slot 33: write lock (line 0x28),
	//! D6Joint::setDrivePosition (the folded empty phys_fn_004248).
	virtual void setDrivePosition(const NxVec3& position);

	//! phys_fn_004463 (0x000b0ab0, 84 B). Slot 34: write lock (line 0x2f),
	//! D6Joint::setDriveOrientation (the folded empty phys_fn_004248).
	virtual void setDriveOrientation(const NxQuat& orientation);

	//! phys_fn_004465 (0x000b0b10, 84 B). Slot 35: write lock (line 0x36),
	//! D6Joint::setDriveLinearVelocity (the folded empty phys_fn_004248).
	virtual void setDriveLinearVelocity(const NxVec3& linVel);

	//! phys_fn_004467 (0x000b0b70, 84 B). Slot 36: write lock (line 0x3d),
	//! D6Joint::setDriveAngularVelocity (the folded empty phys_fn_004248).
	virtual void setDriveAngularVelocity(const NxVec3& angVel);
	};

static_assert(sizeof(NxD6Joint) == sizeof(NxJoint), "NxD6Joint adds no data members");
static_assert(sizeof(NpD6Joint) == 0x1c, "NpD6Joint is 0x1c bytes in the oracle");
static_assert(offsetof(NpD6Joint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpD6Joint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpD6Joint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpD6Joint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpD6Joint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
