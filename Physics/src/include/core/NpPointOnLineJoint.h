#ifndef NX_PHYSICS_CORE_NPPOINTONLINEJOINT
#define NX_PHYSICS_CORE_NPPOINTONLINEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public point-on-line joint object: 0x1c bytes, vtable 0x1011aeb8
// (phys_data_002718, 33 slots) and the one-slot secondary table 0x1011af3c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## PointOnLine" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxPointOnLineJoint, PointOnLineJoint>
// (core/NpJointShared.h): NxPointOnLineJoint primary, the hook base
// secondary, the internal PointOnLineJoint* at +0x18. NxPointOnLineJoint adds
// only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxPointOnLineJoint.h"
#include "ObjectModel.h"
#include "core/PointOnLineJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpPointOnLineJoint : public NpJointShared<NxPointOnLineJoint, PointOnLineJoint>
	{
	public:
	//! phys_fn_004617 (0x000b2300, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxPointOnLineJoint table 0x1011adf8, hook
	//! base (phys_fn_002404), secondary table 0x1011af3c, `internal` at +0x18
	//! and +0x08, final table 0x1011aeb8.
	explicit NpPointOnLineJoint(PointOnLineJoint* internal);

	//! phys_fn_004619 (0x000b2340, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpPointOnLineJoint>") for the second base.
	//! phys_fn_004621 (0x000b2350, 55 B). Slot 0: reinstalls
	//! 0x1011aeb8/0x1011af3c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpPointOnLineJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x10 in every write-lock report) ---

	//! phys_fn_004597 (0x000b1f50, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004599 (0x000b1fb0, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004601 (0x000b2010, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004603 (0x000b2070, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004605 (0x000b20d0, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004611 (0x000b21f0, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004607 (0x000b2140, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004609 (0x000b2190, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxPointOnLineJoint pure virtuals ---

	//! phys_fn_004613 (0x000b2240, 84 B). Slot 31: write lock (line 0x14),
	//! internal slot 9 (PointOnLineJoint::loadFromDesc, phys_fn_004280).
	virtual void loadFromDesc(const NxPointOnLineJointDesc&);

	//! phys_fn_004615 (0x000b22a0, 84 B). Slot 32: write lock (line 0x1f),
	//! internal slot 10 (PointOnLineJoint::saveToDesc, phys_fn_004268).
	virtual void saveToDesc(NxPointOnLineJointDesc&);
	};

static_assert(sizeof(NxPointOnLineJoint) == sizeof(NxJoint), "NxPointOnLineJoint adds no data members");
static_assert(sizeof(NpPointOnLineJoint) == 0x1c, "NpPointOnLineJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpPointOnLineJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpPointOnLineJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpPointOnLineJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpPointOnLineJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpPointOnLineJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
