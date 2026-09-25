/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpCylindricalJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3b. The prismatic Np shape (core/NpPrismaticJoint.cpp):
// the NxJoint setters call NpJointShared's shared forward* bodies with this
// unit's __FILE__ and line; the 13 folded NxJoint bodies live in
// core/NpJointShared.cpp. Every write-locked NxJoint row reports line 0x10;
// loadFromDesc 0x15 and saveToDesc 0x20.

// The oracle's __FILE__ for this unit (the string at 0x1011b15c every
// write-lock report pushes, e.g. 0xb28f3).
#define NX_NPCYLINDRICALJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpCylindricalJoint.cpp"

// phys_fn_004675 (0x000b2c80, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor
// zeroes userData/appData (0xb2c8b/0xb2c8e), the transient
// NxCylindricalJoint table (0x1011b0d8), the secondary table (0x1011b21c) and
// the final table (0x1011b198) are installed by ordinary base/derived
// construction in the oracle's order, and the shared NpJointShared
// constructor zeroes the hook base's two words (phys_fn_002404) and stores
// `internal` at +0x18 and +0x08.
NpCylindricalJoint::NpCylindricalJoint(CylindricalJoint* internal)
	: NpJointShared<NxCylindricalJoint, CylindricalJoint>(internal)
	{
	}

// phys_fn_004677 (0x000b2cc0, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpCylindricalJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpCylindricalJoint() is defined.
// phys_fn_004679 (0x000b2cd0, 55 B)
NpCylindricalJoint::~NpCylindricalJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the Foundation allocator, slot +0x14).
	}

// phys_fn_004655 (0x000b28d0, 84 B)
void NpCylindricalJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPCYLINDRICALJOINT_CPP, 0x10, anchor);
	}

// phys_fn_004657 (0x000b2930, 84 B)
void NpCylindricalJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPCYLINDRICALJOINT_CPP, 0x10, axis);
	}

// phys_fn_004659 (0x000b2990, 89 B)
void NpCylindricalJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPCYLINDRICALJOINT_CPP, 0x10, maxForce, maxTorque);
	}

// phys_fn_004661 (0x000b29f0, 89 B)
void NpCylindricalJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPCYLINDRICALJOINT_CPP, 0x10, point, pointIsOnBody2);
	}

// phys_fn_004663 (0x000b2a50, 97 B)
bool NpCylindricalJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPCYLINDRICALJOINT_CPP, 0x10, normal, pointInPlane);
	}

// phys_fn_004665 (0x000b2ac0, 74 B)
void NpCylindricalJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPCYLINDRICALJOINT_CPP, 0x10);
	}

// phys_fn_004667 (0x000b2b10, 88 B)
void NpCylindricalJoint::setName(const char* name)
	{
	forwardSetName(NX_NPCYLINDRICALJOINT_CPP, 0x10, name);
	}

// phys_fn_004669 (0x000b2b70, 74 B)
void NpCylindricalJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPCYLINDRICALJOINT_CPP, 0x10);
	}

// phys_fn_004671 (0x000b2bc0, 84 B)
// Internal slot 9 (CylindricalJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb2c05).
void NpCylindricalJoint::loadFromDesc(const NxCylindricalJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPCYLINDRICALJOINT_CPP, 0x15, desc);
	}

// phys_fn_004673 (0x000b2c20, 84 B)
// Internal slot 10 (CylindricalJoint::saveToDesc, `[vt+0x28]`, 0xb2c65).
void NpCylindricalJoint::saveToDesc(NxCylindricalJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPCYLINDRICALJOINT_CPP, 0x20, desc);
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/CylindricalJoint.h. Not an oracle row; see the declaration.
NxJoint* nxCylindricalJointAttachScene(CylindricalJoint* internal, void* writeLink, void* readLink)
	{
	NpCylindricalJoint* np = static_cast<NpCylindricalJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
