#ifndef NX_PHYSICS_CORE_NPPOINTINPLANEJOINT
#define NX_PHYSICS_CORE_NPPOINTINPLANEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public point-in-plane joint object: 0x1c bytes, vtable 0x1011ad58
// (phys_data_002715, 33 slots) and the one-slot secondary table 0x1011addc.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## PointInPlane" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxPointInPlaneJoint, PointInPlaneJoint>
// (core/NpJointShared.h): NxPointInPlaneJoint primary, the hook base
// secondary, the internal PointInPlaneJoint* at +0x18. NxPointInPlaneJoint
// adds only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxPointInPlaneJoint.h"
#include "ObjectModel.h"
#include "core/PointInPlaneJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpPointInPlaneJoint : public NpJointShared<NxPointInPlaneJoint, PointInPlaneJoint>
	{
	public:
	//! phys_fn_004591 (0x000b1ec0, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxPointInPlaneJoint table 0x1011ac98, hook
	//! base (phys_fn_002404), secondary table 0x1011addc, `internal` at +0x18
	//! and +0x08, final table 0x1011ad58.
	explicit NpPointInPlaneJoint(PointInPlaneJoint* internal);

	//! phys_fn_004593 (0x000b1f00, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpPointInPlaneJoint>") for the second base.
	//! phys_fn_004595 (0x000b1f10, 55 B). Slot 0: reinstalls
	//! 0x1011ad58/0x1011addc, then the abstract NxJoint table 0x1011a680.
	virtual ~NpPointInPlaneJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0xf in every write-lock report) ---

	//! phys_fn_004567 (0x000b1ab0, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004569 (0x000b1b10, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004571 (0x000b1b70, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004575 (0x000b1c00, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004579 (0x000b1c90, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004585 (0x000b1db0, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004581 (0x000b1d00, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004583 (0x000b1d50, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxPointInPlaneJoint pure virtuals ---

	//! phys_fn_004587 (0x000b1e00, 84 B). Slot 31: write lock (line 0x13),
	//! internal slot 9 (PointInPlaneJoint::loadFromDesc, phys_fn_004266).
	virtual void loadFromDesc(const NxPointInPlaneJointDesc&);

	//! phys_fn_004589 (0x000b1e60, 84 B). Slot 32: write lock (line 0x1e),
	//! internal slot 10 (PointInPlaneJoint::saveToDesc, phys_fn_004256).
	virtual void saveToDesc(NxPointInPlaneJointDesc&);
	};

static_assert(sizeof(NxPointInPlaneJoint) == sizeof(NxJoint), "NxPointInPlaneJoint adds no data members");
static_assert(sizeof(NpPointInPlaneJoint) == 0x1c, "NpPointInPlaneJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpPointInPlaneJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpPointInPlaneJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpPointInPlaneJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpPointInPlaneJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpPointInPlaneJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
