/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpSphericalJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3c. The prismatic Np shape (core/NpPrismaticJoint.cpp):
// the NxJoint setters call NpJointShared's shared forward* bodies with this
// unit's __FILE__ and line; the 13 folded NxJoint bodies live in
// core/NpJointShared.cpp. Every write-locked NxJoint row reports line 0xf;
// loadFromDesc 0x13, saveToDesc 0x1e, setFlags 0x27, setProjectionMode 0x34.

// The oracle's __FILE__ for this unit (the string at 0x1011afec every
// write-lock report pushes, e.g. 0xb23b3).
#define NX_NPSPHERICALJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpSphericalJoint.cpp"

// phys_fn_004649 (0x000b2840, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor
// zeroes userData/appData (0xb284b/0xb284e), the transient NxSphericalJoint
// table (0x1011af58), the secondary table (0x1011b0bc) and the final table
// (0x1011b028) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpSphericalJoint::NpSphericalJoint(SphericalJoint* internal)
	: NpJointShared<NxSphericalJoint, SphericalJoint>(internal)
	{
	}

// phys_fn_004651 (0x000b2880, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpSphericalJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpSphericalJoint() is defined.
// phys_fn_004653 (0x000b2890, 55 B)
NpSphericalJoint::~NpSphericalJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the SDK allocator, slot +0x14).
	}

// phys_fn_004623 (0x000b2390, 84 B)
void NpSphericalJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPSPHERICALJOINT_CPP, 0xf, anchor);
	}

// phys_fn_004625 (0x000b23f0, 84 B)
void NpSphericalJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPSPHERICALJOINT_CPP, 0xf, axis);
	}

// phys_fn_004627 (0x000b2450, 89 B)
void NpSphericalJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPSPHERICALJOINT_CPP, 0xf, maxForce, maxTorque);
	}

// phys_fn_004629 (0x000b24b0, 89 B)
void NpSphericalJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPSPHERICALJOINT_CPP, 0xf, point, pointIsOnBody2);
	}

// phys_fn_004631 (0x000b2510, 97 B)
bool NpSphericalJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPSPHERICALJOINT_CPP, 0xf, normal, pointInPlane);
	}

// phys_fn_004633 (0x000b2580, 74 B)
void NpSphericalJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPSPHERICALJOINT_CPP, 0xf);
	}

// phys_fn_004637 (0x000b2610, 88 B)
void NpSphericalJoint::setName(const char* name)
	{
	forwardSetName(NX_NPSPHERICALJOINT_CPP, 0xf, name);
	}

// phys_fn_004639 (0x000b2670, 74 B)
void NpSphericalJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPSPHERICALJOINT_CPP, 0xf);
	}

// phys_fn_004641 (0x000b26c0, 84 B)
// Internal slot 9 (SphericalJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb2705).
void NpSphericalJoint::loadFromDesc(const NxSphericalJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPSPHERICALJOINT_CPP, 0x13, desc);
	}

// phys_fn_004643 (0x000b2720, 84 B)
// Internal slot 10 (SphericalJoint::saveToDesc, `[vt+0x28]`, 0xb2765).
void NpSphericalJoint::saveToDesc(NxSphericalJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPSPHERICALJOINT_CPP, 0x1e, desc);
	}

// phys_fn_004645 (0x000b2780, 84 B)
// Internal slot 11 (SphericalJoint::setFlags, `[vt+0x2c]`, 0xb27c5).
void NpSphericalJoint::setFlags(NxU32 flags)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPSPHERICALJOINT_CPP, 0x27);
		return;
		}
	mInternal->setFlags(flags);
	nxNpSceneGuardLeave(link);
	}

// Slot 34. The oracle's table (0x1011b0b0) names NpRevoluteJoint's getFlags
// body at 0x000b3120 (folded; its stable-ID line is in
// core/NpRevoluteJoint.cpp): read lock, internal slot 12 (`[vt+0x30]`, here
// SphericalJoint::getFlags), unlock. The same body, not a second claim.
NxU32 NpSphericalJoint::getFlags()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxU32 flags = mInternal->getFlags();
	nxNpSceneGuardLeave(link);
	return flags;
	}

// phys_fn_004647 (0x000b27e0, 84 B)
// Internal slot 13 (SphericalJoint::setProjectionMode, `[vt+0x34]`,
// 0xb2825).
void NpSphericalJoint::setProjectionMode(NxJointProjectionMode projectionMode)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPSPHERICALJOINT_CPP, 0x34);
		return;
		}
	mInternal->setProjectionMode(projectionMode);
	nxNpSceneGuardLeave(link);
	}

// Slot 36. The oracle's table (0x1011b0b8) names NpRevoluteJoint's
// getProjectionMode body at 0x000b31b0 (folded; its stable-ID line is in
// core/NpRevoluteJoint.cpp): read lock, internal slot 14 (`[vt+0x38]`, the
// folded Joint projectionMode getter), unlock. The same body, not a second
// claim.
NxJointProjectionMode NpSphericalJoint::getProjectionMode()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointProjectionMode mode = mInternal->getProjectionMode();
	nxNpSceneGuardLeave(link);
	return mode;
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/SphericalJoint.h. Not an oracle row; see the declaration.
NxJoint* nxSphericalJointAttachScene(SphericalJoint* internal, void* writeLink, void* readLink)
	{
	NpSphericalJoint* np = static_cast<NpSphericalJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
