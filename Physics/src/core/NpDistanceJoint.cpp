/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpDistanceJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3f. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp (one of them, getActors
// 004539, sits at the end of this unit's address range). Every write-locked
// NxJoint row reports line 0x10; loadFromDesc 0x14 and saveToDesc 0x1f. The
// unit ends with its own constructor / thunk / destructor triple
// (004531-004535), followed by the compiler-generated ~NxJoint() body 004537
// (reuse, generated from NxJoint.h). See units/joint-families-contract.md
// "## Distance".

// The oracle's __FILE__ for this unit (the string at 0x1011aa5c every
// write-lock report pushes, e.g. 0xb11c3).
#define NX_NPDISTANCEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpDistanceJoint.cpp"

// phys_fn_004511 (0x000b11a0, 84 B)
void NpDistanceJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPDISTANCEJOINT_CPP, 0x10, anchor);
	}

// phys_fn_004513 (0x000b1200, 84 B)
void NpDistanceJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPDISTANCEJOINT_CPP, 0x10, axis);
	}

// phys_fn_004515 (0x000b1260, 89 B)
void NpDistanceJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPDISTANCEJOINT_CPP, 0x10, maxForce, maxTorque);
	}

// phys_fn_004517 (0x000b12c0, 89 B)
void NpDistanceJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPDISTANCEJOINT_CPP, 0x10, point, pointIsOnBody2);
	}

// phys_fn_004519 (0x000b1320, 97 B)
bool NpDistanceJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPDISTANCEJOINT_CPP, 0x10, normal, pointInPlane);
	}

// phys_fn_004521 (0x000b1390, 74 B)
void NpDistanceJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPDISTANCEJOINT_CPP, 0x10);
	}

// phys_fn_004523 (0x000b13e0, 88 B)
void NpDistanceJoint::setName(const char* name)
	{
	forwardSetName(NX_NPDISTANCEJOINT_CPP, 0x10, name);
	}

// phys_fn_004525 (0x000b1440, 74 B)
void NpDistanceJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPDISTANCEJOINT_CPP, 0x10);
	}

// phys_fn_004527 (0x000b1490, 84 B)
// Internal slot 9 (DistanceJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb14d5).
void NpDistanceJoint::loadFromDesc(const NxDistanceJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPDISTANCEJOINT_CPP, 0x14, desc);
	}

// phys_fn_004529 (0x000b14f0, 84 B)
// Internal slot 10 (DistanceJoint::saveToDesc, `[vt+0x28]`, 0xb1535).
void NpDistanceJoint::saveToDesc(NxDistanceJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPDISTANCEJOINT_CPP, 0x1f, desc);
	}

// phys_fn_004531 (0x000b1550, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb155b/0xb155e), the transient NxDistanceJoint table
// (0x1011a9d8), the secondary table (0x1011ab1c) and the final table
// (0x1011aa98) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpDistanceJoint::NpDistanceJoint(DistanceJoint* internal)
	: NpJointShared<NxDistanceJoint, DistanceJoint>(internal)
	{
	}

// phys_fn_004533 (0x000b1590, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpDistanceJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpDistanceJoint() is defined.
// phys_fn_004535 (0x000b15a0, 55 B)
NpDistanceJoint::~NpDistanceJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the Foundation allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/DistanceJoint.h. Not an oracle row; see the declaration.
NxJoint* nxDistanceJointAttachScene(DistanceJoint* internal, void* writeLink, void* readLink)
	{
	NpDistanceJoint* np = static_cast<NpDistanceJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
