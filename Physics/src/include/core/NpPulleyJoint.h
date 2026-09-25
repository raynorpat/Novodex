#ifndef NX_PHYSICS_CORE_NPPULLEYJOINT
#define NX_PHYSICS_CORE_NPPULLEYJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public pulley joint object: 0x1c bytes, vtable 0x1011a938
// (phys_data_002706, 33 slots) and the one-slot secondary table 0x1011a9bc.
// See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Pulley" and "## Shared NpJoint slots".
//
// The shape is NpJointShared<NxPulleyJoint, PulleyJoint>
// (core/NpJointShared.h): NxPulleyJoint primary, the hook base
// secondary, the internal PulleyJoint* at +0x18. NxPulleyJoint
// adds only loadFromDesc/saveToDesc (slots 31/32) to NxJoint, so this class
// defines the per-family NxJoint rows and those two.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxPulleyJoint.h"
#include "ObjectModel.h"
#include "core/PulleyJoint.h"
#include "core/NpJointShared.h"

#include <cstddef>

class NpPulleyJoint : public NpJointShared<NxPulleyJoint, PulleyJoint>
	{
	public:
	//! phys_fn_004505 (0x000b1110, 57 B). The prismatic constructor's shape
	//! (phys_fn_004753): transient NxPulleyJoint table 0x1011a878, hook
	//! base (phys_fn_002404), secondary table 0x1011a9bc, `internal` at +0x18
	//! and +0x08, final table 0x1011a938.
	explicit NpPulleyJoint(PulleyJoint* internal);

	//! phys_fn_004507 (0x000b1150, 8 B) is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpPulleyJoint>") for the second base.
	//! phys_fn_004509 (0x000b1160, 55 B). Slot 0: reinstalls
	//! 0x1011a938/0x1011a9bc, then the abstract NxJoint table 0x1011a680.
	virtual ~NpPulleyJoint();

	// --- NxJoint setters this family has its own rows for, in NxJoint.h
	//     declaration order (line 0x10 in every write-lock report) ---

	//! phys_fn_004475 (0x000b0c60, 84 B). Slot 2 -> Joint::setGlobalAnchor.
	virtual void setGlobalAnchor(const NxVec3&);

	//! phys_fn_004477 (0x000b0cc0, 84 B). Slot 4 -> Joint::setGlobalAxis.
	virtual void setGlobalAxis(const NxVec3&);

	//! phys_fn_004481 (0x000b0d60, 89 B). Slot 9 -> Joint::setBreakable.
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004485 (0x000b0df0, 89 B). Slot 11 -> Joint::setLimitPoint.
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! phys_fn_004487 (0x000b0e50, 97 B). Slot 13 -> Joint::addLimitPlane.
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004495 (0x000b0fa0, 74 B). Slot 14 -> Joint::purgeLimitPlanes.
	virtual void purgeLimitPlanes();

	//! phys_fn_004489 (0x000b0ec0, 74 B). Slot 15 ->
	//! Joint::resetLimitPlaneIterator.
	virtual void resetLimitPlaneIterator();

	//! phys_fn_004493 (0x000b0f40, 88 B). Slot 29 ->
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	// --- NxPulleyJoint pure virtuals ---

	//! phys_fn_004501 (0x000b1050, 84 B). Slot 31: write lock (line 0x14),
	//! internal slot 9 (PulleyJoint::loadFromDesc, phys_fn_004226).
	virtual void loadFromDesc(const NxPulleyJointDesc&);

	//! phys_fn_004503 (0x000b10b0, 84 B). Slot 32: write lock (line 0x1f),
	//! internal slot 10 (PulleyJoint::saveToDesc, phys_fn_004216).
	virtual void saveToDesc(NxPulleyJointDesc&);
	};

static_assert(sizeof(NxPulleyJoint) == sizeof(NxJoint), "NxPulleyJoint adds no data members");
static_assert(sizeof(NpPulleyJoint) == 0x1c, "NpPulleyJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpPulleyJoint, userData) == 0x04, "NxJoint::userData is at +0x04");
static_assert(offsetof(NpPulleyJoint, appData) == 0x08, "NxJoint::appData is at +0x08");
static_assert(offsetof(NpPulleyJoint, mWord04) == 0x10, "the scene write-lock link is at +0x10");
static_assert(offsetof(NpPulleyJoint, mWord08) == 0x14, "the scene read-lock link is at +0x14");
static_assert(offsetof(NpPulleyJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
