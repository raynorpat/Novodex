#ifndef NX_PHYSICS_CORE_NPFIXEDJOINT
#define NX_PHYSICS_CORE_NPFIXEDJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public fixed joint object: 0x1c bytes, vtable 0x1011abf8
// (phys_data_002712, 33 slots) and the one-slot secondary table 0x1011ac7c.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Fixed" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxFixedJoint, FixedJoint>
// (core/NpJointShared.h): NxFixedJoint primary, the hook base
// secondary, the internal FixedJoint* at +0x18. NxFixedJoint
// adds only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxFixedJoint.h"
#include "ObjectModel.h"
#include "core/FixedJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpFixedJoint : public NpJointShared<NxFixedJoint, FixedJoint>
	{
	public:
	//! phys_fn_004561 (0x000b1a20, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxFixedJoint table 0x1011ab38, hook
	//! base (phys_fn_002404), secondary table 0x1011ac7c, `internal` at +0x18
	//! and +0x08, final table 0x1011abf8.
	explicit NpFixedJoint(FixedJoint* internal);

	//! phys_fn_004563 (0x000b1a60, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpFixedJoint>") for the second base.
	//! phys_fn_004565 (0x000b1a70, 55 B). Slot 0: reinstalls
	//! 0x1011abf8/0x1011ac7c, then the abstract NxJoint table 0x1011a680.
	virtual ~NpFixedJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x10 in every write-lock report) ---

	//! phys_fn_004541 (0x000b1670, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004543 (0x000b16d0, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004545 (0x000b1730, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004547 (0x000b1790, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004549 (0x000b17f0, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004555 (0x000b1910, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004551 (0x000b1860, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004553 (0x000b18b0, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxFixedJoint pure virtuals ---

	//! phys_fn_004557 (0x000b1960, 84 B). Slot 31: write lock (line 0x14),
	//! internal slot 9 (FixedJoint::loadFromDesc, phys_fn_004254).
	virtual void loadFromDesc(const NxFixedJointDesc&);

	//! phys_fn_004559 (0x000b19c0, 84 B). Slot 32: write lock (line 0x1f),
	//! internal slot 10 (FixedJoint::saveToDesc, phys_fn_004242).
	virtual void saveToDesc(NxFixedJointDesc&);
	};

static_assert(sizeof(NxFixedJoint) == sizeof(NxJoint), "NxFixedJoint adds no data members");
static_assert(sizeof(NpFixedJoint) == 0x1c, "NpFixedJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpFixedJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpFixedJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpFixedJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpFixedJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpFixedJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
